#pragma once

#include "image.hpp"


#if defined(_WIN32)
#define SDL_MAIN_HANDLED
#endif

#include <SDL2/SDL.h>

#ifndef NDEBUG
#define PRINT_MESSAGES
#endif

#ifdef __EMSCRIPTEN__
#define NO_CONTROLLER
#define NO_WINDOW
#endif

#ifdef PRINT_MESSAGES
#include <cstdio>
#endif


namespace sdl
{
    static void print_message(const char* msg)
    {
    #ifdef PRINT_MESSAGES
        printf("%s\n", msg);
    #endif
    }
}


#define NO_CONTROLLER


namespace sdl
{
    constexpr auto SCREEN_BYTES_PER_PIXEL = 4;

#ifdef NO_CONTROLLER

    constexpr auto SDL_OPTIONS = SDL_INIT_VIDEO | SDL_INIT_AUDIO;

#else
    
    constexpr auto SDL_OPTIONS = SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER | SDL_INIT_HAPTIC;

#endif


    class EventInfo
    {
    public:
        SDL_Event event;
        bool has_event = false;
    };
}


namespace sdl
{
    static void print_error(const char* msg)
    {
#ifdef PRINT_MESSAGES
        printf("%s\n%s\n", msg, SDL_GetError());
#endif
    }


    static void close()
    {
        SDL_Quit();
    }


    static bool init()
    {    
        if (SDL_Init(SDL_OPTIONS) != 0)
        {
            print_error("SDL_Init failed");
            return false;
        }

        return true;
    }


    static void display_error(const char* msg)
    {
#ifndef NO_WINDOW
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "ERROR", msg, 0);
#endif

        print_error(msg);
    }


    static void handle_window_event(SDL_WindowEvent const& w_event)
    {
#ifndef NO_WINDOW

        auto window = SDL_GetWindowFromID(w_event.windowID);

        switch(w_event.event)
        {
            case SDL_WINDOWEVENT_SIZE_CHANGED:
            {

            }break;
            case SDL_WINDOWEVENT_EXPOSED:
            {
                
            } break;
        }

#endif
    }
    

    template <typename T>
    static void set_window_icon(SDL_Window* window, T const& icon_64)
    {
        // these masks are needed to tell SDL_CreateRGBSurface(From)
        // to assume the data it gets is byte-wise RGB(A) data
        Uint32 rmask, gmask, bmask, amask;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
        int shift = (icon_64.bytes_per_pixel == 3) ? 8 : 0;
        rmask = 0xff000000 >> shift;
        gmask = 0x00ff0000 >> shift;
        bmask = 0x0000ff00 >> shift;
        amask = 0x000000ff >> shift;
#else // little endian, like x86
        rmask = 0x000000ff;
        gmask = 0x0000ff00;
        bmask = 0x00ff0000;
        amask = (icon_64.bytes_per_pixel == 3) ? 0 : 0xff000000;
#endif

        SDL_Surface* icon = SDL_CreateRGBSurfaceFrom(
            (void*)icon_64.pixel_data,
            icon_64.width,
            icon_64.height,
            icon_64.bytes_per_pixel * 8,
            icon_64.bytes_per_pixel * icon_64.width,
            rmask, gmask, bmask, amask);

        SDL_SetWindowIcon(window, icon);

        SDL_FreeSurface(icon);
    }


    static void toggle_fullscreen(SDL_Window* window)
    {
        static bool is_fullscreen = false;

        if (is_fullscreen)
        {
            SDL_SetWindowFullscreen(window, 0);
        }
        else
        {
            SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
        }

        is_fullscreen = !is_fullscreen;
    }
}


/* screen memory */

namespace sdl
{
    class ScreenMemory
    {
    public:

        SDL_Window* window = nullptr;
        SDL_Renderer* renderer = nullptr;
        SDL_Texture* texture = nullptr;

        image::Image image;
        image::ImageView view;

        u32 window_width = 0;
        u32 window_height = 0;
    };


    static void destroy_screen_memory(ScreenMemory& screen)
    {
        if (screen.image.data_)
        {
            image::destroy_image(screen.image);
        }

        if (screen.texture)
        {
            SDL_DestroyTexture(screen.texture);
            screen.texture = nullptr;
        }

        if (screen.renderer)
        {
            SDL_DestroyRenderer(screen.renderer);
            screen.renderer = nullptr;
        }

        if(screen.window)
        {
            SDL_DestroyWindow(screen.window);
            screen.window = nullptr;
        }

        screen.window_width = 0;
        screen.window_height = 0;
    }


    namespace screen
    {
        static bool create_window(ScreenMemory& screen, cstr title, u32 width, u32 height)
        {
            screen.window = SDL_CreateWindow(
                title,
                SDL_WINDOWPOS_UNDEFINED,
                SDL_WINDOWPOS_UNDEFINED,
                (int)width,
                (int)height,
                SDL_WINDOW_RESIZABLE);

            if(!screen.window)
            {
                display_error("SDL_CreateWindow failed");
                return false;
            }

            screen.window_width = width;
            screen.window_height = height;

            return true;
        }


        static bool create_renderer(ScreenMemory& screen)
        {
            screen.renderer = SDL_CreateRenderer(screen.window, -1, 0);

            if(!screen.renderer)
            {
                display_error("SDL_CreateRenderer failed");
                return false;
            }

            return true;
        }


        static bool create_texture(ScreenMemory& screen, u32 width, u32 height)
        {
            screen.texture =  SDL_CreateTexture(
                screen.renderer,
                SDL_PIXELFORMAT_ABGR8888,
                SDL_TEXTUREACCESS_STREAMING,
                width,
                height);
            
            if(!screen.texture)
            {
                display_error("SDL_CreateTexture failed");
                return false;
            }

            return true;
        }


        static bool create_image(ScreenMemory& screen, u32 width, u32 height)
        {
            if(!image::create_image(screen.image, width, height))
            {
                display_error("Allocating image memory failed");
                return false;
            }

            screen.image.width = width;
            screen.image.height = height;

            screen.view = image::make_view(screen.image);

            return true;
        }
    }


    static bool create_screen_memory(ScreenMemory& screen, cstr title, u32 width, u32 height)
    {
        destroy_screen_memory(screen);

        if (!screen::create_window(screen, title, width, height))
        {
            destroy_screen_memory(screen);
            return false;
        }
        
        if (!screen::create_renderer(screen))
        {
            destroy_screen_memory(screen);
            return false;
        }

        if (!screen::create_texture(screen, width, height))
        {
            destroy_screen_memory(screen);
            return false;
        }

        if (!screen::create_image(screen, width, height))
        {
            destroy_screen_memory(screen);
            return false;
        }

        return true;
    }


    static bool create_screen_memory(ScreenMemory& screen, cstr title, Vec2Du32 screen_dim, Vec2Du32 window_dim)
    {
        destroy_screen_memory(screen);

        if (!screen::create_window(screen, title, window_dim.x, window_dim.y))
        {
            destroy_screen_memory(screen);
            return false;
        }
        
        if (!screen::create_renderer(screen))
        {
            destroy_screen_memory(screen);
            return false;
        }

        if (!screen::create_texture(screen, screen_dim.x, screen_dim.y))
        {
            destroy_screen_memory(screen);
            return false;
        }

        if (!screen::create_image(screen, screen_dim.x, screen_dim.y))
        {
            destroy_screen_memory(screen);
            return false;
        }

        return true;
    }


    static bool resize_screen_texture(ScreenMemory& screen, Vec2Du32 screen_dim)
    {
        if (screen.image.data_)
        {
            image::destroy_image(screen.image);
        }
        
        if (screen.texture)
        {
            SDL_DestroyTexture(screen.texture);
            screen.texture = nullptr;
        }

        if (!screen::create_texture(screen, screen_dim.x, screen_dim.y))
        {
            return false;
        }

        if (!screen::create_image(screen, screen_dim.x, screen_dim.y))
        {
            return false;
        }

        return true;
    }


    static void render_screen(ScreenMemory const& screen)
    {
        auto const pitch = screen.image.width * SCREEN_BYTES_PER_PIXEL;        

        #ifdef PRINT_MESSAGES

        auto error = SDL_UpdateTexture(screen.texture, 0, (void*)screen.image.data_, pitch);
        if(error)
        {
            print_error("SDL_UpdateTexture failed");
        }

        error = SDL_RenderCopy(screen.renderer, screen.texture, 0, 0);
        if(error)
        {
            print_error("SDL_RenderCopy failed");
        }

        #else
        SDL_UpdateTexture(screen.texture, 0, (void*)screen.image.data_, pitch);
        SDL_RenderCopy(screen.renderer, screen.texture, 0, 0);
        #endif
        
        SDL_RenderPresent(screen.renderer);
    }
}
