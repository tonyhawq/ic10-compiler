#include "SymbolTable.h"

SymbolTable::Index SymbolTable::Invalid = std::numeric_limits<SymbolTable::Index>::max();

SymbolUseNode::SymbolUseNode(std::shared_ptr<Expr>& expression, UseLocation location)
	: expression(expression.get()), statement(nullptr), location(location)
{}

SymbolUseNode::SymbolUseNode(std::unique_ptr<Stmt>& statement, UseLocation location)
	: expression(nullptr), statement(statement.get()), location(location)
{}

SymbolUseNode::SymbolUseNode(Expr* expression, UseLocation location)
	: expression(expression), statement(nullptr), location(location)
{}

SymbolUseNode::SymbolUseNode(Stmt* statement, UseLocation location)
	: expression(nullptr), statement(statement), location(location)
{}

bool SymbolUseNode::is_expression() const
{
	return this->expression;
}

bool SymbolUseNode::is_statement() const
{
	return this->statement;
}

Stmt& SymbolUseNode::stmt()
{
	if (!this->is_statement())
	{
		throw std::logic_error("SymbolUseNode does not refer to a statement");
	}
	return *this->statement;
}

Expr& SymbolUseNode::expr()
{
	if (!this->is_expression())
	{
		throw std::logic_error("SymbolUseNode does not refer to an expression");
	}
	return *this->expression;
}

const Stmt& SymbolUseNode::stmt() const
{
	if (!this->is_statement())
	{
		throw std::logic_error("SymbolUseNode does not refer to a statement");
	}
	return *this->statement;
}

const Expr& SymbolUseNode::expr() const
{
	if (!this->is_expression())
	{
		throw std::logic_error("SymbolUseNode does not refer to an expression");
	}
	return *this->expression;
}

void* SymbolUseNode::id()
{
	if (this->is_expression())
	{
		return (void*)&this->expr();
	}
	if (this->is_statement())
	{
		return (void*)&this->stmt();
	}
	throw std::logic_error("SymbolUseNode did not contain statement or expression");
}

UseLocation SymbolUseNode::loc() const
{
	return this->location;
}

Symbol::Symbol(const Variable* var)
	: begin(), end(), m_var(var)
{}

Symbol::Symbol(SymbolUseNode initalizer, const Variable* var)
	: begin(initalizer), end(), m_var(var)
{

}

const Variable& Symbol::var() const
{
	return *this->m_var;
}

SymbolUseNode Symbol::beginning()
{
	return this->begin.value();
}

SymbolUseNode Symbol::ending()
{
	return this->end.value();
}

void Symbol::set_end(SymbolUseNode end)
{
	this->end = end;
}

SymbolTable::SymbolTable()
{
}

SymbolTable::Index SymbolTable::create_symbol(SymbolUseNode inital, const Variable* var)
{
	this->symbols.push_back(Symbol(inital, var));
	Index i = this->symbols.size() - 1;
	this->name_to_symbol.emplace(inital.id(), i);
	return i;
}

void SymbolTable::alias_symbol(Index symbol, SymbolUseNode alias)
{
	this->name_to_symbol.emplace(alias.id(), symbol);
}

SymbolTable::Index SymbolTable::lookup_index(const void* ast_node)
{
	auto it = this->name_to_symbol.find(ast_node);
	if (it == this->name_to_symbol.end())
	{
		return this->Invalid;
	}
	return it->second;
}

SymbolTable::Index SymbolTable::lookup_index(const std::unique_ptr<Stmt>& ast_node)
{
	return this->lookup_index((const void*)ast_node.get());
}

SymbolTable::Index SymbolTable::lookup_index(const std::shared_ptr<Expr>& ast_node)
{
	return this->lookup_index((const void*)ast_node.get());
}

Symbol& SymbolTable::lookup(const void* ast_node)
{
	return this->lookup(this->lookup_index(ast_node));
}

Symbol& SymbolTable::lookup(const std::unique_ptr<Stmt>& ast_node)
{
	return this->lookup((const void*)ast_node.get());
}

Symbol& SymbolTable::lookup(const std::shared_ptr<Expr>& ast_node)
{
	return this->lookup((const void*)ast_node.get());
}

Symbol& SymbolTable::lookup(SymbolTable::Index index)
{
	if (index == this->Invalid)
	{
		throw std::logic_error("Attempted to get a non-existent symbol from an AST node");
	}
	return this->symbols[index];
}
