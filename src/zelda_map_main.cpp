#include "../libs/sdl_include.hpp"
#include "../libs/stopwatch.hpp"

#include <filesystem>
#include <vector>
#include <thread>

namespace fs = std::filesystem;
namespace img = image;

using FileList = std::vector<fs::path>;

constexpr auto WATCH_DIR = "D:\\NES\\fceux\\snaps";

constexpr u32 MAP_WIDTH = 16;
constexpr u32 MAP_HEIGHT = 8;

constexpr u32 IMAGE_WIDTH = 256;
constexpr u32 IMAGE_HEIGHT = 168;

constexpr u32 SCREEN_DOWN_SCALE = 4;

constexpr f64 NANO = 1'000'000'000;

constexpr f64 TARGET_FRAMERATE_HZ = 60.0;
constexpr f64 TARGET_NS_PER_FRAME = NANO / TARGET_FRAMERATE_HZ;


namespace map
{
    static Rect2Du32 get_map_location(img::ImageView const& view)
    {
        // mini-map at top of screen
        Rect2Du32 rm{};
        rm.x_begin = 16;
        rm.x_end = rm.x_begin + 64;
        rm.y_begin = 16;
        rm.y_end = rm.y_begin + 32;

        auto vm = img::sub_view(view, rm);

        u32 x = 0;
        u32 y = 0;
        bool found = false;
        for (; y < vm.height && !found; y++)
        {
            auto row = img::row_begin(vm, y);
            for (; x < vm.width && !found; x++)
            {
                found = row[x].green > 200;
            }
        }

        x = (x - 1) / 4;
        y /= 4;

        return img::make_rect(x * IMAGE_WIDTH, y * IMAGE_HEIGHT, IMAGE_WIDTH, IMAGE_HEIGHT);
    }


    static void update_map(img::ImageView const& src, img::ImageView const& map)
    {
        auto dst = img::sub_view(map, get_map_location(src));

        auto r = img::make_rect(0, src.height - IMAGE_HEIGHT, IMAGE_WIDTH, IMAGE_HEIGHT);

        img::copy(img::sub_view(src, r), dst);
    }


    static void scan_dir()
    {

    }
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


#ifndef NDEBUG

        else
        {
            switch (key_code)
            {
            case SDLK_ESCAPE:
                sdl::print_message("ESC");
                end_program();
                break;

            default:
                break;
            }
        }

#endif           

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



static bool main_init()
{
    state.watch_dir = fs::path(WATCH_DIR);
    if (!fs::exists(state.watch_dir) || !fs::is_directory(state.watch_dir))
    {
        sdl::print_message("No image directory");
        return false;
    }
    
    state.image_list.reserve(MAP_WIDTH * MAP_HEIGHT);

    auto map_w = MAP_WIDTH * IMAGE_WIDTH;
    auto map_h = MAP_HEIGHT * IMAGE_HEIGHT;

    if (!img::create_image(state.map_image, map_w, map_h))
    {
        return false;
    }

    state.map_view = img::make_view(state.map_image);

    auto screen_w = map_w / SCREEN_DOWN_SCALE;
    auto screen_h = map_h / SCREEN_DOWN_SCALE;

    if (!sdl::create_screen_memory(state.screen, "Zelda Map", screen_w, screen_h))
    {
        return false;
    }

    set_window_icon(state.screen);

    img::fill(state.map_view, img::to_pixel(0));

    return true;
}


static void main_close()
{
    img::destroy_image(state.map_image);
    sdl::destroy_screen_memory(state.screen);
}


static void main_loop()
{
    Stopwatch sw;
    sw.start();

    while (is_running())
    {
        process_user_input();

        sdl::render_screen(state.screen);

        cap_framerate(sw, TARGET_NS_PER_FRAME);
    }
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