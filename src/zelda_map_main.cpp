#include "../libs/sdl_include.hpp"
#include "../libs/stopwatch.hpp"

#include <filesystem>
#include <vector>
#include <thread>

namespace fs = std::filesystem;
namespace img = image;

using FileList = std::vector<fs::path>;

constexpr auto WATCH_DIR = "";

constexpr u32 MAP_WIDTH = 16;
constexpr u32 MAP_HEIGHT = 8;

constexpr u32 IMAGE_WIDTH = 10;
constexpr u32 IMAGE_HEIGHT = 10;

constexpr u32 SCREEN_SCALE = 2;

constexpr f64 NANO = 1'000'000'000;

constexpr f64 TARGET_FRAMERATE_HZ = 60.0;
constexpr f64 TARGET_NS_PER_FRAME = NANO / TARGET_FRAMERATE_HZ;


enum class RunState : int
{
    Start = 0,
    Running,
    End
};


class AppState
{
public:
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
    state.image_list.reserve(MAP_WIDTH * MAP_HEIGHT);

    auto map_w = MAP_WIDTH * IMAGE_WIDTH;
    auto map_h = MAP_HEIGHT * IMAGE_HEIGHT;

    if (!img::create_image(state.map_image, map_w, map_h))
    {
        return false;
    }

    state.map_view = img::make_view(state.map_image);

    auto screen_w = map_w * SCREEN_SCALE;
    auto screen_h = map_h * SCREEN_SCALE;

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