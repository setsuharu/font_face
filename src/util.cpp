#include <iostream>
#include <exception>
#include <fstream>
#include <string>
#include <filesystem>
#include "util.hpp"

namespace tou
{
	std::array<char, 4> split_bytes(uint32_t data)
	{
		uint32_t temp = data;
		char w = temp;
		temp >>= 8;
		char z = temp;
		temp >>= 8;
		char y = temp;
		temp >>= 8;
		char x = temp;

		return { x, y, z, w };
	}

	std::array<char, 4> split_bytes(int32_t data)
	{
		int32_t temp = data;
		char w = temp;
		temp >>= 8;
		char z = temp;
		temp >>= 8;
		char y = temp;
		temp >>= 8;
		char x = temp;

		return { x, y, z, w };
	}

	std::array<char, 2> split_bytes(uint16_t data)
	{
		uint16_t temp = data;
		char y = static_cast<char>(temp);
		temp >>= 8;
		char x = static_cast<char>(temp);

		return { x, y };
	}

	uint32_t join_bytes(const std::array<char, 4>& b)
	{
		uint32_t x = b[0];
		x = static_cast<uint8_t>(x);
		x <<= 24;

		uint32_t y = b[1];
		y = static_cast<uint8_t>(y);
		y <<= 16;

		uint32_t z = b[2];
		z = static_cast<uint8_t>(z);
		z <<= 8;

		uint32_t v = b[3];
		v = static_cast<uint8_t>(v);

		return (x | y | z | v);
	}

	uint16_t join_bytes(char b1, char b2)
	{
		uint16_t x = b1;
		x <<= 8;

		uint8_t x2 = b2;
		return (x | x2) ;
	}

	int32_t join_bytes_signed(const std::array<char, 4>& b)
	{
		int32_t x = (int8_t)b[0];
		x <<= 24;

		int32_t x2 = (int8_t)b[1];
		x2 <<= 16;

		int32_t x3 = (int8_t)b[2];
		x3 <<= 8;

		int32_t x4 = (int8_t)b[3];
		
		return (x | x2 | x3 | x4);
	}

	int16_t join_bytes_signed(char b1, char b2)
	{
		int16_t x = b1;
		x = (int8_t)x;
		x <<= 8;

		int16_t x2 = b2;
		x2 = (int8_t)x2;
		x2 &= 0x00FF;

		return (x | x2);
	}

	uint32_t little_endian(uint32_t data)
	{
		std::array<char, 4> bytes = split_bytes(data);
		std::array<char, 4> rbytes{0};
		uint32_t i = 0;
		for (auto rit = bytes.rbegin(); rit < bytes.rend(); rit++)
		{
			rbytes[i] = *rit;
			i++;
		}
		return join_bytes(rbytes);
	}


	vector_reader::vector_reader()
		:m_current_position(0)
	{
	}

	vector_reader::vector_reader(const std::string& filepath)
		:m_current_position(0)
	{
		load(filepath);
	}

	vector_reader::~vector_reader()
	{
	}

	bool vector_reader::load(const std::string& filepath)
	{
		std::ifstream input;
		try
		{
			input.open(filepath, std::ios::binary);
			if (!input)
			{
				throw std::system_error(errno, std::system_category(), "Failed to open " + filepath);
			}
		}
		catch (const std::exception& e)
		{
			LOG(e.what());
			return false;
		}
		m_bytes.reserve(static_cast<size_t>(std::filesystem::file_size(filepath)));
		m_bytes = std::vector<char>(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
		input.close();
		return true;
	}

	uint32_t vector_reader::get_uint32()
	{
		uint32_t x = join_bytes({ m_bytes[m_current_position], m_bytes[m_current_position + 1], m_bytes[m_current_position + 2], m_bytes[m_current_position + 3] });
		m_current_position += 4;
		return x;
	}

	uint16_t vector_reader::get_uint16()
	{
		uint16_t x = join_bytes(m_bytes[m_current_position], m_bytes[m_current_position + 1]);
		m_current_position += 2;
		return x;
	}

	uint8_t vector_reader::get_uint8()
	{
		uint8_t x = (uint8_t)m_bytes[m_current_position];
		m_current_position += 1;
		return x;
	}

	int32_t vector_reader::get_int32()
	{
		int32_t x = join_bytes_signed({ m_bytes[m_current_position], m_bytes[m_current_position + 1], m_bytes[m_current_position + 2], m_bytes[m_current_position + 3] });
		m_current_position += 4;
		return x;
	}

	int16_t vector_reader::get_int16()
	{
		int16_t x = join_bytes_signed(m_bytes[m_current_position], m_bytes[m_current_position + 1]);
		m_current_position += 2;
		return x;
	}

}


