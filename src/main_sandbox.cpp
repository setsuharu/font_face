#include <iostream>
#include <fstream>
#include <string>
#include <array>

#include "font_face.hpp"
#include "bitmap.hpp"
#include "texture_atlas.hpp"

void makefile(const std::string& filename, const tou::bitmap_image& bmp)
{
	std::cout << "Writing bitmap " << filename << " to disk..." << std::endl;
	std::vector<char> v;
	bmp.file(v);
	std::ofstream out;
	out.open(filename, std::ios::binary | std::ios::out);
	out.write(v.data(), v.size());
	out.close();
}

int main(int argc, char** argv)
{
	int fileiteration = 2;
	float pointsize = 12.0f;

	tou::font_face face("GrisaiaCustom.ttf");
	if (!face)
		return -1;

	//uint16_t ch = 0x30E2;
	//std::string filename  = "glyph" + std::to_string(ch) + "_@" + std::to_string(static_cast<int>(pointsize)) + "pt_" + std::to_string(fileiteration) + "full.bmp";
	//std::string filenameo = "glyph" + std::to_string(ch) + "_@" + std::to_string(static_cast<int>(pointsize)) + "pt_" + std::to_string(fileiteration) + "outline.bmp";
	//auto glyph = face.get_glyph_bitmap(ch, pointsize, true, true);
	//auto glypho = face.get_glyph_bitmap(ch, pointsize, true, false);
	//makefile(filename, glyph.image);
	//makefile(filenameo, glypho.image);

	std::string atlasname = "atlas" + std::to_string(fileiteration) + ".bmp";
	tou::texture_atlas<uint16_t> atlas(1024, 1024, { 0xFF, 0xFF, 0xFF, 0xFF });
	for (uint16_t ch = 0x0021; ch <= 0x007E; ch++)
	{
		auto glyph = face.get_glyph_bitmap(ch, pointsize, true, true);
		if (glyph.id != 0)
			atlas.push(glyph.image, ch);
	}
	pointsize = 12.0f;
	for (uint16_t ch = 0x30A0; ch <= 0x30FF; ch++)
	{
		auto glyph = face.get_glyph_bitmap(ch, pointsize, true, true);
		if (glyph.id != 0)
			atlas.push(glyph.image, ch);
	}
	for (uint16_t ch = 0x3040; ch <= 0x309F; ch++)
	{
		auto glyph = face.get_glyph_bitmap(ch, pointsize, true, true);
		if (glyph.id != 0)
			atlas.push(glyph.image, ch);
	}
	for (uint16_t ch = 0x4E00; ch <= 0x4F00; ch++)
	{
		auto glyph = face.get_glyph_bitmap(ch, pointsize, true, true);
		if (glyph.id != 0)
			if (!atlas.push(glyph.image, ch))
				break;
	}
	makefile(atlasname, atlas.get());

	return 0;
}