#include <windows.h>
#include <png.h>
#include <stdlib.h>
#include "trend.h"

extern HWND wnd;

FARPROC f(char *fun_name)
{
    HINSTANCE dll;
    
    dll=LoadLibrary("cygpng12.dll");
    if (!dll)
    {
        MessageBox(wnd, "Can't load cygpng12.dll", "Missing DLL", MB_ICONERROR);
        fprintf(stderr,"Can't load cygpng12.dll\n");
        abort();
    }
    return GetProcAddress(dll, fun_name);
}

pixel *read_png(char *file_name, int *width, int *height)
{
    FILE *fp;
    int i;
    pixel *pic;
    png_bytepp row_pointers;
    int bit_depth, color_type, interlace_type, compression_type, filter_method;

    if (!(fp=fopen(file_name, "rb")))
        return 0;

    png_structp png_ptr = (png_structp)(*f("png_create_read_struct"))
                          (PNG_LIBPNG_VER_STRING, 0,0,0);
    if (!png_ptr)
        return 0;

    png_infop info_ptr = (png_infop)(*f("png_create_info_struct"))(png_ptr);
    if (!info_ptr)
    {
        (*f("png_destroy_read_struct"))(&png_ptr, 0, 0);
        fclose(fp);
        return 0;
    }

    png_infop end_info = (png_infop)(*f("png_create_info_struct"))(png_ptr);
    if (!end_info)
    {
        (*f("png_destroy_read_struct"))(&png_ptr, &info_ptr, 0);
        fclose(fp);
        return 0;
    }

    (*f("png_init_io"))(png_ptr, fp);

    (*f("png_read_info"))(png_ptr, info_ptr);

    (*f("png_get_IHDR"))(png_ptr, info_ptr, (png_uint_32*)width, (png_uint_32*)height,
                 &bit_depth, &color_type, &interlace_type,
                 &compression_type, &filter_method);

    if (!(pic=malloc((*height)*(*width)*sizeof(pixel))))
    {
        (*f("png_destroy_read_struct"))(&png_ptr, &info_ptr, 0);
        fclose(fp);
        return 0;
    }

    row_pointers = (png_bytepp)(*f("png_malloc"))(png_ptr, (*height)*sizeof(png_bytep));
    for (i=0; i<*height; i++)
        row_pointers[i]=(png_bytep)(pic+i*(*width));
    (*f("png_set_rows"))(png_ptr, info_ptr, row_pointers);



    if (color_type == PNG_COLOR_TYPE_PALETTE)
        (*f("png_set_palette_to_rgb"))(png_ptr);

    if (color_type == PNG_COLOR_TYPE_GRAY &&
            bit_depth < 8) (*f("png_set_gray_1_2_4_to_8"))(png_ptr);

    if ((*f("png_get_valid"))(png_ptr, info_ptr,
                      PNG_INFO_tRNS)) (*f("png_set_tRNS_to_alpha"))(png_ptr);

    if (bit_depth == 16)
        (*f("png_set_strip_16"))(png_ptr);

    if (color_type == PNG_COLOR_TYPE_GRAY ||
            color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
        (*f("png_set_gray_to_rgb"))(png_ptr);

    if (color_type == PNG_COLOR_TYPE_RGB ||
            color_type == PNG_COLOR_TYPE_RGB_ALPHA)
        (*f("png_set_bgr"))(png_ptr);



    if (color_type == PNG_COLOR_TYPE_RGB)
        (*f("png_set_filler"))(png_ptr, 0, PNG_FILLER_AFTER);

    (*f("png_set_invert_alpha"))(png_ptr);

    (*f("png_read_image"))(png_ptr, row_pointers);



    (*f("png_read_end"))(png_ptr, 0);
    (*f("png_free"))(png_ptr, row_pointers);
    (*f("png_destroy_read_struct"))(&png_ptr, &info_ptr, 0);
    fclose(fp);

    return pic;
}
