#pragma once

#include <vector>
#include <unordered_map>

#include "Errors.h"
#include "Token.h"

class Compiler;

class Scanner
{
public:
	Scanner(Compiler& compiler, const std::string& in);
	std::vector<Token> scan();
	bool at_eof();
	void scan_token();
	char advance();
	Token& add_token(TokenType type);
	Token& add_token(TokenType type, const char* literal);
	Token& add_token(TokenType type, double literal);
	Token& add_token(TokenType type, bool literal);
	void scan_hashed_string();
	void scan_string();
	void scan_number();
	void scan_identifier();
	bool is_digit(char character);
	bool is_alpha(char character);
	bool is_alphanumeric(char character);
	bool match(char character);
	char peek();
	char peek_ahead();

	static std::unordered_map<std::string, TokenType> keywords;
private:
	Compiler& compiler;
	std::string source;
	size_t current_character;
	size_t token_start;
	int current_line;
	std::vector<Token> tokens;
};

