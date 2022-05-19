#pragma once
#include "util.hpp"
#include "bitmap.hpp"
#include "font_face.hpp"
#include <string>

namespace tou
{
	class bitmap_string
	{
	public:
		bitmap_string();
		bitmap_string(const std::wstring& str, float pointsize, tou::font_face& face);
		~bitmap_string() = default;

		const tou::bitmap_image& get_bitmap() const { return m_bitmap; }
		const std::wstring& get_string() const { return m_str; }
		float get_pointsize() const { return m_ptsize; }

	private:
		void m_combine_bitmap_glyphs(const std::vector<tou::font_face::bitmap_glyph>& glyphs);

	private:
		std::wstring m_str;
		float m_ptsize;
		tou::bitmap_image m_bitmap;
	};
}