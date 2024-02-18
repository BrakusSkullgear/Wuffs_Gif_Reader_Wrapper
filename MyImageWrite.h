#pragma once
#include <string>

class MyImageWrite
{
public:

	// Write rgba data.
	static void write_png(const std::string &file_name, unsigned char* rgba_data, int w, int h);
}; // End MyImageWrite.

