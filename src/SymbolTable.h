#pragma once

#include <unordered_map>
#include <string>
#include <memory>
#include <stdexcept>
#include <optional>

#include "Typing.h"

struct Expr;
struct Stmt;

enum class UseLocation
{
	During,
	BeforeStatement,
	AfterStatement,
};

class SymbolUseNode
{
public:
	SymbolUseNode(std::shared_ptr<Expr>& expression, UseLocation location);
	SymbolUseNode(std::unique_ptr<Stmt>& statement, UseLocation location);
	SymbolUseNode(Expr* expression, UseLocation location);
	SymbolUseNode(Stmt* statement, UseLocation location);

	bool is_expression() const;
	bool is_statement() const;

	Stmt& stmt();
	Expr& expr();
	const Stmt& stmt() const;
	const Expr& expr() const;
	UseLocation loc() const;

	void* id();
private:
	UseLocation location;
	Stmt* statement;
	Expr* expression;
};

class Symbol
{
public:
	explicit Symbol();
	explicit Symbol(SymbolUseNode initalizer);

	SymbolUseNode beginning();
	SymbolUseNode ending();

	void set_end(SymbolUseNode end);
	TypeID type;
private:
	std::optional<SymbolUseNode> begin;
	std::optional<SymbolUseNode> end;
};

class SymbolTable
{
public:
	typedef size_t Index;
	static Index Invalid;

	SymbolTable();
	
	Index create_symbol(SymbolUseNode inital);
	void alias_symbol(Index symbol, SymbolUseNode alias);
	Index lookup_index(const void* ast_node);
	Index lookup_index(const std::unique_ptr<Stmt>& ast_node);
	Index lookup_index(const std::shared_ptr<Expr>& ast_node);
	Symbol& lookup(const void* ast_node);
	Symbol& lookup(const std::unique_ptr<Stmt>& ast_node);
	Symbol& lookup(const std::shared_ptr<Expr>& ast_node);
	Symbol& lookup(SymbolTable::Index index);
private:
	std::vector<Symbol> symbols;
	std::unordered_map<const void*, Index> name_to_symbol;
};

