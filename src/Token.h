#pragma once

#include <string>
#include <stdexcept>
#include <memory>

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
	QUESTION,
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

	FIXED,
	STATIC,
	NODISCARD,
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
	DLOAD,
	DSET,

	T_EOF
};

struct Literal
{
	Literal();
	Literal(const std::string& string);
	Literal(double number);
	Literal(bool boolean);
	Literal(const Literal& other);
	Literal(Literal&& other) noexcept;

	Literal& operator=(const Literal& other);
	Literal& operator=(Literal&& other) noexcept;

	std::string display();

	bool is_number() const;
	bool is_string() const;
	bool is_hashstring() const;
	bool is_boolean() const;
	bool is_integral() const;

	int as_integer() const;
	double as_number() const;
	const std::string& as_string() const;
	const std::string& as_hash_string() const;
	bool as_boolean() const;

	std::string to_value_string() const;
	std::string to_lexeme() const;
	TokenType type() const;

	std::unique_ptr<double> number;
	std::unique_ptr<std::string> string;
	std::unique_ptr<bool> boolean;
	bool string_hashed;
};

class Token
{
public:
	Token(int line, TokenType type, const std::string& lexeme);
	Token(int line, TokenType type, const std::string& lexeme, const Literal& literal);
	Token(int line, TokenType type, const std::string& lexeme, bool literal);
	Token(int line, TokenType type, const std::string& lexeme, double literal);
	Token(int line, TokenType type, const std::string& lexeme, const std::string& literal);
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
