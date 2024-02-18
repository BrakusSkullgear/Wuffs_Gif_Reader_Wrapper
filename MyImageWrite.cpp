#include "MyImageWrite.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

void MyImageWrite::write_png(const std::string &file_name, unsigned char* rgba_data, int w, int h)
{
	stbi_write_png(file_name.c_str(), w, h, 4, rgba_data, 0);
} // End write_png.

