#pragma once

#define STBI_NO_GIF
#define STBI_NO_PSD
#define STBI_NO_PIC
#define STBI_NO_PNM
#define STBI_NO_HDR
#define STBI_NO_TGA
#define STBI_NO_JPEG

//#define STBI_NO_PNG
//#define STBI_NO_BMP
//#define STBI_NO_SIMD
//#define STBI_NEON


#ifdef IMAGE_READ
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif


#ifdef IMAGE_WRITE
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#endif


#ifdef IMAGE_RESIZE
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize2.h"
#endif
