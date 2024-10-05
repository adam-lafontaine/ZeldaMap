#pragma once

#include "image.hpp"

#include <cstdlib>
#include <cassert>


namespace mem
{
    template <typename T>
    T* alloc(u32 n_elements)
    {
        return (T*)std::malloc(n_elements * sizeof(T));
    }


    template <typename T>
    static void free(T* data)
    {
        std::free(data);
    }
}


namespace image
{
    bool create_image(Image& image, u32 width, u32 height)
    {
        auto data = mem::alloc<Pixel>(width * height);
        if (!data)
        {
            return false;
        }

        image.data_ = data;
        image.width = width;
        image.height = height;

        return true;
    }

    
    void destroy_image(Image& image)
    {
        if (image.data_)
		{
			mem::free(image.data_);
			image.data_ = nullptr;
		}

		image.width = 0;
		image.height = 0;
    }
}


/* make_view */

namespace image
{
    ImageView make_view(Image const& image)
    {
        assert(image.data_);
        assert(image.width);
        assert(image.height);

        ImageView view{};

        view.width = image.width;
        view.height = image.height;
        view.matrix_data_ = image.data_;

        return view;
    }
}


/* fill */

namespace image
{
    void fill(ImageView const& view, Pixel color)
    {
        auto len = view.width * view.height;

        for (u32 i = 0; i < len; i++)
        {
            view.matrix_data_[i] = color;
        }
    }


    void fill(SubView const& view, Pixel color)
    {
        for (u32 y = 0; y < view.height; y++)
        {
            auto row = row_begin(view, y);
            for (u32 x = 0; x < view.width; x++)
            {
                row[x] = color;
            }
        }
    }
}


/* scale */

namespace image
{
    static void scale_view(ImageView const& src, SubView const& dst)
    {
        assert(dst.width % src.width == 0);
        assert(dst.height % src.height == 0);

        auto ws = dst.width / src.width;
        auto hs = dst.height / src.height;

        auto rect = make_rect(ws, hs);
        auto sub = sub_view(dst, rect);

        for (u32 y = 0; y < src.height; y++)
        {
            auto row = row_begin(src, y);
            for (u32 x = 0; x < src.width; x++)
            {
                auto p = row[x];
                sub = sub_view(dst, rect);

                fill(sub, p);

                rect.x_begin += ws;
                rect.x_end += ws;
            }

            rect.x_begin = 0;
            rect.x_end = ws;
            rect.y_begin += hs;
            rect.y_end += hs;
        }
    }
}