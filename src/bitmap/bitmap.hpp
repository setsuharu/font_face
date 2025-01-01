#pragma once
#include "util.hpp"

namespace tou
{
	namespace bitmap
	{
		// little endian format
		struct header
		{
			uint16_t id = 0;				// usually 'BM' in ASCII hex code
			uint32_t filesize = 0;			// size of the BMP file in bytes
			uint16_t app1 = 0, app2 = 0;	// application specific, if created manually can be 0
			uint32_t data_offset = 0;		// offset from the beginning of the file in which the pixel data can be found
		};
		struct dib_bitmap_info_header
		{
			// the type of DIB header used for this case is 'BITMAPINFOHEADER'
			uint32_t	hsize = 0;					// size of the header in bytes (40 bytes for BITMAPINFOHEADER)
			int32_t		bmp_width = 0;				// width of bitmap in pixels (signed integer)
			int32_t		bmp_height = 0;				// height of bitmap in pixels (signed integer)
			uint16_t	num_color_planes = 0;		// (must be 1)
			uint16_t	bpp = 0;					// bits per pixel (bpp) typical values are: 1, 4, 8, 16, 24, 32
			uint32_t	compression = 0;			// compression method being used (use value: 0 (BI_RGB) for no compression)
			uint32_t	datasize = 0;				// size of just the raw bitmap data (a dummy 0 can be given for BI_RGB bitmaps)
			int32_t		hres = 0;					// horizontal resolution of the image (pixel per metre, signed int)
			int32_t		vres = 0;					// vertical resolution of the image
			uint32_t	num_palette_colors = 0;		// number of colors in palette (use 0 to default to 2^n)
			uint32_t	num_important_colors = 0;	// usually ignored (use 0 to denote every color as important)
		};
		struct cie_xyz_triple
		{
			// totals 36 bytes (set all to zero to ignore)
			uint32_t colorspace_xyz_block1 = 0x00000000;
			uint32_t colorspace_xyz_block2 = 0x00000000;
			uint32_t colorspace_xyz_block3 = 0x00000000;
			uint32_t colorspace_xyz_block4 = 0x00000000;
			uint32_t colorspace_xyz_block5 = 0x00000000;
			uint32_t colorspace_xyz_block6 = 0x00000000;
			uint32_t colorspace_xyz_block7 = 0x00000000;
			uint32_t colorspace_xyz_block8 = 0x00000000;
			uint32_t colorspace_xyz_block9 = 0x00000000;
		};
		struct dib_bitmap_v4_header
		{
			uint32_t	hsize = 0;
			int32_t		bmp_width = 0;
			int32_t		bmp_height = 0;
			uint16_t	num_color_planes = 0;
			uint16_t	bpp = 0;
			uint32_t	compression = 0;
			uint32_t	datasize = 0;
			int32_t		hres = 0;
			int32_t		vres = 0;
			uint32_t	num_palette_colors = 0;
			uint32_t	num_important_colors = 0;
			// the above is the same as BITMAPINFOHEADER

			uint32_t red_channel_bitmask = 0;
			uint32_t green_channel_bitmask = 0;
			uint32_t blue_channel_bitmask = 0;
			uint32_t alpha_channel_bitmask = 0;
			uint32_t lcs_windows_colorspace = 0;	// default 0x206E6957 (little-endian for "Win ")
			cie_xyz_triple colorspace_endpoints;	// 36 bytes in size (unused for LCS "Win " or "sRGB")
			uint32_t red_gamma = 0;					// unused for LCS "Win " or "sRGB"
			uint32_t green_gamma = 0;				// unused for LCS "Win " or "sRGB"
			uint32_t blue_gamma = 0;				// unused for LCS "Win " or "sRGB"
		};
		struct argb32
		{
			// pads not needed when each pixel is 4 bytes each
			// the order is bgra
			uint8_t b = 0, g = 0, r = 0, a = 0;

			bool operator==(const argb32& rhs)
			{
				return ((this->r == rhs.r) && (this->g == rhs.g) && (this->b == rhs.b) && (this->a == rhs.a));
			}
		};

		struct region
		{
			tou::ivec2 bottom_left{ 0, 0 };
			tou::ivec2 top_right{ 0, 0 }; // non-inclusive
			uint32_t width() const { return top_right.x - bottom_left.x; }
			uint32_t height() const { return top_right.y - bottom_left.y; }
		};
	}

	/* temporary */
	enum class image_type
	{
		bmp, jpg, png
	};

	class bitmap_image
	{
	public:
		bitmap_image();
		bitmap_image(uint32_t bitmap_width, uint32_t bitmap_height, const bitmap::argb32& default_color = { 0xFF, 0xFF, 0xFF, 0xFF });
		~bitmap_image() = default;

		// resize the bitmap - will overwrite data already present in the container
		void resize(uint32_t bitmap_width, uint32_t bitmap_height, const bitmap::argb32& default_color = { 0xFF, 0xFF, 0xFF, 0xFF });

		// the manipulation functions are not the best performing and can have O(n^4) run time in worst case scenarios
		void crop(uint32_t from_left, uint32_t from_right, uint32_t from_top, uint32_t from_bottom);
		void extrude(uint32_t from_right, uint32_t from_left, uint32_t from_top, uint32_t from_bottom, const bitmap::argb32& default_color = { 0xFF, 0xFF, 0xFF, 0xFF });
		void translate_region(const tou::ivec2& bottom_left, const tou::ivec2& top_right, uint32_t right, uint32_t left, uint32_t up, uint32_t down); // region.top_right is non-inclusive
		bool swap_regions(uint32_t subwidth, uint32_t subheight, tou::ivec2 region1_origin, tou::ivec2 region2_origin);

		// fills given vector with data composing a complete bitmap file
		void file(std::vector<char>& v) const;

		void insert_other_bitmap_at_coordinate(const tou::bitmap_image& bitmap, const tou::ivec2& coordinate);

		bitmap::argb32& operator[](const tou::ivec2& coordinate) { return m_pixels[m_get_nth_element_index(coordinate, m_width)]; }
		const bitmap::argb32& operator[](const tou::ivec2& coordinate) const { return m_pixels[m_get_nth_element_index(coordinate, m_width)]; }
		bitmap::argb32& operator[](uint32_t x) { return m_pixels[x]; }
		const bitmap::argb32& operator[](uint32_t x) const { return m_pixels[x]; }
		
		std::vector<bitmap::argb32>::iterator begin() { return m_pixels.begin(); }
		std::vector<bitmap::argb32>::iterator end() { return m_pixels.end(); }
		std::vector<bitmap::argb32>::const_iterator cend() const { return m_pixels.cend(); }
		std::vector<bitmap::argb32>::const_iterator cbegin() const { return m_pixels.cbegin(); }
		
		uint32_t pixel_count() const { return static_cast<uint32_t>(m_pixels.size()); }
		uint32_t raw_size() const { return static_cast<uint32_t>(m_pixels.size()) * 4; }
		uint32_t total_size() const { return 122 + (raw_size()); }
		uint32_t width() const { return m_width; }
		uint32_t height() const { return m_height; }

	private:
		uint32_t m_get_nth_element_index(const tou::ivec2& c, uint32_t width) const { return (((width - 1) * c.y) + (c.x + c.y)); }
		tou::ivec2 m_get_nth_element_coordinate(uint32_t i, uint32_t width) const;
		void m_delete_region(const tou::ivec2& start, const tou::ivec2& end, uint32_t n);
		void m_push_struct(const bitmap::header& h, std::vector<char>& v) const;
		void m_push_struct(const bitmap::dib_bitmap_v4_header& d, std::vector<char>& v) const;
		void m_push_struct(const bitmap::argb32& rgba, std::vector<char>& v) const;

	private:
		uint32_t m_width;
		uint32_t m_height;
		std::vector<bitmap::argb32> m_pixels;
	};
}