#include <png.h>
#include <stdlib.h>
#include "trend.h"

pixel *read_png(char *file_name, int *width, int *height)
{
    FILE *f;
    int i;
    pixel *pic;
    png_bytepp row_pointers;
    int bit_depth, color_type, interlace_type, compression_type, filter_method;

    if (!(f=fopen(file_name, "rb")))
        return 0;

    png_structp png_ptr = png_create_read_struct
                          (PNG_LIBPNG_VER_STRING, 0,0,0);
    if (!png_ptr)
        return 0;

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr)
    {
        png_destroy_read_struct(&png_ptr, 0, 0);
        fclose(f);
        return 0;
    }

    png_infop end_info = png_create_info_struct(png_ptr);
    if (!end_info)
    {
        png_destroy_read_struct(&png_ptr, &info_ptr, 0);
        fclose(f);
        return 0;
    }

    png_init_io(png_ptr, f);

    png_read_info(png_ptr, info_ptr);

    png_get_IHDR(png_ptr, info_ptr, (png_uint_32*)width, (png_uint_32*)height,
                 &bit_depth, &color_type, &interlace_type,
                 &compression_type, &filter_method);

    if (!(pic=malloc((*height)*(*width)*sizeof(pixel))))
    {
        png_destroy_read_struct(&png_ptr, &info_ptr, 0);
        fclose(f);
        return 0;
    }

    row_pointers = png_malloc(png_ptr, (*height)*sizeof(png_bytep));
    for (i=0; i<*height; i++)
        row_pointers[i]=(png_bytep)(pic+i*(*width));
    png_set_rows(png_ptr, info_ptr, row_pointers);



    if (color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_palette_to_rgb(png_ptr);

    if (color_type == PNG_COLOR_TYPE_GRAY &&
            bit_depth < 8) png_set_gray_1_2_4_to_8(png_ptr);

    if (png_get_valid(png_ptr, info_ptr,
                      PNG_INFO_tRNS)) png_set_tRNS_to_alpha(png_ptr);

    if (bit_depth == 16)
        png_set_strip_16(png_ptr);

    if (color_type == PNG_COLOR_TYPE_GRAY ||
            color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
        png_set_gray_to_rgb(png_ptr);

    if (color_type == PNG_COLOR_TYPE_RGB ||
            color_type == PNG_COLOR_TYPE_RGB_ALPHA)
        png_set_bgr(png_ptr);



    if (color_type == PNG_COLOR_TYPE_RGB)
        png_set_filler(png_ptr, 0, PNG_FILLER_AFTER);

    png_set_invert_alpha(png_ptr);

    png_read_image(png_ptr, row_pointers);



    png_read_end(png_ptr, 0);
    png_free(png_ptr, row_pointers);
    png_destroy_read_struct(&png_ptr, &info_ptr, 0);
    fclose(f);

    return pic;
}
