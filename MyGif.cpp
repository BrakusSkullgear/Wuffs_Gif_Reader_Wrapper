#include "MyGif.h"
#include <assert.h>
#include <iostream>
#include <fstream>

// Add wuffs here.
#define WUFFS_IMPLEMENTATION
#define WUFFS_CONFIG__STATIC_FUNCTIONS
#include "wuffs-v0.3.c"

//===========================================================================
// INT2.
MyInt2 MyInt2::ZERO = MyInt2(0, 0);

bool MyInt2::operator==(const MyInt2& other) const
{
	return x == other.x && y == other.y;
} // End operator==.

// END INT2.
//===========================================================================
// WUFFS INTERNAL GIF.
class WuffsGifImplementation
{
public:
	wuffs_base__io_buffer v_file_buffer;
	wuffs_gif__decoder v_decoder;

	wuffs_base__slice_u8 v_work_buffer = { 0 };
	wuffs_base__image_config v_image_config = { 0 };
	wuffs_base__pixel_buffer v_pixel_buffer = { 0 };

	void init_decoder(bool honor_background);
	void set_file_buffer(const char* data, int data_size);
	MyInt2 determine_size();
	int get_work_buffer_size() const;
	void set_work_buffer(unsigned char* data, int data_size);
	void set_pixel_buffer(unsigned char* data, int data_size);
	void get_frame_headers(std::vector<MyGifFrameHeader>& r_headers);

	// Setup buffer position.
	void set_buffer_position(int i_frame, int io_position);

	// Decode current frame.
	void decode_frame(MyGifPixelBuffer& r_buffers, bool init_refresh);
	int get_loop_count();

private:
	std::array<unsigned char, 4> get_background_rgba(const wuffs_base__frame_config& frame_config) const;

}; // End WuffsGifImplementation.

void WuffsGifImplementation::init_decoder(bool honor_background)
{
	// Decoder and status.
	wuffs_base__status i_status = wuffs_gif__decoder__initialize(&v_decoder, sizeof(v_decoder), WUFFS_VERSION, 0);

	// See if status good.
	if (!wuffs_base__status__is_ok(&i_status))
	{
		std::cout << wuffs_base__status__message(&i_status) << "\n";
		std::terminate();
	} // End if no good.

	// See if background setup.
	if (honor_background)
	{
		wuffs_gif__decoder__set_quirk_enabled(&v_decoder, WUFFS_GIF__QUIRK_HONOR_BACKGROUND_COLOR, true);
	}
} // End init_file_decoder.


void WuffsGifImplementation::set_file_buffer(const char* data, int data_size)
{
	v_file_buffer.data.ptr = (uint8_t*)data;
	v_file_buffer.data.len = data_size;
	v_file_buffer.meta.wi = data_size;
	v_file_buffer.meta.ri = 0;
	v_file_buffer.meta.pos = 0;
	v_file_buffer.meta.closed = true;
} // End set_file_buffer.

MyInt2 WuffsGifImplementation::determine_size()
{
	// Get status.
	wuffs_base__status dic_status = wuffs_gif__decoder__decode_image_config(&v_decoder, &v_image_config, &v_file_buffer);

	// See if bad status.
	if (!wuffs_base__status__is_ok(&dic_status))
	{
		std::cout << wuffs_base__status__message(&dic_status) << "\n";
		std::terminate();
	} // End if bad status.

	// Config bad.
	if (!wuffs_base__image_config__is_valid(&v_image_config))
	{
		std::cout << "invalid image configuration" << "\n";
		std::terminate();
	} // End if config bad.

	// Get image dimensions.
	uint32_t width = wuffs_base__pixel_config__width(&v_image_config.pixcfg);
	uint32_t height = wuffs_base__pixel_config__height(&v_image_config.pixcfg);

	// See if too big image.
	if ((width > 4096) || (height > 4096))
	{
		std::cout << "GIF image dimensions are too large\n";
		std::terminate();
	} // End if too big image.

	// Override the source's indexed pixel format to be non-indexed.
	{
		const uint32_t pixel_format = WUFFS_BASE__PIXEL_FORMAT__RGBA_NONPREMUL;
		const uint32_t sub_sampling = WUFFS_BASE__PIXEL_SUBSAMPLING__NONE;

		wuffs_base__pixel_config__set(&v_image_config.pixcfg, pixel_format, sub_sampling, width, height);

	} // End setup config.

	// Get size.
	return MyInt2((int)width, (int)height);

} // End determine_size.

int WuffsGifImplementation::get_work_buffer_size() const
{
	return (int)wuffs_gif__decoder__workbuf_len(&v_decoder).max_incl;
} // End get_work_buffer_size.


void WuffsGifImplementation::set_work_buffer(unsigned char* data, int data_size)
{
	v_work_buffer = wuffs_base__make_slice_u8(data, data_size);
} // End set_work_buffer.

void WuffsGifImplementation::set_pixel_buffer(unsigned char* data, int data_size)
{
	// Get slice from data.
	wuffs_base__slice_u8 base_slice = wuffs_base__make_slice_u8(data, data_size);

	// Build pixel buffer.
	wuffs_base__status sfs0_status = wuffs_base__pixel_buffer__set_from_slice(&v_pixel_buffer, &v_image_config.pixcfg, base_slice);

	// See if faulty status.
	if (!wuffs_base__status__is_ok(&sfs0_status))
	{
		std::cout << wuffs_base__status__message(&sfs0_status) << "\n";
		std::terminate();
	} // End faulty status.

} // End set_pixel_buffer.

// Get showtimes.
void WuffsGifImplementation::get_frame_headers(std::vector<MyGifFrameHeader>& r_headers)
{
	// Get frame count.
	while (true)
	{
		wuffs_base__frame_config fc = { 0 };
		wuffs_base__status dfc_status = wuffs_gif__decoder__decode_frame_config(&v_decoder, &fc, &v_file_buffer);

		// Not good.
		if (!wuffs_base__status__is_ok(&dfc_status))
		{


			// See if done.
			if (dfc_status.repr == wuffs_base__note__end_of_data)
			{
				break;
			} // End if done.

			// Add frame.
			else
			{
				std::cout << wuffs_base__status__message(&dfc_status) << "\n";
				break;
			} // End add_frame.

		} // End if not good.

		// Good.
		else
		{
			wuffs_base__flicks flicks = wuffs_base__frame_config__duration(&fc);
			int io_position = wuffs_base__frame_config__io_position(&fc);

			// Get frames in 60fps.
			double res = (double)flicks / 11760000;
			int n_frames = round(res);

			MyGifFrameHeader header;
			header.v_flicks = flicks;
			header.v_io_position = io_position;
			header.v_show_time = n_frames;
			r_headers.push_back(header);

		} // End is good.

	} // End while go through.

} // End get_show_times.

void WuffsGifImplementation::set_buffer_position(int i_frame, int io_position)
{
	v_file_buffer.meta.ri = io_position;
	wuffs_base__status status = wuffs_gif__decoder__restart_frame(&v_decoder, i_frame, v_file_buffer.reader_position());
	if (status.is_error()) std::cout << "Set buffer position fail\n";

} // End set_buffer_position. 

std::array<unsigned char, 4> WuffsGifImplementation::get_background_rgba(const wuffs_base__frame_config& frame_config) const
{
	// Get color.
	wuffs_base__color_u32_argb_premul argb_premul = wuffs_base__frame_config__background_color(&frame_config);
	uint32_t argb_nonpremul = wuffs_base__color_u32_argb_premul__as__color_u32_argb_nonpremul(argb_premul);
	
	// Convert.
	unsigned char a = (unsigned char)(0xFF & (argb_nonpremul >> 24));
	unsigned char r = (unsigned char)(0xFF & (argb_nonpremul >> 16));
	unsigned char g = (unsigned char)(0xFF & (argb_nonpremul >> 8));
	unsigned char b = (unsigned char)(0xFF & (argb_nonpremul >> 0));
	
	// Get as rgba.
	std::array<unsigned char, 4> color = { r,g,b,a };
	return color;

} // End get_background_rgba.

void WuffsGifImplementation::decode_frame(MyGifPixelBuffer& r_buffers, bool init_refresh)
{
	wuffs_base__frame_config frame_config = { 0 };
	wuffs_base__status dfc_status = wuffs_gif__decoder__decode_frame_config(&v_decoder, &frame_config, &v_file_buffer);

	// See if status not good.
	if (!wuffs_base__status__is_ok(&dfc_status))
	{
		// See if done.
		if (dfc_status.repr == wuffs_base__note__end_of_data)
		{
			return;
		} // End if done.

		// Otherwise error.
		else
		{
			std::cout << wuffs_base__status__message(&dfc_status) << "\n";
			std::terminate();
		}
	} // End if status not good.

	// See if first index of the gif. Write background.
	if (wuffs_base__frame_config__index(&frame_config) == 0 || init_refresh)
	{
		std::array<unsigned char, 4> color = get_background_rgba(frame_config);
		r_buffers.clear_background(color);
	} // End config index.

	// See if something.
	switch (wuffs_base__frame_config__disposal(&frame_config))
	{
		// Copy current buffer to previous.
		case WUFFS_BASE__ANIMATION_DISPOSAL__RESTORE_PREVIOUS:
		{
			r_buffers.set_previous();
		} break;
	} // End if something.

	// See if overwrite.
	bool want_overwrite = wuffs_base__frame_config__overwrite_instead_of_blend(&frame_config);
	wuffs_base__pixel_blend pixel_blend;
	if (want_overwrite)
	{
		pixel_blend = WUFFS_BASE__PIXEL_BLEND__SRC;
	}
	else
	{
		pixel_blend = WUFFS_BASE__PIXEL_BLEND__SRC_OVER;
	}

	// Decode finally new data.
	wuffs_base__status decode_frame_status = wuffs_gif__decoder__decode_frame(&v_decoder, &v_pixel_buffer, &v_file_buffer,
		pixel_blend, v_work_buffer, NULL);

	// Copy to result. Check for opengl.
	r_buffers.write_result();

	// See if done.
	if (decode_frame_status.repr == wuffs_base__note__end_of_data)
	{
		return;
	} // End if done.

	// Got frame now in current destination buffer.
	switch (wuffs_base__frame_config__disposal(&frame_config))
	{
		// Restoring background to window.
		case WUFFS_BASE__ANIMATION_DISPOSAL__RESTORE_BACKGROUND:
		{
			// Get window.
			wuffs_base__rect_ie_u32 wuff_rect = wuffs_base__frame_config__bounds(&frame_config);
			MyInt2 top_left(wuff_rect.min_incl_x, wuff_rect.min_incl_y);
			MyInt2 bottom_right(wuff_rect.max_excl_x, wuff_rect.max_excl_y);

			// Get color.
			std::array<unsigned char, 4> color = get_background_rgba(frame_config);
			r_buffers.write_background(top_left, bottom_right, color);

		} break;

		// Set previous data.
		case WUFFS_BASE__ANIMATION_DISPOSAL__RESTORE_PREVIOUS:
		{
			r_buffers.swap_buffers();
			set_pixel_buffer(r_buffers.p_current_buffer, r_buffers.get_byte_size());
		} break;

	} // End if disposal after.

	// See if failing somehow.
	if (!wuffs_base__status__is_ok(&decode_frame_status))
	{
		std::cout << wuffs_base__status__message(&decode_frame_status) << "\n";
		std::terminate();
	} // End if failing somehow.

} // End decode_frame.

int WuffsGifImplementation::get_loop_count()
{
	return (int)wuffs_gif__decoder__num_animation_loops(&v_decoder);
} // End get_loop_count.



// END INTERNAL WUFFS.
//===========================================================================




//===========================================================================
// PIXEL BUFFER.
MyGifPixelBuffer::MyGifPixelBuffer()
{
	p_current_buffer = p_previous_buffer = p_result_buffer = nullptr;
	v_flip_y = false; // Set to true if want upside down results.
} // End constructor.

int MyGifPixelBuffer::get_byte_size() const
{
	return v_size.x * v_size.y * 4;
} // End get_byte_size.

void MyGifPixelBuffer::swap_buffers()
{
	std::swap(p_current_buffer, p_previous_buffer);
} // End swap_buffers.

void MyGifPixelBuffer::set_previous()
{
	memcpy(p_previous_buffer, p_current_buffer, get_byte_size());
} // End set_previous.

void MyGifPixelBuffer::write_result()
{
	if (v_flip_y)
	{
		int byte_size = get_byte_size();
		int stride = 4 * v_size.x;

		unsigned char* src_row = p_current_buffer;
		unsigned char* dest_row = p_result_buffer + (byte_size - stride);
		unsigned char* src_end = src_row + byte_size;

		while (src_row < src_end)
		{
			// Copy data.
			memcpy(dest_row, src_row, stride);

			// Add strides.
			dest_row -= stride;
			src_row += stride;
		} // End go through rows.
	} // End flipping y.

	else
	{
		memcpy(p_result_buffer, p_current_buffer, get_byte_size());
	} // End not flipping y.

} // End write_result.

void MyGifPixelBuffer::clear_background(std::array<unsigned char,4> color)
{
	int n = v_size.x * v_size.y;
	unsigned char* d = p_current_buffer;
	for (int i = 0; i < n; ++i)
	{
		d[0] = color[0];
		d[1] = color[1];
		d[2] = color[2];
		d[3] = color[3];
		d += sizeof(color);
	}
} // End clear_background.

void MyGifPixelBuffer::write_background(const MyInt2& top_left, const MyInt2& bottom_right, std::array<unsigned char, 4> color)
{
	int stride = 4 * v_size.x;

	// Go through rows.
	for (int y = top_left.y; y < bottom_right.y; ++y)
	{
		// Get row.
		unsigned char* row = p_current_buffer + y * stride;

		// Shift.
		row += 4 * top_left.x;

		// Go through.
		for (int x = top_left.x; x < bottom_right.x; ++x)
		{
			row[0] = color[0];
			row[1] = color[1];
			row[2] = color[2];
			row[3] = color[3];
			row += sizeof(color);
		} // End x.

	} // End y.
} // End write_background.

// END PIXEL BUFFER.
//===========================================================================
// GIF DATA.

MyGifData::MyGifData()
{
	m_implementation = nullptr;
	m_triple_pixel_buffer = nullptr;
	m_work_buffer = nullptr;

	v_frame_count = 0;
	v_work_buffer_size = 0;
	v_result_index = -1;
	v_loop_count = 0;

} // End constructor.

MyGifData::~MyGifData()
{
	destroy();
} // End destructor.

bool MyGifData::load_from_file(const std::string& file_name)
{
	std::ifstream bin_file(file_name, std::ios::binary);

	// All good.
	if (bin_file.good())
	{
		v_file_data.assign((std::istreambuf_iterator<char>(bin_file)), (std::istreambuf_iterator<char>()));
		bin_file.close();

		// Create from file data.
		load_from_memory(v_file_data.data(), v_file_data.size());
		return true;
	}

	// Can't open.
	else
	{
		return false;
	}
} // End load_from_file.

void MyGifData::load_from_memory(const char* data, int data_size)
{
	destroy();

	// Initialize implementation.
	{
		m_implementation = new WuffsGifImplementation;
		if (m_implementation == nullptr)
		{
			std::cout << "Can't allocate memory for gif wuffs data.\n";
			std::terminate();
		}

		m_implementation->init_decoder(true);
		m_implementation->set_file_buffer(data, data_size);
	} // End initialize.

	// Create buffers.
	{
		// Get size.
		v_gif_size = m_implementation->determine_size();
		int pixel_byte_count = v_gif_size.x * v_gif_size.y * 4;
		m_triple_pixel_buffer = new unsigned char[3 * pixel_byte_count];
		if (m_triple_pixel_buffer == nullptr)
		{
			std::cout << "Can't allocate memory for gif pixel datas.\n";
			std::terminate();
		}

		// Create workbuffer.
		{
			v_work_buffer_size = m_implementation->get_work_buffer_size();

			// See if required.
			if (v_work_buffer_size > 0)
			{
				m_work_buffer = new unsigned char[v_work_buffer_size];
				if (m_work_buffer == nullptr)
				{
					std::cout << "Can't allocate memory for gif work buffer.\n";
					std::terminate();
				} // End can't alloc.
			} // End setup implementation workd buffer.

			// Set work buffer. Data may or may not exist.
			m_implementation->set_work_buffer(m_work_buffer, v_work_buffer_size);

		} // End work buffer.

		// Setup buffer ptrs.
		{
			v_buffer_ptrs.v_size = v_gif_size;

			// Setup buffers.
			v_buffer_ptrs.p_current_buffer = m_triple_pixel_buffer;
			v_buffer_ptrs.p_previous_buffer = m_triple_pixel_buffer + pixel_byte_count;
			v_buffer_ptrs.p_result_buffer = m_triple_pixel_buffer + 2 * pixel_byte_count;

			// Set buffer for pixels.
			m_implementation->set_pixel_buffer(v_buffer_ptrs.p_current_buffer, pixel_byte_count);
		} // End setup buffer ptrs.

	} // End create buffers.

	// Setup initial values. Buffers at -1 now.
	{
		m_implementation->get_frame_headers(v_frame_headers);
		v_frame_count = (int)v_frame_headers.size();
		v_result_index = -1;
		v_loop_count = m_implementation->get_loop_count();
	} // End setup_initial_values.

} // End load_from_memory.

void MyGifData::destroy()
{
	if (m_implementation)
	{
		delete m_implementation;
		m_implementation = nullptr;
	} // End if implementation exists.

	if (m_work_buffer)
	{
		delete [] m_work_buffer;
		m_work_buffer = nullptr;
		v_work_buffer_size = 0;
	} // End if work buffer.

	if (m_triple_pixel_buffer)
	{
		delete[] m_triple_pixel_buffer;
		m_triple_pixel_buffer = nullptr;
	}


	v_frame_headers.clear();
	v_gif_size = MyInt2::ZERO;
	v_frame_count = 0;
} // End destroy.

MyInt2 MyGifData::get_gif_size() const
{
	return v_gif_size;
} // End get_gif_size.

int MyGifData::get_frame_count() const
{
	return v_frame_count;
} // End get_frame_count.

int MyGifData::get_loop_count() const
{
	return v_loop_count;
} // End get_loop_count.

unsigned char* MyGifData::get_frame_data(int frame_index)
{
	// See that legit.
	assert(frame_index >= 0 && frame_index < v_frame_count);

	// See if current is previous. Get next.
	if (v_result_index >= 0 && frame_index - 1 == v_result_index)
	{
		// Get next.
		m_implementation->decode_frame(v_buffer_ptrs, false);
		v_result_index++;

	} // End if current previous.

	// Recreate data from requested frame, unless current result is requested.
	else if (frame_index != v_result_index)
	{
		MyGifFrameHeader* header = &v_frame_headers[frame_index];
		m_implementation->set_buffer_position(frame_index, header->v_io_position);
		m_implementation->decode_frame(v_buffer_ptrs, true);
		v_result_index = frame_index;
	} // End gotta restart.

	// else no changes made. Requested data is already decoded.

	// Get result data.
	return v_buffer_ptrs.p_result_buffer;
} // End get_frame_data.

int MyGifData::get_show_time(int frame_index) const
{
	return v_frame_headers[frame_index].v_show_time;
} // End get_show_time.

int64_t MyGifData::get_flicks(int frame_index) const
{
	return v_frame_headers[frame_index].v_flicks;
} // End get_flicks.

// END GIF DATA.
//===========================================================================
