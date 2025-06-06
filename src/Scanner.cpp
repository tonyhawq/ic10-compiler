#include "Scanner.h"
#include "Compiler.h"

#define CHECK_SINGLE_TOKEN(wanted_character, token_type) case wanted_character: this->add_token(token_type); break

#define CHECK_TWO_TOKENS(wanted_character, second_character, first_type, second_type) case wanted_character: this->add_token(this->match(second_character) ? second_type : first_type); break

#define SWITCH_CASE_DIGIT case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9'

std::unordered_map<std::string, TokenType> Scanner::keywords = {
	{"and", TokenType::AND},
	{"or", TokenType::OR},
	{"true", TokenType::TRUE},
	{"false", TokenType::FALSE},
	{"for", TokenType::FOR},
	{"while", TokenType::WHILE},
	{"if", TokenType::IF},
	{"else", TokenType::ELSE},
	{"break", TokenType::BREAK},
	{"return", TokenType::RETURN},
	{"function", TokenType::FUNCTION},
	{"asm", TokenType::ASM},
	{"print", TokenType::PRINT},
	{"struct", TokenType::STRUCT},
	{"const", TokenType::CONST},
};

Scanner::Scanner(Compiler& compiler, const std::string& in)
	:source(in), current_character(0), current_line(1), token_start(0), compiler(compiler)
{

}

std::vector<Token> Scanner::scan()
{
	while (!this->at_eof())
	{
		this->token_start = this->current_character;
		this->scan_token();
	}
	this->tokens.emplace_back(this->current_line, TokenType::T_EOF, "");
	return std::move(this->tokens);
}

bool Scanner::at_eof()
{
	return this->current_character >= this->source.size();
}

char Scanner::advance()
{
	size_t to_eat = this->current_character;
	this->current_character++;
	return this->source.at(to_eat);
}

void Scanner::add_token(TokenType type)
{
	this->add_token(type, nullptr);
}

void Scanner::add_token(TokenType type, double literal)
{
	this->tokens.emplace_back(this->current_line, type, this->source.substr(this->token_start, this->current_character - this->token_start), literal);
}

void Scanner::add_token(TokenType type, const char* literal)
{
	this->tokens.emplace_back(this->current_line, type, this->source.substr(this->token_start, this->current_character - this->token_start), literal);
}

void Scanner::add_token(TokenType type, bool literal)
{
	this->tokens.emplace_back(this->current_line, type, this->source.substr(this->token_start, this->current_character - this->token_start), literal);
}

bool Scanner::match(char character)
{
	if (this->at_eof())
	{
		return false;
	}
	if (this->source.at(this->current_character) != character)
	{
		return false;
	}
	this->advance();
	return true;
}

char Scanner::peek()
{
	if (this->at_eof())
	{
		return '\0';
	}
	return this->source.at(this->current_character);
}

char Scanner::peek_ahead()
{
	this->current_character++;
	if (this->at_eof())
	{
		this->current_character--;
		return '\0';
	}
	this->current_character--;
	return this->source.at(this->current_character + 1);
}

void Scanner::scan_hashed_string()
{
	while (this->peek() != '"' && !this->at_eof())
	{
		if (this->peek() == '\n')
		{
			this->current_line++;
		}
		this->advance();
	}
	if (this->at_eof())
	{
		this->compiler.error(this->current_line, "Unterminated hashed string found.");
	}
	this->advance();
	std::string substr = this->source.substr(this->token_start, this->current_character - this->token_start);
	this->add_token(TokenType::HASHED_STRING, substr.c_str());
}

bool Scanner::is_digit(char character)
{
	return character >= '0' && character <= '9';
}

bool Scanner::is_alpha(char character)
{
	return (character >= 'a' && character <= 'z') ||
		(character >= 'A' && character <= 'Z') ||
		character == '_';
}

bool Scanner::is_alphanumeric(char character)
{
	return this->is_alpha(character) || this->is_digit(character);
}

void Scanner::scan_string()
{
	while (this->peek() != '"' && !this->at_eof())
	{
		if (this->peek() == '\n')
		{
			this->current_line++;
		}
		this->advance();
	}
	if (this->at_eof())
	{
		this->compiler.error(this->current_line, "Unterminated string found.");
	}
	this->advance();
	std::string substr = this->source.substr(this->token_start, this->current_character - this->token_start);
	this->add_token(TokenType::STRING, substr.c_str());
}

void Scanner::scan_number()
{
	while (this->is_digit(this->peek()))
	{
		this->advance();
	}
	if (this->peek() == '.' && this->is_digit(this->peek_ahead()))
	{
		this->advance();
		while (this->is_digit(this->peek()))
		{
			this->advance();
		}
	}
	this->add_token(TokenType::NUMBER, std::strtod(this->source.c_str() + this->token_start, nullptr));
}

void Scanner::scan_identifier()
{
	while (this->is_alphanumeric(this->peek()))
	{
		this->advance();
	}
	std::string identifier = this->source.substr(this->token_start, this->current_character - this->token_start);
	if (Scanner::keywords.count(identifier))
	{
		TokenType type = Scanner::keywords.at(identifier);
		switch (type)
		{
		case TokenType::TRUE:
			this->add_token(type, true);
			break;
		case TokenType::FALSE:
			this->add_token(type, false);
			break;
		default:
			this->add_token(type);
			break;
		}
		return;
	}
	this->add_token(TokenType::IDENTIFIER);
}

void Scanner::scan_token()
{
	char character = advance();
	switch (character)
	{
		CHECK_SINGLE_TOKEN('(', TokenType::LEFT_PAREN);
		CHECK_SINGLE_TOKEN(')', TokenType::RIGHT_PAREN);
		CHECK_SINGLE_TOKEN('{', TokenType::LEFT_BRACE);
		CHECK_SINGLE_TOKEN('}', TokenType::RIGHT_BRACE);
		CHECK_SINGLE_TOKEN(',', TokenType::COMMA);
		CHECK_SINGLE_TOKEN('.', TokenType::DOT);
		CHECK_SINGLE_TOKEN('+', TokenType::PLUS);
		CHECK_SINGLE_TOKEN(';', TokenType::SEMICOLON);
		CHECK_SINGLE_TOKEN('*', TokenType::STAR);
		CHECK_SINGLE_TOKEN('&', TokenType::AMPERSAND);
		CHECK_SINGLE_TOKEN('|', TokenType::BAR);
		CHECK_SINGLE_TOKEN('?', TokenType::QUESTION);

		CHECK_TWO_TOKENS('!', '=', TokenType::BANG, TokenType::BANG_EQUAL);
		CHECK_TWO_TOKENS('-', '>', TokenType::MINUS, TokenType::ARROW);
		CHECK_TWO_TOKENS('=', '=', TokenType::EQUAL, TokenType::EQUAL_EQUAL);
		CHECK_TWO_TOKENS('<', '=', TokenType::LESS, TokenType::LESS_EQUAL);
		CHECK_TWO_TOKENS('>', '=', TokenType::GREATER, TokenType::GREATER_EQUAL);
		
	case '#':
		if (this->match('"'))
		{
			this->scan_hashed_string();
		}
		else
		{
			while (this->peek() != '\n' && !this->at_eof())
			{
				this->advance();
			}
		}
		break;
	case '"':
		this->scan_string();
		break;

	case ' ':
	case '\r':
	case '\t':
		break;
	case '\n':
		this->current_line++;
		break;
	SWITCH_CASE_DIGIT:
		this->scan_number();
		break;
	default:
		if (this->is_alpha(character))
		{
			this->scan_identifier();
			break;
		}
		this->compiler.error(this->current_line, std::string("Unexpected character: '") + character + "'");
		break;
	}
}
