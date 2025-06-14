#include "Parser.h"

#include "Compiler.h"

Parser::Parser(Compiler& compiler, std::vector<Token> tokens)
	:tokens(std::move(tokens)), compiler(compiler), current_token(0)
{}

void Parser::synchronize()
{
	this->advance();

	while (!this->is_eof()) {
		if (this->peek_previous().type == TokenType::SEMICOLON) return;

		switch (peek().type) {
		case TokenType::STRUCT:
		case TokenType::FUNCTION:
		case TokenType::FOR:
		case TokenType::IF:
		case TokenType::WHILE:
		case TokenType::PRINT:
		case TokenType::ASM:
		case TokenType::RETURN:
			return;
		}

		this->advance();
	}
}

TypeName Parser::parse_type()
{
	// type:
	// const | fixed identifier (pointer_decl)*
	// pointer decl:
	// const? star question?
	bool base_is_const = false;
	bool base_is_fixed = false;
	if (this->match({ TokenType::CONST }))
	{
		base_is_const = true;
	}
	else if (this->match({ TokenType::FIXED }))
	{
		base_is_fixed = true;
	}
	const Token& type = this->consume(TokenType::IDENTIFIER, "Expected type name");
	TypeName type_info = TypeName(base_is_const, type.lexeme);
	if (base_is_fixed)
	{
		type_info.compile_time = true;
	}
	while (true)
	{
		bool is_const = false;
		if (this->match({ TokenType::CONST }))
		{
			is_const = true;
		}
		if (!this->match({ TokenType::STAR }))
		{
			if (is_const)
			{
				this->error(this->peek(), "Expected * in pointer decl");
			}
			break;
		}
		bool is_nullable = false;
		if (this->match({ TokenType::QUESTION }))
		{
			is_nullable = true;
		}
		type_info.make_pointer_type(is_const, is_nullable);
	}
	return type_info;
}

std::unique_ptr<Stmt> Parser::parse_static_declaration()
{
	return std::make_unique<Stmt::Static>(std::unique_ptr<Stmt::Variable>(dynamic_cast<Stmt::Variable*>(this->parse_variable_declaration().release())));
}

std::unique_ptr<Stmt> Parser::parse_variable_declaration()
{
	TypeName type_info = this->parse_type();
	const Token& name = this->consume(TokenType::IDENTIFIER, "Expected variable name");
	std::shared_ptr<Expr> initalizer;
	if (this->match({ TokenType::EQUAL }))
	{
		initalizer = this->parse_expression();
	}
	this->consume(TokenType::SEMICOLON, "Expected semicolon after variable declaration.");
	return std::make_unique<Stmt::Variable>(std::move(type_info), name, initalizer);
}

std::unique_ptr<Stmt> Parser::parse_function_declaration()
{
	const Token& name = this->consume(TokenType::IDENTIFIER, "Expected function name.");
	this->consume(TokenType::LEFT_PAREN, "Expected ( following function name.");
	std::vector<Stmt::Function::Param> params;
	if (!this->current_token_is(TokenType::RIGHT_PAREN))
	{
		do {
			params.push_back({ this->parse_type(), this->consume(TokenType::IDENTIFIER, "Expected argument name.") });
		} while (this->match({ TokenType::COMMA }));
	}
	this->consume(TokenType::RIGHT_PAREN, "Expected ) following function name and arguments.");
	bool attribute_nodiscard = false;
	if (this->match({ TokenType::NODISCARD }))
	{
		attribute_nodiscard = true;
	}
	this->consume(TokenType::ARROW, "Expected -> to denote return type following function name and arguments.");
	TypeName return_type = this->parse_type();
	consume(TokenType::LEFT_BRACE, "Expected { before function body.");
	std::unique_ptr<Stmt::Block> body(dynamic_cast<Stmt::Block*>(this->parse_block().release()));
	return std::make_unique<Stmt::Function>(name, std::move(return_type), params, std::move(body->statements));
}

bool Parser::peek_var_decl()
{
	return (this->peek().type == TokenType::IDENTIFIER && (
		this->peek_ahead(1).type == TokenType::IDENTIFIER ||
		this->peek_ahead(1).type == TokenType::CONST ||
		this->peek_ahead(1).type == TokenType::STAR
		)) || (this->peek().type == TokenType::CONST || this->peek().type == TokenType::FIXED);
}

std::unique_ptr<Stmt> Parser::parse_symbols()
{
	try
	{
		if (this->match({ TokenType::FUNCTION }))
		{
			return this->parse_function_declaration();
		}
		if (this->match({TokenType::STATIC}))
		{
			return this->parse_static_declaration();
		}
		this->error(this->peek(), "Expected a function or static variable declaration.");
	}
	catch (ParseError)
	{
		this->synchronize();
		return nullptr;
	}
	this->error(this->peek(), "Fell through.");
	return nullptr;
}

std::unique_ptr<Stmt> Parser::parse_declaration()
{
	try
	{
		if (this->peek_var_decl())
		{
			return this->parse_variable_declaration();
		}
		if (this->peek().type == TokenType::IDENTIFIER && this->peek_ahead(1).type == TokenType::EQUAL)
		{
			// variable assignment
			std::shared_ptr<Expr> assignment_expression = this->parse_assignment();
			this->consume(TokenType::SEMICOLON, "Expected semicolon after variable assignment.");
			return std::make_unique<Stmt::Expression>(assignment_expression);
		}
		return this->parse_statement();
	}
	catch (ParseError)
	{
		this->synchronize();
		return nullptr;
	}
	return nullptr;
}

std::unique_ptr<Stmt> Parser::parse_statement()
{
	if (this->match({ TokenType::ASM }))
	{
		return this->parse_asm_statement();
	}
	if (this->match({ TokenType::WHILE }))
	{
		return this->parse_while_statement();
	}
	if (this->match({ TokenType::FOR }))
	{
		return this->parse_for_statement();
	}
	if (this->match({ TokenType::PRINT }))
	{
		return this->parse_print_statement();
	}
	if (this->match({ TokenType::DSET }))
	{
		return this->parse_device_set_statement();
	}
	if (this->match({ TokenType::IF }))
	{
		return this->parse_if();
	}
	if (this->match({ TokenType::RETURN }))
	{
		return this->parse_return_statement();
	}
	if (this->match({ TokenType::LEFT_BRACE }))
	{
		return this->parse_block();
	}
	return this->parse_expression_statement();
}

std::vector<std::unique_ptr<Stmt>> Parser::parse()
{
	std::vector<std::unique_ptr<Stmt>> statements;
	while (!this->is_eof())
	{
		std::unique_ptr<Stmt> statement = this->parse_symbols();
		if (statement)
		{
			statements.push_back(std::move(statement));
		}
	}
	return statements;
}

std::unique_ptr<Stmt> Parser::parse_for_statement()
{
	const Token& token = this->peek();
	this->consume(TokenType::LEFT_PAREN, "Expected ( following for.");
	std::unique_ptr<Stmt> initalizer;
	if (this->match({ TokenType::SEMICOLON })) {}
	else if (this->peek().type == TokenType::IDENTIFIER && this->peek_ahead(1).type == TokenType::IDENTIFIER)
	{
		initalizer = this->parse_variable_declaration();
	}
	else
	{
		initalizer = this->parse_expression_statement();
	}
	// Parser::consume semicolon skipped because it would have been done inside parse_variable_declaration() or parse_expression_statement()
	std::shared_ptr<Expr> condition;
	if (this->current_token_is(TokenType::SEMICOLON)) {}
	else
	{
		condition = this->parse_expression();
	}
	this->consume(TokenType::SEMICOLON, "Expected ; following for condition");
	std::shared_ptr<Expr> increment;
	if (this->current_token_is(TokenType::RIGHT_PAREN)) {}
	else
	{
		increment = this->parse_expression();
	}
	const Token& right_paren = this->consume(TokenType::RIGHT_PAREN, "Expected ) following for header");
	std::unique_ptr<Stmt> body = this->parse_statement();

	if (increment)
	{
		std::vector<std::unique_ptr<Stmt>> new_body; 
		new_body.push_back(std::move(body));
		new_body.push_back(std::make_unique<Stmt::Expression>(increment));
		body = std::make_unique<Stmt::Block>(std::move(new_body), right_paren);
	}
	if (!condition)
	{
		condition = std::make_shared<Expr::Literal>(Token(token.line, TokenType::TRUE, "true", true));
	}
	body = std::make_unique<Stmt::While>(token, condition, std::move(body));
	if (initalizer)
	{
		std::vector<std::unique_ptr<Stmt>> new_body;
		new_body.push_back(std::move(initalizer));
		new_body.push_back(std::move(body));
		body = std::make_unique<Stmt::Block>(std::move(new_body), right_paren);
	}
	return body;
}

std::unique_ptr<Stmt> Parser::parse_while_statement()
{
	const Token& token = this->peek_previous();
	this->consume(TokenType::LEFT_PAREN, "Expected ( following while.");
	std::shared_ptr<Expr> condition = this->parse_expression();
	this->consume(TokenType::RIGHT_PAREN, "Expected ) following while condition.");
	std::unique_ptr<Stmt> body = this->parse_statement();
	return std::make_unique<Stmt::While>(token, condition, std::move(body));
}

std::unique_ptr<Stmt> Parser::parse_return_statement()
{
	const Token& token = this->peek_previous();
	std::shared_ptr<Expr> value;
	if (!this->current_token_is(TokenType::SEMICOLON))
	{
		value = this->parse_expression();
	}
	this->consume(TokenType::SEMICOLON, "Expected ; following return value.");
	return std::make_unique<Stmt::Return>(token, value);
}

std::unique_ptr<Stmt> Parser::parse_if()
{
	const Token& token = this->peek();
	this->consume(TokenType::LEFT_PAREN, "Expected ( after if");
	std::shared_ptr<Expr> condition = this->parse_expression();
	this->consume(TokenType::RIGHT_PAREN, "Expected ) after if condition.");
	std::unique_ptr<Stmt> true_branch = this->parse_statement();
	std::unique_ptr<Stmt> false_branch = nullptr;
	if (this->match({ TokenType::ELSE }))
	{
		false_branch = this->parse_statement();
	}
	return std::make_unique<Stmt::If>(token, condition, std::move(true_branch), std::move(false_branch));
}

std::unique_ptr<Stmt> Parser::parse_block()
{
	std::vector<std::unique_ptr<Stmt>> statements;
	bool seen_return = false;
	while (!this->current_token_is(TokenType::RIGHT_BRACE) && !this->is_eof())
	{
		std::unique_ptr<Stmt> statement = this->parse_declaration();
		if (statement)
		{
			statements.push_back(std::move(statement));
		}
	}
	const Token& right_brace = this->consume(TokenType::RIGHT_BRACE, "Expected } after block.");
	return std::make_unique<Stmt::Block>(std::move(statements), right_brace);
}

std::unique_ptr<Stmt> Parser::parse_print_statement()
{
	std::shared_ptr<Expr> expression = this->parse_expression();
	this->consume(TokenType::SEMICOLON, "Expected semicolon after print literal.");
	return std::make_unique<Stmt::Print>(expression);
}

std::unique_ptr<Stmt> Parser::parse_device_set_statement()
{
	Token token = this->peek_previous();
	std::shared_ptr<Expr> device = this->parse_expression();
	Token logic_type = this->consume(TokenType::STRING, "Expected string for device set logic type.");
	std::shared_ptr<Expr> value = this->parse_expression();
	this->consume(TokenType::SEMICOLON, "Expected ;");
	return std::make_unique<Stmt::DeviceSet>(token, device, logic_type, value);
}

std::unique_ptr<Stmt> Parser::parse_asm_statement()
{
	std::shared_ptr<Expr> expression = this->parse_expression();
	if (!expression->is<Expr::Literal>())
	{
		this->error(this->peek(), "Expected string literal following asm.");
	}
	if (!dynamic_cast<Expr::Literal&>(*expression).literal.literal.string)
	{	
		this->error(this->peek(), "Expected string literal following asm.");
	}
	this->consume(TokenType::SEMICOLON, "Expected semicolon after asm literal.");
	return std::make_unique<Stmt::Asm>(expression);
}

std::unique_ptr<Stmt> Parser::parse_expression_statement()
{
	std::shared_ptr<Expr> expression = this->parse_expression();
	this->consume(TokenType::SEMICOLON, "Expected semicolon after expression.");
	return std::make_unique<Stmt::Expression>(expression);
}

bool Parser::match(const std::vector<TokenType>& types)
{
	for (const auto& type : types)
	{
		if (this->current_token_is(type))
		{
			this->advance();
			return true;
		}
	}
	return false;
}

const TokenType* Parser::matching_type(const std::vector<TokenType>& types)
{
	if (this->match(types))
	{
		return &(this->peek_previous().type);
	}
	return nullptr;
}

bool Parser::current_token_is(TokenType type)
{
	if (this->is_eof())
	{
		return false;
	}
	return this->peek().type == type;
}

const Token& Parser::peek()
{
	return this->tokens.at(this->current_token);
}

const Token& Parser::peek_ahead(size_t amount)
{
	if (this->current_token + amount >= this->tokens.size())
	{
		return this->tokens[this->tokens.size() - 1];
	}
	return this->tokens[this->current_token + amount];
}

const Token& Parser::advance()
{
	if (!this->is_eof())
	{
		this->current_token++;
	}
	return this->peek_previous();
}

const Token& Parser::peek_previous()
{
	return this->tokens.at(this->current_token - 1);
}

bool Parser::is_eof()
{
	return this->peek().type == TokenType::T_EOF;
}

std::shared_ptr<Expr> Parser::parse_expression()
{
	return this->parse_assignment();
}

std::shared_ptr<Expr> Parser::parse_assignment()
{
	std::shared_ptr<Expr> expression = this->parse_or();
	if (this->match({ TokenType::EQUAL }))
	{
		this->current_token--;
		const Token& equals = this->peek_previous();
		this->current_token++;
		std::shared_ptr<Expr> value = this->parse_assignment();
		if (expression->is<Expr::Variable>())
		{
			const Token& name = dynamic_cast<Expr::Variable&>(*expression).name;
			return std::make_shared<Expr::Assignment>(name, value);
		}
		this->error(equals, "Invalid target for assignment.");
	}
	return expression;
}

std::shared_ptr<Expr> Parser::parse_or()
{
	std::shared_ptr<Expr> expression = this->parse_and();
	while (this->match({ TokenType::OR }))
	{
		const Token& op = this->peek_previous();
		std::shared_ptr<Expr> right = this->parse_and();
		expression = std::make_shared<Expr::Logical>(expression, op, right);
	}
	return expression;
}

std::shared_ptr<Expr> Parser::parse_and()
{
	std::shared_ptr<Expr> expression = this->parse_equality();
	while (this->match({ TokenType::AND }))
	{
		const Token& op = this->peek_previous();
		std::shared_ptr<Expr> right = this->parse_equality();
		expression = std::make_shared<Expr::Logical>(expression, op, right);
	}
	return expression;
}

std::shared_ptr<Expr> Parser::parse_equality()
{
	std::shared_ptr<Expr> expression = this->parse_comparison();
	while (this->match({ TokenType::BANG_EQUAL, TokenType::EQUAL_EQUAL }))
	{
		const Token& op = this->peek_previous();
		std::shared_ptr<Expr> right = this->parse_comparison();
		expression = std::make_shared<Expr::Binary>(expression, op, right);
	}
	return expression;
}

std::shared_ptr<Expr> Parser::parse_comparison()
{
	std::shared_ptr<Expr> expression = this->parse_term();
	while (this->match({ TokenType::GREATER, TokenType::GREATER_EQUAL, TokenType::LESS, TokenType::LESS_EQUAL }))
	{
		const Token& op = this->peek_previous();
		std::shared_ptr<Expr> right = this->parse_term();
		expression = std::make_shared<Expr::Binary>(expression, op, right);
	}
	return expression;
}

std::shared_ptr<Expr> Parser::parse_term()
{
	std::shared_ptr<Expr> expression = this->parse_factor();
	while (this->match({ TokenType::PLUS, TokenType::MINUS }))
	{
		const Token& op = this->peek_previous();
		std::shared_ptr<Expr> right = this->parse_factor();
		expression = std::make_shared<Expr::Binary>(expression, op, right);
	}
	return expression;
}

std::shared_ptr<Expr> Parser::parse_factor()
{
	std::shared_ptr<Expr> expression = this->parse_unary();
	while (this->match({ TokenType::STAR, TokenType::SLASH }))
	{
		const Token& op = this->peek_previous();
		std::shared_ptr<Expr> right = this->parse_unary();
		expression = std::make_shared<Expr::Binary>(expression, op, right);
	}
	return expression;
}

std::shared_ptr<Expr> Parser::parse_unary()
{
	if (this->match({ TokenType::BANG, TokenType::MINUS, TokenType::AMPERSAND, TokenType::BAR }))
	{
		const Token& op = this->peek_previous();
		std::shared_ptr<Expr> right = this->parse_unary();
		return std::make_shared<Expr::Unary>(op, right);
	}
	return this->parse_call();
}

std::shared_ptr<Expr> Parser::finish_parse_call(std::shared_ptr<Expr> expression)
{
	std::vector<std::shared_ptr<Expr>> arguments;
	if (!this->current_token_is(TokenType::RIGHT_PAREN))
	{
		do {
			arguments.push_back(this->parse_expression());
		} while (this->match({ TokenType::COMMA }));
	}
	const Token& ending_paren = this->consume(TokenType::RIGHT_PAREN, "Expected closing paren after arguments.");
	return std::make_shared<Expr::Call>(expression, ending_paren, arguments);
}

std::shared_ptr<Expr> Parser::parse_call()
{
	std::shared_ptr<Expr> expression = this->parse_primary();
	while (true)
	{
		if (this->match({ TokenType::LEFT_PAREN }))
		{
			expression = this->finish_parse_call(expression);
		}
		else
		{
			break;
		}
	}
	return expression;
}

std::shared_ptr<Expr> Parser::parse_primary()
{
	if (this->match({ TokenType::FALSE }))
	{
		return std::make_shared<Expr::Literal>(this->peek_previous());
	}
	if (this->match({ TokenType::TRUE }))
	{
		return std::make_shared<Expr::Literal>(this->peek_previous());
	}
	if (this->match({ TokenType::NUMBER, TokenType::STRING, TokenType::HASHED_STRING }))
	{
		return std::make_shared<Expr::Literal>(this->peek_previous());
	}
	if (this->match({ TokenType::IDENTIFIER }))
	{
		return std::make_shared<Expr::Variable>(this->peek_previous());
	}
	if (this->match({ TokenType::DLOAD }))
	{
		return this->helper_parse_device_load();
	}
	if (this->match({ TokenType::LEFT_PAREN }))
	{
		std::shared_ptr<Expr> expression = this->parse_expression();
		this->consume(TokenType::RIGHT_PAREN, "Expect ) after expression.");
		return std::make_shared<Expr::Grouping>(expression);
	}
	this->error(this->peek(), "Expect expression.");
	return nullptr;
}

std::shared_ptr<Expr> Parser::helper_parse_device_load()
{
	std::shared_ptr<Expr> device = this->parse_expression();
	Token logic_type = this->consume(TokenType::STRING, "Expected string for device load logic type.");
	TypeName operation_type = TypeName("number");
	if (this->match({ TokenType::ARROW }))
	{
		operation_type = this->parse_type();
	}
	return std::make_shared<Expr::DeviceLoad>(device, logic_type, operation_type);
}

void Parser::error(const Token& error_token, const std::string& message)
{
	this->compiler.error(error_token.line, message + " (on token " + error_token.lexeme + ")");
	throw ParseError();
}

const Token& Parser::consume(TokenType type, const std::string& error_message)
{
	if (this->current_token_is(type))
	{
		return this->advance();
	}
	this->error(this->peek(), error_message);
	return Token(0, TokenType::T_EOF, "Unreachable");
}
