#pragma once

#include "types.hpp"


/*  image basic */

namespace image
{
    class RGBAu8
    {
    public:
        u8 red;
        u8 green;
        u8 blue;
        u8 alpha;
    };

    using Pixel = RGBAu8;
    using Image = Matrix2D<Pixel>;
    using ImageView = MatrixView2D<Pixel>;


    bool create_image(Image& image, u32 width, u32 height);

    void destroy_image(Image& image);
}


namespace image
{
    template <typename T>
    class MatrixSubView2D
    {
    public:
        T*  matrix_data_;
        u32 matrix_width;

        u32 x_begin;
        u32 y_begin;

        u32 width;
        u32 height;
    };


    using SubView = MatrixSubView2D<Pixel>;    
}


namespace image
{  
    constexpr inline Pixel to_pixel(u8 red, u8 green, u8 blue, u8 alpha)
    {
        Pixel p{};
        p.red = red;
        p.green = green;
        p.blue = blue;
        p.alpha = alpha;

        return p;
    }


    constexpr inline Pixel to_pixel(u8 red, u8 green, u8 blue)
    {
        return to_pixel(red, green, blue, 255);
    }


    constexpr inline Pixel to_pixel(u8 gray)
    {
        return to_pixel(gray, gray, gray);
    } 


    inline Rect2Du32 make_rect(u32 width, u32 height)
    {
        Rect2Du32 range{};
        range.x_begin = 0;
        range.x_end = width;
        range.y_begin = 0;
        range.y_end = height;

        return range;
    }


    inline Rect2Du32 make_rect(u32 x_begin, u32 y_begin, u32 width, u32 height)
    {
        Rect2Du32 range{};
        range.x_begin = x_begin;
        range.x_end = x_begin + width;
        range.y_begin = y_begin;
        range.y_end = y_begin + height;

        return range;
    }
}


/* row_begin */

namespace image
{
    template <typename T>
    static inline T* row_begin(MatrixView2D<T> const& view, u32 y)
    {
        return view.matrix_data_ + (u64)y * view.width;
    }


    template <typename T>
    static inline T* row_begin(MatrixSubView2D<T> const& view, u32 y)
    {
        return view.matrix_data_ + (u64)(view.y_begin + y) * view.matrix_width + view.x_begin;
    }
}


/* xy_at */

namespace image
{
    template <typename T>
    static inline T* xy_at(MatrixView2D<T> const& view, u32 x, u32 y)
    {
        return row_begin(view, y) + x;
    }


    template <typename T>
    static inline T* xy_at(MatrixSubView2D<T> const& view, u32 x, u32 y)
    {
        return row_begin(view, y) + x;
    }
}


/* make_view */

namespace image
{
    ImageView make_view(Image const& image);
}


/* sub_view */

namespace image
{
    template <typename T>
    inline MatrixSubView2D<T> sub_view(MatrixView2D<T> const& view, Rect2Du32 const& range)
    {
        MatrixSubView2D<T> sub_view{};

        sub_view.matrix_data_ = view.matrix_data_;
        sub_view.matrix_width = view.width;
        sub_view.x_begin = range.x_begin;
        sub_view.y_begin = range.y_begin;
        sub_view.width = range.x_end - range.x_begin;
        sub_view.height = range.y_end - range.y_begin;

        return sub_view;
    }


    template <typename T>
    inline MatrixSubView2D<T> sub_view(MatrixSubView2D<T> const& view, Rect2Du32 const& range)
    {
        MatrixSubView2D<T> sub_view{};

        sub_view.matrix_data_ = view.matrix_data_;
        sub_view.matrix_width = view.matrix_width;

        sub_view.x_begin = range.x_begin + view.x_begin;
		sub_view.y_begin = range.y_begin + view.y_begin;

		sub_view.width = range.x_end - range.x_begin;
		sub_view.height = range.y_end - range.y_begin;

        return sub_view;
    }


    template <typename T>
    inline MatrixSubView2D<T> sub_view(MatrixView2D<T> const& view)
    {
        auto range = make_range(view.width, view.height);
        return sub_view(view, range);
    }
}


/* copy */

namespace image
{
    void copy(SubView const& src, SubView const& dst);
}


/* fill */

namespace image
{
    void fill(ImageView const& view, Pixel color);

    void fill(SubView const& view, Pixel color);
}


/* read, write, resize */

namespace image
{
    bool read_image_from_file(const char* img_path_src, Image& image_dst);

    bool write_to_file(ImageView const& image_src, const char* file_path_dst);

    bool resize(ImageView const& image_src, ImageView& image_dst);
}