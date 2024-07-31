#include <iostream>
#include <fstream>
#include <string>
#include "font_face.hpp"
#include "bitmap.hpp"
#include "input_parser.hpp"

int main(int argc, char** argv)
{
	// expected {argument list}
	// fontface {arial.ttf 65 12 filename}
	// fontface -f arial.ttf -u 65 -p 12
	// fontface -font "arial.ttf" -unicode "65" -pointsize "12"

	input_parser args(argc, argv);

	constexpr int exit_success = 0;
	constexpr int exit_failure = -1;

	const std::string help_token = "-h";
	const std::string fontface_filepath_token = "-f";
	const std::string unicode_token = "-u";
	const std::string pointsize_token = "-p";
	const std::string output_filepath_token = "-o";

	if (args.token_exists(help_token))
	{
		std::cout << "Usage example: fontface " << fontface_filepath_token << " arial.ttf " << unicode_token << " 65 " << pointsize_token << " 12\n";
		std::cout << fontface_filepath_token << ": filename for truetype font file to use\n";
		std::cout << unicode_token << ": unicode codepoint in decimal format of desired glyph\n";
		std::cout << pointsize_token << ": desired pointsize of glyph (rendered using 300 dpi)\n";
		std::cout << output_filepath_token << ": path to write output to. Omit to use the current directory. A .bmp extension on the last path component specifies the filename to use." << std::endl;
		return exit_success;
	}
	if (!args.token_exists(fontface_filepath_token) && !args.token_exists(unicode_token) && !args.token_exists(pointsize_token))
	{
		std::cout << "Insufficient parameters provided. Use 'fontface -h' for help." << std::endl;
		return exit_failure;
	}

	std::string fontface_filepath = args.get_token_value(fontface_filepath_token);
	std::string output_filepath = "";

	int codepoint = std::stoi(args.get_token_value(unicode_token).c_str());
	float pointsize = std::stof(args.get_token_value(pointsize_token).c_str());


	if (args.token_exists(output_filepath_token))
	{
		output_filepath = args.get_token_value(output_filepath_token);
	}

	std::string type = fontface_filepath.substr(fontface_filepath.size() - 4, 4);

#ifdef _DEBUG
	LOG("[Debug] type: " << type);
	LOG("[Debug] codepoint: " << codepoint);
	LOG("[Debug] pointsize: " << pointsize);
#endif

	if (type != ".ttf" && type != ".otf")
	{
		std::cout << "The file provided is not of a valid filetype. Please provide a .ttf or .otf font file." << std::endl;
		return exit_failure;
	}
	if (codepoint < 0 || codepoint > 0xFFFF)
	{
		std::cout << "The unicode codepoint given is not valid or exists outside the 16-bit standard unicode range.\nPlease provide a decimal value";
		std::cout << " between 0 and 65,535" << std::endl;
		return exit_failure;
	}
	if (pointsize < 0 || pointsize > 102.0f) // arbitrary limit of 102...
	{
		std::cout << "The given pointsize is outside the supported range. Please provide a pointsize value between 0 and 102.0" << std::endl;
		return exit_failure;
	}

	tou::font_face face(fontface_filepath);
	if (!face)
	{
		std::cout << "The font file could not be loaded. Please verify that the given file is a valid\n";
		std::cout << " truetype (.ttf) or opentype (.otf) font file" << std::endl;
		return exit_failure;
	}
	
	auto glyph = face.get_glyph_bitmap(static_cast<uint16_t>(codepoint), pointsize, true, true);
	
	std::string filename = "glyph" + std::to_string(codepoint) + "@" + std::to_string(static_cast<int>(pointsize)) + "pt.bmp";
	if (output_filepath.length() > 0)
	{
		std::string type = output_filepath.substr(output_filepath.size() - 4, 4);
		if (type != ".bmp")
			filename = output_filepath + "/" + filename;
		else
			filename = output_filepath;
	}


	std::cout << "Writing bitmap to " << filename << "..." << std::endl;
	std::vector<char> v;
	glyph.image.file(v);
	std::ofstream out;
	out.open(filename, std::ios::binary | std::ios::out);
	out.write(v.data(), v.size());
	out.close();
	std::cout << "Done." << std::endl;

	return exit_success;
}