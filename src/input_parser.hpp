#pragma once
#include <vector>
#include <string>

class input_parser
{
public:
	input_parser(int argc, char** argv);
	~input_parser() = default;

	bool token_exists(const std::string& token);
	const std::string& get_token_value(const std::string& token);

private:
	std::vector<std::string> m_tokens;
};