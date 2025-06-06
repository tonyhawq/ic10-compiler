#pragma once

#include <string>

enum class TokenType
{
	LEFT_PAREN,
	RIGHT_PAREN,
	LEFT_BRACE,
	RIGHT_BRACE,
	COMMA,
	DOT,
	MINUS,
	PLUS,
	SEMICOLON,
	SLASH,
	STAR,
	AMPERSAND,
	HASH,
	BAR,

	BANG,
	BANG_EQUAL,
	EQUAL,
	EQUAL_EQUAL,
	GREATER,
	GREATER_EQUAL,
	LESS,
	LESS_EQUAL,
	ARROW,

	IDENTIFIER,
	NUMBER,
	STRING,
	HASHED_STRING,

	QUESTION,
	CONST,
	AND,
	OR,
	STRUCT,
	IF,
	ELSE,
	FUNCTION,
	FOR,
	WHILE,
	BREAK,
	RETURN,
	TRUE,
	FALSE,
	ASM,
	PRINT,

	T_EOF
};

struct Literal
{
	Literal();
	Literal(const char* string);
	Literal(double number);
	Literal(bool boolean);
	~Literal();
	Literal(const Literal& other);
	Literal(Literal&& other) noexcept;
	std::string display();

	double* number;
	std::string* string;
	bool* boolean;
};

class Token
{
public:
	Token(int line, TokenType type, const std::string& lexeme);
	Token(int line, TokenType type, const std::string& lexeme, bool literal);
	Token(int line, TokenType type, const std::string& lexeme, double literal);
	Token(int line, TokenType type, const std::string& lexeme, const char* literal);
	Token(const Token& other);
	Token(Token&& other) noexcept;
	~Token();

	std::string to_string() const;

	int line;
	TokenType type;
	std::string lexeme;
	Literal literal;
private:
};
