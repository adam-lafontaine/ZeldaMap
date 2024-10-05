#include "../libs/sdl_include.hpp"
#include "../libs/stopwatch.hpp"

#include <filesystem>
#include <thread>
#include <unordered_map>
#include <cassert>
#include <windows.h>

namespace fs = std::filesystem;
namespace img = image;


constexpr auto WATCH_DIR = "D:\\NES\\fceux\\snaps";
constexpr auto MAP_SAVE_PATH = "./zelda_map.png";

constexpr u32 MAP_WIDTH = 16;
constexpr u32 MAP_HEIGHT = 8;

constexpr u32 IMAGE_WIDTH = 256;
constexpr u32 IMAGE_HEIGHT = 168;

constexpr f32 SCREEN_SCALE = 0.4f;

constexpr f64 NANO = 1'000'000'000;

constexpr f64 TARGET_FRAMERATE_HZ = 60.0;
constexpr f64 TARGET_NS_PER_FRAME = NANO / TARGET_FRAMERATE_HZ;



enum class FileState : int
{
    New = 0,
    Existing,
    Deleted
};


using FileList = std::unordered_map<fs::path, FileState>;


static bool write_map(img::ImageView const& src, img::ImageView const& map)
{
    // mini-map at top of screen
    Rect2Du32 rm{};
    rm.x_begin = 16;
    rm.x_end = rm.x_begin + 64;
    rm.y_begin = 16;
    rm.y_end = rm.y_begin + 32;

    auto vm = img::sub_view(src, rm);

    u32 x = 0;
    u32 y = 0;
    bool found = false;
    for (y = 0; y < vm.height && !found; y++)
    {
        auto row = img::row_begin(vm, y);
        for (x = 0; x < vm.width && !found; x++)
        {
            auto p = row[x];
            found = p.red == 128 && p.green == 208 && p.blue == 16;
        }
    }

    if (!found)
    {
        return false;
    }

    x = (x - 1) / 4;
    y /= 4;

    auto r = img::make_rect(x * IMAGE_WIDTH, y * IMAGE_HEIGHT, IMAGE_WIDTH, IMAGE_HEIGHT);

    auto dst = img::sub_view(map, r);

    r = img::make_rect(0, src.height - IMAGE_HEIGHT, IMAGE_WIDTH, IMAGE_HEIGHT);

    img::copy(img::sub_view(src, r), dst);

    return true;
}


static bool update_map(fs::path const& parent_dir, FileList& image_list, img::ImageView const& map)
{
    bool update = false;

    img::Image image;
    for (auto& [path, state] : image_list)
    {
        if (state != FileState::New)
        {
            continue;
        }

        state = FileState::Existing;

        auto full_path = parent_dir / path;

        if (!img::read_image_from_file(full_path.generic_string().c_str(), image))
        {
            continue;
        }

        update |= write_map(img::make_view(image), map);

        img::destroy_image(image);
    }

    return update;
}    



enum class RunState : int
{
    Start = 0,
    Running,
    End
};


class AppState
{
public:
    fs::path watch_dir;
    HANDLE watch_dir_h;

    FileList image_list;
    img::Image map_image;
    img::ImageView map_view;
    
    sdl::ScreenMemory screen;

    RunState run_state;
};


static AppState state;


static bool is_running()
{
    return state.run_state == RunState::Running;
}


static void end_program()
{
    state.run_state = RunState::End;
}


static void monitor_image_directory(HANDLE dir, FileList& image_list)
{
    // https://gist.github.com/nickav/a57009d4fcc3b527ed0f5c9cf30618f8

    char buffer[1024] = { 0 };
    DWORD bytesReturned = 0;

    OVERLAPPED overlapped = {0};
    overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    auto success = ReadDirectoryChangesW(
        dir,                // handle to directory
        &buffer,             // buffer to store results
        sizeof(buffer),      // size of buffer
        FALSE,                // watch the directory tree
        FILE_NOTIFY_CHANGE_FILE_NAME |
        FILE_NOTIFY_CHANGE_DIR_NAME |
        FILE_NOTIFY_CHANGE_ATTRIBUTES |
        FILE_NOTIFY_CHANGE_SIZE |
        FILE_NOTIFY_CHANGE_LAST_WRITE |
        FILE_NOTIFY_CHANGE_CREATION,
        &bytesReturned,      // bytes returned
        &overlapped,                // overlapped structure
        NULL);

    while (is_running())
    {
        auto result = WaitForSingleObject(overlapped.hEvent, 0);

        if (result == WAIT_OBJECT_0) 
        {
            DWORD bytes_transferred;
            GetOverlappedResult(dir, &overlapped, &bytes_transferred, FALSE);

            FILE_NOTIFY_INFORMATION *event = (FILE_NOTIFY_INFORMATION*)buffer;            

            while (is_running())
            {
                std::wstring fileName(event->FileName, event->FileNameLength / sizeof(WCHAR));
                auto path = fs::path(fileName);
                if (path.extension() == ".png")
                {
                    switch (event->Action) 
                    {
                    case FILE_ACTION_ADDED: {
                        image_list[path] = FileState::New;
                        } break;

                    case FILE_ACTION_REMOVED: {
                        image_list[path] = FileState::Deleted;
                        } break;

                        /*case FILE_ACTION_MODIFIED: {
                            wprintf(L"    Modified: %.*s\n", name_len, event->FileName);
                        } break;

                        case FILE_ACTION_RENAMED_OLD_NAME: {
                        wprintf(L"Renamed from: %.*s\n", name_len, event->FileName);
                        } break;

                        case FILE_ACTION_RENAMED_NEW_NAME: {
                        wprintf(L"          to: %.*s\n", name_len, event->FileName);
                        } break;*/

                        default: {
                        //printf("Unknown action!\n");
                        } break;
                    }
                }

                // Are there more events to handle?
                if (event->NextEntryOffset) 
                {
                    *((uint8_t**)&event) += event->NextEntryOffset;
                } 
                else 
                {
                    break;
                }
            }
            
            // Queue the next event
            success = ReadDirectoryChangesW(
                dir, &buffer, sizeof(buffer), TRUE,
                FILE_NOTIFY_CHANGE_FILE_NAME  |
                FILE_NOTIFY_CHANGE_DIR_NAME   |
                FILE_NOTIFY_CHANGE_LAST_WRITE,
                NULL, &overlapped, NULL);

        }

        // reduce CPU usage
        std::this_thread::sleep_for(std::chrono::milliseconds((i64)(200)));
    }
}


static void save_map()
{
    img::write_to_file(state.map_view, MAP_SAVE_PATH);
}


static void set_window_icon(sdl::ScreenMemory const& screen)
{
#include "../res/icon_64.c"
    sdl::set_window_icon(screen.window, icon_64);
}


static void cap_framerate(Stopwatch& sw, f64 target_ns)
{
    constexpr f64 fudge = 0.9;

    auto sleep_ns = target_ns - sw.get_time_nano();
    if (sleep_ns > 0)
    {
        std::this_thread::sleep_for(std::chrono::nanoseconds((i64)(sleep_ns * fudge)));
    }

    sw.start();
}


static void handle_sdl_event(SDL_Event const& event, SDL_Window* window)
{
    switch(event.type)
    {
    case SDL_WINDOWEVENT:
        sdl::handle_window_event(event.window);
        break;

    case SDL_QUIT:
        sdl::print_message("SDL_QUIT");
        end_program();
        break;

    case SDL_KEYDOWN:
    case SDL_KEYUP:
    {
        auto key_code = event.key.keysym.sym;
        auto alt = event.key.keysym.mod & KMOD_ALT;

        if (alt)
        {
            switch (key_code)
            {
            case SDLK_F4:
                sdl::print_message("ALT F4");
                end_program();
                break;
#ifndef NDEBUG
            case SDLK_RETURN:
            case SDLK_KP_ENTER:
                sdl::print_message("ALT ENTER");
                sdl::toggle_fullscreen(window);
                break;
#endif            
            default:
                break;
            }
        }

        switch (key_code)
        {
        case SDLK_s:
            save_map();
            break;

#ifndef NDEBUG
        case SDLK_ESCAPE:
            sdl::print_message("ESC");
            end_program();
            break;
#endif

        default:
            break;
        } 

    } break;
        
    }
}


static void process_user_input()
{
    SDL_Event event;
    
    while (SDL_PollEvent(&event))
    {
        handle_sdl_event(event, state.screen.window);
    }
}


static bool open_watch_directory()
{
    state.watch_dir = fs::path(WATCH_DIR);
    if (!fs::exists(state.watch_dir) || !fs::is_directory(state.watch_dir))
    {
        sdl::print_message("No image directory");
        return false;
    }

    auto dir_w = state.watch_dir.wstring();

    state.watch_dir_h = CreateFileW(
        dir_w.c_str(),
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
        NULL);

    if (state.watch_dir_h == INVALID_HANDLE_VALUE) 
    {            
        return false;
    }

    return true;
}


static bool load_map()
{
    if (!fs::exists(MAP_SAVE_PATH))
    {
        assert("*** No map ***" && false);
        return false;
    }

    if (!img::read_image_from_file(MAP_SAVE_PATH, state.map_image))
    {
        assert("*** read failed ***" && false);
        return false;
    }

    auto map_w = MAP_WIDTH * IMAGE_WIDTH;
    auto map_h = MAP_HEIGHT * IMAGE_HEIGHT;

    if (state.map_image.width != map_w || state.map_image.height != map_h)
    {
        assert("*** Bad dims ***" && false);
        img::destroy_image(state.map_image);
        return false;
    }

    state.map_view = img::make_view(state.map_image);    

    return true;
}


static bool main_init()
{
    if (!open_watch_directory())
    {
        return false;
    }

    auto map_w = MAP_WIDTH * IMAGE_WIDTH;
    auto map_h = MAP_HEIGHT * IMAGE_HEIGHT;

    if (!load_map())
    {
        if (!img::create_image(state.map_image, map_w, map_h))
        {
            return false;
        }

        state.map_view = img::make_view(state.map_image);
        img::fill(state.map_view, img::to_pixel(0));
    }

    auto screen_w = (u32)(map_w * SCREEN_SCALE + 0.5f);
    auto screen_h = (u32)(map_h * SCREEN_SCALE + 0.5f);

    if (!sdl::create_screen_memory(state.screen, "Zelda Map", screen_w, screen_h))
    {
        return false;
    }

    set_window_icon(state.screen);
    img::resize(state.map_view, state.screen.view);

    return true;
}


static void main_close()
{
    save_map();
    CloseHandle(state.watch_dir_h);
    img::destroy_image(state.map_image);
    sdl::destroy_screen_memory(state.screen);
}


static void main_loop()
{
    auto const monitor_images = []()
    {
        monitor_image_directory(state.watch_dir_h, state.image_list);
    };

    std::thread th(monitor_images);

    Stopwatch sw;
    sw.start();

    while (is_running())
    {
        process_user_input();

        if (update_map(state.watch_dir, state.image_list, state.map_view))
        {
            img::resize(state.map_view, state.screen.view);
        }

        sdl::render_screen(state.screen);

        cap_framerate(sw, TARGET_NS_PER_FRAME);
    }
    
    th.join();
}



int main()
{
    if (!main_init())
    {
        return 1;
    }

    state.run_state = RunState::Running;

    main_loop();

    main_close();

    return 0;
}


#include "../libs/image.cpp"