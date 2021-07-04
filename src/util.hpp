#pragma once
#include <string>
#include <array>
#include <vector>

#ifdef _DEBUG
	#include <iostream>
	#include <cassert>
	#define LOG(x) std::cout << x << '\n'
	#define debug_break(message) LOG(message); assert(false)
#else
	#define LOG(x)
	#define debug_break(message)
#endif

#define FLT(x) static_cast<float>(x)

namespace tou
{
	struct ivec2
	{
		uint32_t x = 0, y = 0;
	};

	struct fvec2
	{
		float x = 0.0f, y = 0.0f;
	};

	std::array<char, 4> split_bytes(uint32_t data);
	std::array<char, 4> split_bytes(int32_t data);
	std::array<char, 2> split_bytes(uint16_t data);

	uint32_t join_bytes(const std::array<char, 4>& b);
	uint16_t join_bytes(char b1, char b2);
	int32_t join_bytes_signed(const std::array<char, 4>& b);
	int16_t join_bytes_signed(char b1, char b2);

	uint32_t little_endian(uint32_t data);

	class vector_reader
	{
	public:
		vector_reader();
		vector_reader(const std::string& filepath);
		~vector_reader();

		bool load(const std::string& filepath);

		void set_position(size_t x) { m_current_position = x; }
		void increment_position(size_t x) { m_current_position += x; }

		size_t get_position() const { return m_current_position; }
		const std::vector<char>& get_array() const { return m_bytes; }
		uint32_t get_uint32();
		uint16_t get_uint16();
		uint8_t get_uint8();
		int32_t get_int32();
		int16_t get_int16();

	private:
		size_t m_current_position;
		std::vector<char> m_bytes;
	};
}