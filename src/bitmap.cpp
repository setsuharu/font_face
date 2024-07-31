#include "bitmap.hpp"
#include <algorithm>

#define V_PUSH_BACK(x, v) for(char& b : split_bytes(x)) v.push_back(b)

namespace tou
{
	bitmap_image::bitmap_image()
		:m_width(0), m_height(0)
	{
	}

	bitmap_image::bitmap_image(uint32_t bitmap_width, uint32_t bitmap_height, const bitmap::argb32& default_color)
		:m_width(bitmap_width), m_height(bitmap_height)
	{
		resize(bitmap_width, bitmap_height, default_color);
	}

	void bitmap_image::resize(uint32_t bitmap_width, uint32_t bitmap_height, const bitmap::argb32& default_color)
	{
		m_width = bitmap_width;
		m_height = bitmap_height;

		uint32_t n = bitmap_width * bitmap_height;

		if (!m_pixels.empty())
			m_pixels.clear();

		m_pixels.reserve(n);
		m_pixels.resize(n, default_color);
	}

	void bitmap_image::crop(uint32_t from_right, uint32_t from_left, uint32_t from_top, uint32_t from_bottom)
	{
		int check_height =	(static_cast<int>(m_height) - static_cast<int>(from_top) - static_cast<int>(from_bottom));
		int check_width =	(static_cast<int>(m_width) - static_cast<int>(from_left) - static_cast<int>(from_right));

		if (check_height < 0 || check_width < 0)
			return; // temp

		if (from_right)
		{
			uint32_t new_width = m_width - from_right;
			tou::ivec2 start_pos = { new_width, 0 };
			tou::ivec2 end_pos = { m_width, m_height };	// m_width, m_height are not valid positions since the container starts at 0
			uint32_t n = from_right * m_height;	// n elements will be deleted
			m_delete_region(start_pos, end_pos, n);
			m_width = new_width;
		}
		if (from_left)
		{
			uint32_t new_width = m_width - from_left;
			tou::ivec2 start_pos = { 0, 0 };
			tou::ivec2 end_pos = { from_left, m_height };
			uint32_t n = from_left * m_height;
			m_delete_region(start_pos, end_pos, n);
			m_width = new_width;
		}
		if (from_top)
		{
			uint32_t new_height = m_height - from_top;
			tou::ivec2 start_pos = { 0, new_height };
			tou::ivec2 end_pos = { m_width, m_height };
			uint32_t n = m_width * from_top;
			m_delete_region(start_pos, end_pos, n);
			m_height = new_height;
		}
		if (from_bottom)
		{
			uint32_t new_height = m_height - from_bottom;
			tou::ivec2 start_pos = { 0, 0 };
			tou::ivec2 end_pos = { m_width, from_bottom };
			uint32_t n = m_width * from_bottom;
			m_delete_region(start_pos, end_pos, n);
			m_height = new_height;
		}
	}

	void bitmap_image::extrude(uint32_t from_right, uint32_t from_left, uint32_t from_top, uint32_t from_bottom, const bitmap::argb32& default_color)
	{
		if (from_right)
		{
			uint32_t new_width = m_width + from_right;
			uint32_t amount_of_new_elements = from_right * m_height;
			std::vector<bitmap::argb32> new_pixels(m_pixels.size() + amount_of_new_elements, default_color);
			tou::ivec2 pos = { 0, 0 };
			tou::ivec2 original_end = { m_width, m_height };
			for (uint32_t i = 0; i < m_pixels.size(); i++)
			{
				if (pos.x >= original_end.x)
				{
					pos.x = 0;
					pos.y++;
				}
				new_pixels[static_cast<size_t>(m_get_nth_element_index(pos, new_width))] = m_pixels[m_get_nth_element_index(pos, m_width)];
				pos.x++;
			}
			m_pixels = new_pixels;
			m_width = new_width;
		}
		if (from_top)
		{
			uint32_t new_height = m_height + from_top;
			uint32_t amount_of_new_elements = m_width * from_top;
			m_pixels.resize(m_pixels.size() + amount_of_new_elements, default_color);
			m_height = new_height;
		}
		if (from_left)
		{
			uint32_t new_width = from_left + m_width;
			uint32_t amount_of_new_elements = from_left * m_height;
			std::vector<bitmap::argb32> new_pixels(m_pixels.size() + amount_of_new_elements, default_color);
			tou::ivec2 pos = { from_left, 0 };
			tou::ivec2 original_end = { new_width, m_height };
			for (uint32_t i = 0; i < m_pixels.size(); i++)
			{
				if (pos.x >= original_end.x)
				{
					pos.x = from_left;
					pos.y++;
				}
				new_pixels[static_cast<size_t>(m_get_nth_element_index(pos, new_width))] = m_pixels[m_get_nth_element_index({pos.x - from_left, pos.y}, m_width)];
				pos.x++;
			}
			m_pixels = new_pixels;
			m_width = new_width;
		}
		if (from_bottom)
		{
			uint32_t new_height = from_bottom + m_height;
			uint32_t amount_of_new_elements = m_width * from_bottom;
			std::vector<bitmap::argb32> new_pixels(m_pixels.size() + amount_of_new_elements, default_color);
			tou::ivec2 pos = { 0, from_bottom };
			uint32_t i = m_get_nth_element_index(pos, m_width);
			for (uint32_t j = 0; j < m_pixels.size(); j++)
			{
				new_pixels[i] = m_pixels[j];
				i++;
			}
			m_pixels = new_pixels;
			m_height = new_height;
		}
	}

	void bitmap_image::translate_region(const tou::ivec2& bottom_left, const tou::ivec2& top_right, uint32_t right, uint32_t left, uint32_t up, uint32_t down)
	{
		uint32_t h = top_right.y - bottom_left.y;
		uint32_t w = top_right.x - bottom_left.x;
		if (h > m_height || w > m_width || top_right.x > m_width || top_right.y > m_height)
		{
			debug_break("region cannot be greater than container size");
			return;
		}
		if (right)
		{
			if (top_right.x > m_width || top_right.x + right > m_width)
			{
				debug_break("cannot translate past container bounds"); // maybe include functionality to extrude first then translate
				return;
			}
			uint32_t n = w * h;
			tou::ivec2 pos = { top_right.x - 1, top_right.y - 1 };
			tou::ivec2 end_pos = bottom_left;
			for (uint32_t i = 0; i < n; i++)
			{
				if (pos.x < end_pos.x)
				{
					pos.x = top_right.x - 1;
					pos.y--;
				}
				m_pixels[m_get_nth_element_index({ pos.x + right, pos.y }, m_width)] = m_pixels[m_get_nth_element_index(pos, m_width)];
				m_pixels[m_get_nth_element_index(pos, m_width)] = { 0xFF, 0xFF, 0xFF, 0xFF };
				pos.x--;
			}
		}
		if (left)
		{
			if (static_cast<int>(bottom_left.x) - static_cast<int>(left) < 0)
			{
				debug_break("cannot translate past container bounds");
				return;
			}
			uint32_t n = w * h;
			tou::ivec2 pos = bottom_left;
			tou::ivec2 end_pos = top_right;
			for (uint32_t i = 0; i < n; i++)
			{
				if (pos.x == end_pos.x)
				{
					pos.x = bottom_left.x;
					pos.y++;
				}
				m_pixels[m_get_nth_element_index({ pos.x - left, pos.y }, m_width)] = m_pixels[m_get_nth_element_index(pos, m_width)];
				m_pixels[m_get_nth_element_index(pos, m_width)] = { 0xFF, 0xFF, 0xFF, 0xFF };
				pos.x++;
			}
		}
		if (up)
		{
			if (top_right.y > m_height || top_right.y + up > m_height)
			{
				debug_break("cannot translate past container bounds");
				return;
			}
			uint32_t n = w * h;
			tou::ivec2 pos = { top_right.x - 1, top_right.y - 1 };
			tou::ivec2 end_pos = bottom_left;
			for (uint32_t i = 0; i < n; i++)
			{
				if (pos.x < end_pos.x)
				{
					pos.x = top_right.x - 1;
					pos.y--;
				}
				m_pixels[m_get_nth_element_index({ pos.x, pos.y + up }, m_width)] = m_pixels[m_get_nth_element_index(pos, m_width)];
				m_pixels[m_get_nth_element_index(pos, m_width)] = { 0xFF, 0xFF, 0xFF, 0xFF };
				pos.x--;
			}
		}
		if (down)
		{
			if (static_cast<int>(bottom_left.y) - static_cast<int>(down) < 0)
			{
				debug_break("cannot translate past container bounds");
				return;
			}
			uint32_t n = w * h;
			tou::ivec2 pos = bottom_left;
			tou::ivec2 end_pos = top_right;
			for (uint32_t i = 0; i < n; i++)
			{
				if (pos.x == end_pos.x)
				{
					pos.x = bottom_left.x;
					pos.y++;
				}
				m_pixels[m_get_nth_element_index({ pos.x, pos.y - down }, m_width)] = m_pixels[m_get_nth_element_index(pos, m_width)];
				m_pixels[m_get_nth_element_index(pos, m_width)] = { 0xFF, 0xFF, 0xFF, 0xFF };
				pos.x++;
			}
		}
	}

	bool bitmap_image::swap_regions(uint32_t subwidth, uint32_t subheight, tou::ivec2 region1_origin, tou::ivec2 region2_origin)
	{
		if (region1_origin.x + subwidth >= m_width || region1_origin.y + subheight >= m_height || region2_origin.x + subwidth >= m_width || region2_origin.y + subheight >= m_height)
			return false;

		if (region1_origin.x > region2_origin.x && region1_origin.y > region2_origin.y)
		{
			// swap their places
			tou::ivec2 temp = region1_origin;
			region1_origin = region2_origin;
			region2_origin = temp;
		}

		tou::ivec2 region1_position = region1_origin, region1_topright = { region1_origin.x + subwidth, region1_origin.y + subheight };
		tou::ivec2 region2_position = region2_origin, region2_topright = { region2_origin.x + subwidth, region2_origin.y + subheight };

		if ((region1_topright.x > region2_origin.x && region1_topright.y > region2_origin.y))
			return false;	// overlapping regions, maybe include functionality to ignore the overlapping regions

		uint32_t n = subwidth * subheight;
		for (uint32_t i = 0; i < n; i++)
		{
			if (region1_position.x == region1_topright.x)
			{
				region1_position.x = region1_origin.x;
				region1_position.y++;
				region2_position.x = region2_origin.x;
				region2_position.y++;
			}
			bitmap::argb32 temp = operator[](region2_position);
			operator[](region2_position) = operator[](region1_position);
			operator[](region1_position) = temp;
			region1_position.x++;
			region2_position.x++;
		}
		return true;
	}

	void bitmap_image::file(std::vector<char>& v) const
	{
		// prepare the header and dib
		v.reserve(total_size());

		bitmap::header h{ 0 };
		h.id = 0x424D;
		h.filesize = little_endian(static_cast<uint32_t>(14 + 108 + (m_pixels.size() * 4)));
		h.app1 = 0x0000;
		h.app2 = 0x0000;
		h.data_offset = little_endian(static_cast<uint32_t>(14 + 108));

		bitmap::dib_bitmap_v4_header dib;
		dib.hsize = 0x6C000000;
		dib.bmp_width = static_cast<int32_t>(little_endian( static_cast<uint32_t>(m_width)));
		dib.bmp_height = static_cast<int32_t>(little_endian(static_cast<uint32_t>(m_height)));
		dib.num_color_planes = 0x0100;
		dib.bpp = 0x2000;		// 32 bpp
		dib.compression = 0x03000000;
		dib.datasize = little_endian(static_cast<uint32_t>(m_pixels.size() * 4));	// 4 bytes per pixel
		dib.hres = 0x130B0000;	// 72 dpi * 39.3701 = 2835
		dib.vres = 0x130B0000;
		dib.num_palette_colors = 0x00000000;
		dib.num_important_colors = 0x00000000;
		dib.red_channel_bitmask = 0x0000FF00;
		dib.green_channel_bitmask = 0x00FF0000;
		dib.blue_channel_bitmask = 0xFF000000;
		dib.alpha_channel_bitmask = 0x000000FF;
		dib.lcs_windows_colorspace = 0x206E6957;
		dib.red_gamma = 0x00000000;
		dib.green_gamma = 0x00000000;
		dib.blue_gamma = 0x00000000;

		m_push_struct(h, v);
		m_push_struct(dib, v);

		for (const auto& x : m_pixels)
			m_push_struct(x, v);
	}

	void bitmap_image::insert_other_bitmap_at_coordinate(const tou::bitmap_image& bitmap, const tou::ivec2& coordinate)
	{
		// take coordinate as the bottom-left of region
		// take width and height of given bitmap
		// iterate through pixels and copy to 'this' bitmap
		uint32_t otherwidth = bitmap.width();
		uint32_t otherheight = bitmap.height();
		tou::ivec2 pen = coordinate;
		tou::ivec2 otherpen = { 0, 0 };

		if ((pen.x + otherwidth >= m_width) || (pen.y + otherheight >= m_height))
		{
			// for now, we return
			// To Do: extrude 'this' to make room for new data
			LOG("Unable to insert new bitmap data into existing bitmap");
			return;
		}
		for (uint32_t i = 0; i < bitmap.pixel_count(); i++)
		{
			if (otherpen.x >= otherwidth)
			{
				otherpen.x = 0;
				otherpen.y++;
				pen.x = coordinate.x;
				pen.y++;
			}
			m_pixels[m_get_nth_element_index(pen, m_width)] = bitmap[otherpen];
			pen.x++;
			otherpen.x++;
		}
	}

	tou::ivec2 bitmap_image::m_get_nth_element_coordinate(uint32_t i, uint32_t width) const
	{
		int y = static_cast<int>(i / width);
		if (y == 0)
			return { i, static_cast<uint32_t>(y) };
		int x = ((width - 1) * y + y - i) * -1;
		return { static_cast<uint32_t>(x), static_cast<uint32_t>(y) };
	}

	void bitmap_image::m_delete_region(const tou::ivec2& start, const tou::ivec2& end, uint32_t n)
	{
		tou::ivec2 pos = start;
		std::vector<bitmap::argb32> new_pixels;
		std::vector<uint32_t> indexes;
		new_pixels.reserve(m_pixels.size() - n);
		indexes.reserve(n);
		for (uint32_t i = 0; i < n; i++)
		{
			if (pos.x == end.x)
			{
				pos.x = start.x;
				pos.y++;
			}
			indexes.push_back(m_get_nth_element_index(pos, m_width));
			pos.x++;
		}
		for (uint32_t i = 0; i < m_pixels.size(); i++)
		{
			if (!(std::find(indexes.begin(), indexes.end(), i) != indexes.end()))
				new_pixels.push_back(m_pixels[i]);
		}
		m_pixels = new_pixels;
	}

	void bitmap_image::m_push_struct(const bitmap::header& h, std::vector<char>& v) const
	{
		V_PUSH_BACK(h.id, v);
		V_PUSH_BACK(h.filesize, v);
		V_PUSH_BACK(h.app1, v);
		V_PUSH_BACK(h.app2, v);
		V_PUSH_BACK(h.data_offset, v);
	}

	void bitmap_image::m_push_struct(const bitmap::dib_bitmap_v4_header& d, std::vector<char>& v) const
	{
		V_PUSH_BACK(d.hsize, v);
		V_PUSH_BACK(d.bmp_width, v);
		V_PUSH_BACK(d.bmp_height, v);
		V_PUSH_BACK(d.num_color_planes, v);
		V_PUSH_BACK(d.bpp, v);
		V_PUSH_BACK(d.compression, v);
		V_PUSH_BACK(d.datasize, v);
		V_PUSH_BACK(d.hres, v);
		V_PUSH_BACK(d.vres, v);
		V_PUSH_BACK(d.num_palette_colors, v);
		V_PUSH_BACK(d.num_important_colors, v);

		V_PUSH_BACK(d.red_channel_bitmask, v);
		V_PUSH_BACK(d.green_channel_bitmask, v);
		V_PUSH_BACK(d.blue_channel_bitmask, v);
		V_PUSH_BACK(d.alpha_channel_bitmask, v);
		V_PUSH_BACK(d.lcs_windows_colorspace, v);
		V_PUSH_BACK(d.colorspace_endpoints.colorspace_xyz_block1, v);
		V_PUSH_BACK(d.colorspace_endpoints.colorspace_xyz_block2, v);
		V_PUSH_BACK(d.colorspace_endpoints.colorspace_xyz_block3, v);
		V_PUSH_BACK(d.colorspace_endpoints.colorspace_xyz_block4, v);
		V_PUSH_BACK(d.colorspace_endpoints.colorspace_xyz_block5, v);
		V_PUSH_BACK(d.colorspace_endpoints.colorspace_xyz_block6, v);
		V_PUSH_BACK(d.colorspace_endpoints.colorspace_xyz_block7, v);
		V_PUSH_BACK(d.colorspace_endpoints.colorspace_xyz_block8, v);
		V_PUSH_BACK(d.colorspace_endpoints.colorspace_xyz_block9, v);
		V_PUSH_BACK(d.red_gamma, v);
		V_PUSH_BACK(d.green_gamma, v);
		V_PUSH_BACK(d.blue_gamma, v);
	}

	void bitmap_image::m_push_struct(const bitmap::argb32& rgba, std::vector<char>& v) const
	{
		v.push_back(static_cast<char>(rgba.b));
		v.push_back(static_cast<char>(rgba.g));
		v.push_back(static_cast<char>(rgba.r));
		v.push_back(static_cast<char>(rgba.a));
	}
}