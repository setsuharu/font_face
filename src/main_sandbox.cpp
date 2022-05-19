#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <string>
#include <array>
#include <memory>

#include "font_face.hpp"
#include "bitmap.hpp"
#include "bitmap_string.hpp"
#include "texture_atlas.hpp"

void makefile(const std::wstring& filename, const tou::bitmap_image& bmp)
{
	std::wcout << L"Writing bitmap " << filename << L" to disk..." << std::endl;
	std::vector<char> v;
	bmp.file(v);
	std::ofstream out;
	out.open(filename, std::ios::binary | std::ios::out);
	out.write(v.data(), v.size());
	out.close();
}

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

std::shared_ptr<tou::bitmap_image> make_glyph_canvas(uint16_t glyphhex, float pointsize, tou::font_face& face)
{
	//uint16_t glyphhex = 0x5931;
	//std::wstringstream ss;
	//ss << L"0x" << std::setfill(L'0') << std::setw(sizeof(uint16_t) * 2) << std::hex << glyphhex;

	auto glyph = face.get_glyph_bitmap(glyphhex, pointsize, true, true);
	tou::bitmap_string glyph_id_string(std::wstring() + std::to_wstring(glyph.id)/* + L"-" + ss.str()*/, 4.0f, face);

	uint32_t glyphwidth = glyph.image.width();
	uint32_t glyphheight = glyph.image.height();
	uint32_t strwidth = glyph_id_string.get_bitmap().width();
	uint32_t strheight = glyph_id_string.get_bitmap().height();

	uint32_t cwidth = (glyphwidth >= strwidth ? glyphwidth : strwidth);
	uint32_t cheight = (glyphheight + strheight);

	tou::bitmap_image canvas(cwidth + 1, cheight + 1);
	canvas.insert_other_bitmap_at_coordinate(glyph.image, { 0, strheight });
	canvas.insert_other_bitmap_at_coordinate(glyph_id_string.get_bitmap(), { 0, 0 });

	return std::make_shared<tou::bitmap_image>(canvas);
}

void render_glyph_to_bitmap_file(uint16_t ch, tou::font_face& face, float pointsize, int fileiteration)
{
	std::string filename = "glyph" + std::to_string(ch) + "_@" + std::to_string(static_cast<int>(pointsize)) + "pt_" + std::to_string(fileiteration) + "full.bmp";
	std::string filenameo = "glyph" + std::to_string(ch) + "_@" + std::to_string(static_cast<int>(pointsize)) + "pt_" + std::to_string(fileiteration) + "outline.bmp";
	auto glyph = face.get_glyph_bitmap(ch, pointsize, true, true);
	auto glypho = face.get_glyph_bitmap(ch, pointsize, true, false);
	makefile(filename, glyph.image);
	makefile(filenameo, glypho.image);
}

void render_font_atlas_bitmap_file(tou::font_face& face, int fileiteration)
{
	float pointsize = 12.0f;
	std::wstring atlasname = L"atlas" + std::to_wstring(fileiteration) + L".bmp";
	tou::texture_atlas<uint16_t> atlas(2048, 2048, { 0xFF, 0xFF, 0xFF, 0xFF });
	for (uint16_t ch = 0x0021; ch <= 0x007E; ch++) // alphabet
	{
		//auto glyph = face.get_glyph_bitmap(ch, pointsize, true, true);
		//if (glyph.id != 0)
		//	atlas.push(glyph.image, ch);
		tou::font_face::truetype_glyph glyph = face.get_glyph(ch);
		if (glyph.id != 0)
		{
			auto canvas = make_glyph_canvas(ch, pointsize, face);
			atlas.push(*canvas.get(), ch);
		}
	}
	for (uint16_t ch = 0x30A0; ch <= 0x30FF; ch++) // katakana
	{
		tou::font_face::truetype_glyph glyph = face.get_glyph(ch);
		if (glyph.id != 0)
		{
			auto canvas = make_glyph_canvas(ch, pointsize, face);
			atlas.push(*canvas.get(), ch);
		}
	}
	for (uint16_t ch = 0x3040; ch <= 0x309F; ch++) // hiragana
	{
		tou::font_face::truetype_glyph glyph = face.get_glyph(ch);
		if (glyph.id != 0)
		{
			auto canvas = make_glyph_canvas(ch, pointsize, face);
			atlas.push(*canvas.get(), ch);
		}
	}
	for (uint16_t ch = 0x4E00; ch <= 0x9FBF; ch++) // kanji (limit previously 0x4F00)
	{
		//auto glyph = face.get_glyph_bitmap(ch, pointsize, true, true);
		//if (glyph.id != 0)
		//	if (!atlas.push(glyph.image, ch))
		//		break;
		tou::font_face::truetype_glyph glyph = face.get_glyph(ch);
		if (glyph.id != 0)
		{
			auto canvas = make_glyph_canvas(ch, pointsize, face);
			//atlas.push(*canvas.get(), ch);
			if (!atlas.push(*canvas.get(), ch))
				break;
		}
	}
	makefile(atlasname, atlas.get());
}

int main(int argc, char** argv)
{
	int fileiteration = 11;
	float pointsize = 64.0f;

	tou::font_face face("GrisaiaCustom.ttf");
	if (!face)
		return -1;

	// auto canvas = make_glyph_canvas(0x5263, 18.0f, face);
	// makefile(L"canvas.bmp", *canvas.get());

	//std::wstring test_wstring = L"Hello";
	//std::wstring test_wstring = L"英語って、よく分からないんだ。。。";
	//tou::bitmap_string testbitmapstr(test_wstring, pointsize, face);
	//std::wstring filename = L"bitmap_string" + L"_" + test_wstring + L"_" + std::to_wstring(static_cast<int>(pointsize)) + L"pt_" + std::to_wstring(fileiteration);
	//makefile(L"test.bmp", testbitmapstr.get_bitmap());

	//tou::bitmap_image testimage(1000, 1000);
	//testimage.insert_other_bitmap_at_coordinate(testbitmapstr.get_bitmap(), { 0, 500 });
	//makefile(L"testinsert.bmp", testimage);

	uint16_t ch0 = 0x006D; // m
	render_glyph_to_bitmap_file(ch0, face, pointsize, fileiteration);
	
	uint16_t ch1 = 0x305D; // そ
	render_glyph_to_bitmap_file(ch1, face, pointsize, fileiteration);
	
	uint16_t ch3 = 0x0024;
	render_glyph_to_bitmap_file(ch3, face, pointsize, fileiteration);
	
	uint16_t ch2 = 0x51CB;
	render_glyph_to_bitmap_file(ch2, face, pointsize, fileiteration);
	
	uint16_t ch4 = 0x4F5D;
	render_glyph_to_bitmap_file(ch4, face, pointsize, fileiteration);
	
	//render_font_atlas_bitmap_file(face, fileiteration);
	

	return 0;
}