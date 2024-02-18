#pragma once
#include <cinttypes>
#include <vector>
#include <string>
#include <array>
class WuffsGifImplementation; // Found in cpp-file.

//================================================================================
class MyInt2
{
public:

	static MyInt2 ZERO;

	int x, y;

	MyInt2() : x(0), y(0) {}
	MyInt2(int ix, int iy) : x(ix), y(iy) {}
	explicit MyInt2(int val) : x(val), y(val) {}

	bool operator==(const MyInt2& other) const;

};

//================================================================================
class MyGifFrameHeader
{
public:
	int64_t v_flicks;
	int v_show_time; // Frame count when 60fps.
	int v_io_position;

}; // End MyGifFrameHeader.


//================================================================================
class MyGifPixelBuffer
{
public:

	MyGifPixelBuffer();

	int get_byte_size() const;

	// Swap current and previous.
	void swap_buffers();

	// Writes previous same as current buffer.
	void set_previous();

	// Writes result from current. If v_flip_y true, creates the result image upside down (for OpenGL usage possibly).
	void write_result();

	// Writes color to background (rgba).
	void clear_background(std::array<unsigned char,4> color);
	void write_background(const MyInt2& top_left, const MyInt2& bottom_right, std::array<unsigned char,4> color);

	// Data. Pointers only. Data exists elsewhere.
	unsigned char* p_current_buffer;
	unsigned char* p_previous_buffer;
	unsigned char* p_result_buffer;
	MyInt2 v_size;
	bool v_flip_y;

}; // End MyGifPixelBuffer.

//================================================================================

class MyGifData
{
public:

	MyGifData();
	~MyGifData();

	//=============================================================
	// Load from file. Return true if good, false if fails. Creates it's own memory.
	bool load_from_file(const std::string& file_name);

	// Create directly from memory. WARNING: memory must exist entire time this gif-data is used.
	void load_from_memory(const char* data, int data_size);
	
	// Destroy all allocations.
	void destroy();

	//=============================================================
	// Get data.
	MyInt2 get_gif_size() const;

	// Get frame count.
	int get_frame_count() const;

	// Loop count hard-coded to gif-file.
	int get_loop_count() const;

	//=============================================================
	// Get frame data in rgba. Can be called in arbitrary order if gif frame doesn't depend on previous one.
	unsigned char* get_frame_data(int frame_index);

	// Get frame time in 60fps (1/60 seconds).
	int get_show_time(int frame_index) const;

	// Get frame time as flicks. https://github.com/facebookarchive/Flicks
	int64_t get_flicks(int frame_index) const;

	//=============================================================

private:

	// Optional filedata if loading directly from file.
	std::vector<char> v_file_data;

	// Wuffs data stored here.
	WuffsGifImplementation* m_implementation;

	// Buffers. nullptr if not defined.
	unsigned char* m_triple_pixel_buffer;
	unsigned char* m_work_buffer;

	// Current ones.
	MyGifPixelBuffer v_buffer_ptrs;

	// Frame data. Lenght of play and position in file data stored here.
	std::vector<MyGifFrameHeader> v_frame_headers;

	// Size.
	MyInt2 v_gif_size;
	int v_loop_count;
	int v_work_buffer_size;

	// Total number of different images gif contains.
	int v_frame_count;

	// Current image index. -1 if not defined.
	int v_result_index;

}; // End MyGifData.

