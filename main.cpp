#include <iostream>
#include "MyGif.h"
#include "MyImageWrite.h"

int main()
{
	// Change file name here.
	std::string gif_file_name = "your_gif.gif";

	// Result image names: your_gif_0.png, your_gif_1.png etc.
	std::string result_base_name = "your_gif_";

	MyGifData gif_data;
	if (gif_data.load_from_file(gif_file_name))
	{
		MyInt2 gif_size = gif_data.get_gif_size();

		for (int i = 0; i < gif_data.get_frame_count(); ++i)
		{
			unsigned char* rgba_data = gif_data.get_frame_data(i);
			
			// Get full png name.
			std::string png_name = result_base_name + std::to_string(i) + ".png";
			MyImageWrite::write_png(png_name, rgba_data, gif_size.x, gif_size.y);
		}
	} // End load ok.

	// Failed.
	else
	{
		std::cout << "ERROR, can't load gif file " << gif_file_name << "!\n";
	} // End can't load file.

	return 0;

} // End main.
