#include "input_parser.hpp"

input_parser::input_parser(int argc, char** argv)
	:m_tokens(argv, argv + argc)
{
}

bool input_parser::token_exists(const std::string& token)
{
	return std::find(m_tokens.begin(), m_tokens.end(), token) != m_tokens.end();
}

const std::string& input_parser::get_token_value(const std::string& token)
{
	auto it = std::find(m_tokens.begin(), m_tokens.end(), token);
	if (it != m_tokens.end() && ++it != m_tokens.end())
	{
		return *it;
	}
	static const std::string empty("");
	return empty;
}
