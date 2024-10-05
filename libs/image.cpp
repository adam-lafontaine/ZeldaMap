#pragma once

#include "image.hpp"

#define IMAGE_READ
#define IMAGE_WRITE
#define IMAGE_RESIZE
#include "stb_image/stb_image_options.hpp"


#include <cstdlib>
#include <cassert>
#include <cstring>


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


/* copy */

namespace image
{
    void copy(SubView const& src, ImageView const& dst)
    {
        assert(src.width == dst.width);
        assert(src.height == dst.height);

        for (u32 y = 0; y < src.height; y++)
        {
            auto s = row_begin(src, y);
            auto d = row_begin(dst, y);
            for (u32 x = 0; x < src.width; x++)
            {
                d[x] = s[x];
            }
        }
    }


    void copy(SubView const& src, SubView const& dst)
    {
        assert(src.width == dst.width);
        assert(src.height == dst.height);

        for (u32 y = 0; y < src.height; y++)
        {
            auto s = row_begin(src, y);
            auto d = row_begin(dst, y);
            for (u32 x = 0; x < src.width; x++)
            {
                d[x] = s[x];
            }
        }
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


/* read, write, resize */

namespace image
{
    static bool has_extension(const char* filename, const char* ext)
    {
        size_t file_length = std::strlen(filename);
        size_t ext_length = std::strlen(ext);

        return !std::strcmp(&filename[file_length - ext_length], ext);
    }


    static bool is_bmp(const char* filename)
    {
        return has_extension(filename, ".bmp") || has_extension(filename, ".BMP");
    }


    static bool is_png(const char* filename)
    {
        return has_extension(filename, ".png") || has_extension(filename, ".PNG");
    }


    bool read_image_from_file(const char* img_path_src, Image& image_dst)
	{
		int width = 0;
		int height = 0;
		int image_channels = 0;
		int desired_channels = 4;

		auto data = (Pixel*)stbi_load(img_path_src, &width, &height, &image_channels, desired_channels);

		assert(data);
		assert(width);
		assert(height);

		if (!data)
		{
			return false;
		}

		image_dst.data_ = data;
		image_dst.width = width;
		image_dst.height = height;

		return true;
	}


    bool write_to_file(ImageView const& image_src, const char* file_path_dst)
	{
		assert(image_src.width);
		assert(image_src.height);
		assert(image_src.matrix_data_);

		int width = (int)(image_src.width);
		int height = (int)(image_src.height);
		int channels = 4;
		auto const data = image_src.matrix_data_;

		int result = 0;

		if(is_bmp(file_path_dst))
		{
			result = stbi_write_bmp(file_path_dst, width, height, channels, data);
			assert(result && " *** stbi_write_bmp() failed *** ");
		}
		else if(is_png(file_path_dst))
		{
			int stride_in_bytes = width * channels;

			result = stbi_write_png(file_path_dst, width, height, channels, data, stride_in_bytes);
			assert(result && " *** stbi_write_png() failed *** ");
		}
		else
		{
			assert(false && " *** not a valid image format *** ");
		}

		return (bool)result;
	}


    bool resize(ImageView const& image_src, ImageView& image_dst)
	{
		assert(image_src.width);
		assert(image_src.height);
		assert(image_src.matrix_data_);
		assert(image_dst.width);
		assert(image_dst.height);

		int channels = 4;

		auto layout = stbir_pixel_layout::STBIR_RGBA;

		int width_src = (int)(image_src.width);
		int height_src = (int)(image_src.height);
		int stride_bytes_src = width_src * channels;

		int width_dst = (int)(image_dst.width);
		int height_dst = (int)(image_dst.height);
		int stride_bytes_dst = width_dst * channels;

		auto data = stbir_resize_uint8_linear(
			(u8*)image_src.matrix_data_, width_src, height_src, stride_bytes_src,
			(u8*)image_dst.matrix_data_, width_dst, height_dst, stride_bytes_dst,
			layout);

		assert(data && " *** resize_image failed *** ");

		if (!image_dst.matrix_data_)
		{
			image_dst.matrix_data_ = (Pixel*)data;
		}

		return (bool)data;
	}
}