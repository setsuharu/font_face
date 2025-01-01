#include <iostream>
#include <fstream>
#include <string>

#include <argparse/argparse.hpp>

#include "font_face.hpp"
#include "bitmap/bitmap.hpp"

int main(int argc, char** argv)
{
    argparse::ArgumentParser program("fontface", "2024.12.0", argparse::default_arguments::help, true);

    const std::string arg_fontpath = "fontpath";
    const std::string arg_unicode = "unicode";
    const std::string arg_pointsize = "pointsize";
    const std::string arg_output = "output";

    program.add_argument(arg_fontpath).help("Path to a truetype font file.");
    program.add_argument(arg_unicode).help("Decimal representation of desired glyph's unicode codepoint.").default_value(65).scan<'i', int>();
    program.add_argument("-p", "--" + arg_pointsize).help("If writing a bitmap, this is the integer value denoting the pointsize to return the glyph as.").default_value(12).scan<'i', int>();
    program.add_argument("-o", "--" + arg_output).help("Write the glyph bitmap to a given path.");

    try
    {
        program.parse_args(argc, argv);
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        std::cerr << program;
        return EXIT_FAILURE;
    }

    std::string font_path = program.get<std::string>(arg_fontpath);
    int codepoint = program.get<int>(arg_unicode);
    int pointsize = program.get<int>(arg_pointsize);
    
    std::string out_path;
    if(program.present("-o"))
    {
        out_path = program.get<std::string>(arg_output);
    }

    std::string type = font_path.substr(font_path.size() - 4);
    if (type != ".ttf" && type != ".otf")
    {
        std::cout << "The provided font file with type " << type << " does not seem to be a TrueType or OpenType font file\n";
        return EXIT_FAILURE;
    }
    if (codepoint < 0 || codepoint > 0xFFFF)
	{
		std::cout << "The unicode codepoint given is not valid or exists outside the 16-bit standard unicode range.\nPlease provide a decimal value between 0 and 65,535\n";
		return EXIT_FAILURE;
	}
    if (pointsize < 0 || pointsize > 102.0f) // arbitrary limit of 102...
	{
		std::cout << "The given pointsize is outside the supported range. Please provide a pointsize value between 0 and 102\n";
		return EXIT_FAILURE;
	}

    tou::font_face face(font_path);
    if (!face)
    {
        std::cout << "The font file could not be loaded. Please verify that the given file is a valid truetype (.ttf) or opentype (.otf) font file\n";
		return EXIT_FAILURE;
    }


    if (!out_path.empty())
    {
        tou::font_face::bitmap_glyph glyph = face.get_glyph_bitmap(static_cast<uint16_t>(codepoint), pointsize, true, true);
        std::string filename = "glyph" + std::to_string(codepoint) + "@" + std::to_string(static_cast<int>(pointsize)) + "pt.bmp";
	    if (out_path.length() > 0)
	    {
	    	std::string type = out_path.substr(out_path.size() - 4);
	    	if (type != ".bmp")
	    		filename = out_path + "/" + filename; // assuming just a directory was given
	    	else
	    		filename = out_path;
	    }
        std::vector<char> bitmap_data;
        glyph.image.file(bitmap_data);
        std::ofstream out;
        out.open(filename, std::ios::binary | std::ios::out);
        out.write(bitmap_data.data(), bitmap_data.size());
        out.close();
    }
    else
    {
        tou::font_face::truetype_glyph glyph = face.get_glyph(codepoint);
        std::cout << "glyph metrics:\n";
        std::cout << "id: " << glyph.id << "\n";
        std::cout << "x_min, ymin: " << glyph.x_min << ", " << glyph.y_min << "\n";
        std::cout << "x_max, y_max: " << glyph.x_max << ", " << glyph.y_max << "\n";
        std::cout << "num_contours: " << glyph.num_contours << "\n";
        std::cout << "num_points: " << glyph.num_points << "\n";
        std::cout << "instruction_len: " << glyph.instruction_len << "\n";
        std::cout << "advance_width: " << glyph.advance_width << "\n";
        std::cout << "left_side_bearing: " << glyph.left_side_bearing << "\n";
    }

    return EXIT_SUCCESS;
}