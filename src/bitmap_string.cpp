#include "bitmap_string.hpp"

namespace tou
{
	bitmap_string::bitmap_string()
	{
	}

	bitmap_string::bitmap_string(const std::wstring& str, float pointsize, tou::font_face& face)
		:m_str(str), m_ptsize(pointsize)
	{
		std::vector<tou::font_face::bitmap_glyph> glyphs;
		// To do: to make it more efficient, detect identical chars in a string to avoid creating the same glyph twice
		for (wchar_t wch : str)
		{
			auto glyph = face.get_glyph_bitmap(static_cast<uint16_t>(wch), pointsize, true, true);
			glyphs.push_back(glyph);
		}

		// To do: in regaeds to above To do, code (function) for combining the bitmaps will need to be rewritten
		m_combine_bitmap_glyphs(glyphs);
	}

	void bitmap_string::m_combine_bitmap_glyphs(const std::vector<tou::font_face::bitmap_glyph>& glyphs)
	{
		uint32_t pxwidth = 0;
		uint32_t pxmaxheight = 0;
		for (const tou::font_face::bitmap_glyph& glyph : glyphs)
		{
			pxwidth += glyph.image.width();
			if (pxmaxheight < glyph.image.height())
				pxmaxheight = glyph.image.height();
		}

		tou::bitmap_image image(pxwidth, pxmaxheight);
		uint32_t image_pen_x_traveled = 0;
		uint32_t image_pen_x = 0;
		uint32_t image_pen_y = 0;
		for (const tou::font_face::bitmap_glyph& glyph : glyphs)
		{
			// get width of the glyph, advance 'pen' to next start position (lower left corner pixel)
			// start drawing from there for next glyph
			uint32_t xpos = 0;
			uint32_t ypos = 0;
			uint32_t glyph_width = glyph.image.width();
			for (uint32_t i = 0; i < glyph.image.pixel_count(); i++)
			{
				if (xpos >= glyph_width)
				{
					ypos++;
					xpos = 0;
					image_pen_y++;
					image_pen_x = image_pen_x_traveled;
				}
				image[{image_pen_x, image_pen_y}] = glyph.image[{xpos, ypos}];
				image_pen_x++;
				xpos++;
			}
			image_pen_x_traveled += glyph_width;
			image_pen_y = 0;
		}
		m_bitmap = image;
	}
}