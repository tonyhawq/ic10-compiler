#pragma once

#include "TypeChecker.h"

typedef std::shared_ptr<int> RegisterHandle;

struct StackVariable
{
	StackVariable(const std::string& name, int size) :name(name), size(size), offset(0), is_static(false), id(0) {};
	std::string name;
	int offset;
	int size;
	bool is_static;
	size_t id;
};

class StackEnvironment
{
public:
	explicit StackEnvironment();
	explicit StackEnvironment(StackEnvironment* parent);
	~StackEnvironment();
	StackEnvironment(StackEnvironment&& other) noexcept = default;
	StackEnvironment(const StackEnvironment& other) = delete;
	StackEnvironment& operator=(const StackEnvironment& other) = delete;
	StackEnvironment& operator=(StackEnvironment&& other)  noexcept;

	void set_frame_size(int value);
	int frame_size() const;

	void forget(const std::string& name);
	StackVariable& define(const std::string& name, int size);
	StackVariable& define_static(const std::string& name, int size);
	std::unique_ptr<StackVariable> resolve(const std::string& name);

	bool is_in_function() const;
	const std::string& function_name() const;

	const std::vector<StackVariable*>& see_variables() const;

	StackEnvironment* pop_to_function();
	StackEnvironment* spawn();
	StackEnvironment* spawn_in_function(const std::string& name);
	StackEnvironment* pop();

	StackEnvironment* child;
	StackEnvironment* parent;
private:
	std::unique_ptr<std::string> m_function_name;
	std::unordered_map<std::string, StackVariable> variables;
	std::vector<StackVariable*> variable_list;
	int m_frame_size;
};

class Register
{
public:
	explicit Register(RegisterHandle handle);
	~Register();
	Register(const Register& other);
	Register(Register&& other) noexcept;

	Register& operator=(Register& other) = default;
	Register& operator=(Register&& other) = default;

	RegisterHandle* release();
	RegisterHandle handle();
	int index() const;

	std::string to_string() const;
private:
	RegisterHandle register_handle;
};

class RegisterOrLiteral
{
public:
	RegisterOrLiteral(const Register& reg);
	RegisterOrLiteral(const Literal& literal);
	RegisterOrLiteral(const RegisterOrLiteral& other) = delete;
	RegisterOrLiteral(RegisterOrLiteral&& other) noexcept;

	bool is_register() const;
	bool is_literal() const;

	Register& get_register();
	Literal& get_literal();
	const Register& get_register() const;
	const Literal& get_literal() const;

	std::string to_string() const;
private:
	std::unique_ptr<Register> m_reg;
	std::unique_ptr<Literal> m_literal;
};

class RegisterAllocator
{
public:
	RegisterAllocator(size_t register_count);
	~RegisterAllocator();
	Register allocate();

	size_t register_count() const;
	bool is_register_in_use(size_t id) const;
	std::vector<Register> registers_in_use() const;
private:
	std::vector<RegisterHandle> registers;
};

struct Placeholder
{
	int id;
};

class CodeGenerator : public Expr::Visitor, public Stmt::Visitor
{
public:
	enum class Pass
	{
		GlobalLinkage,
		FunctionLinkage,
	};

	CodeGenerator(Compiler& compiler, TypeCheckedProgram& program);
	void error(const Token& token, const std::string& str);

	std::string generate();

	Register get_or_make_output_register(const RegisterOrLiteral& a, const RegisterOrLiteral& b);

	std::unique_ptr<RegisterOrLiteral> visit_expr_raw(std::shared_ptr<Expr> expression);
	std::unique_ptr<RegisterOrLiteral> visit_expr(std::shared_ptr<Expr> expression);

	void visit_stmt(std::unique_ptr<Stmt>& stmt);

	virtual void* visitExprBinary(Expr::Binary& expr) override;
	virtual void* visitExprGrouping(Expr::Grouping& expr) override;
	virtual void* visitExprLiteral(Expr::Literal& expr) override;
	virtual void* visitExprUnary(Expr::Unary& expr) override;
	virtual void* visitExprVariable(Expr::Variable& expr) override;
	virtual void* visitExprAssignment(Expr::Assignment& expr) override;
	virtual void* visitExprCall(Expr::Call& expr) override;
	virtual void* visitExprLogical(Expr::Logical& expr) override;
	virtual void* visitExprDeviceLoad(Expr::DeviceLoad& expr) override;

	virtual void* visitStmtExpression(Stmt::Expression& expr) override;
	virtual void* visitStmtAsm(Stmt::Asm& expr) override;
	virtual void* visitStmtPrint(Stmt::Print& expr) override;
	virtual void* visitStmtVariable(Stmt::Variable& expr) override;
	virtual void* visitStmtBlock(Stmt::Block& expr) override;
	virtual void* visitStmtIf(Stmt::If& expr) override;
	virtual void* visitStmtFunction(Stmt::Function& expr) override;
	virtual void* visitStmtReturn(Stmt::Return& expr) override;
	virtual void* visitStmtWhile(Stmt::While& expr) override;
	virtual void* visitStmtStatic(Stmt::Static& expr) override;
	virtual void* visitStmtDeviceSet(Stmt::DeviceSet& expr) override;
private:
	void store_register_values();
	void restore_register_values();

	std::vector<Register> stored_registers;

	std::string get_register_name(const Register& reg);

	void comment(const std::string& val);
	void emit_raw(const std::string& val);
	void emit_register_use(const Register& reg);
	void emit_register_use(const Register& a, const Register& b);
	void emit_peek_stack_from_into(Register* reg);
	void emit_store_into(int offset, const RegisterOrLiteral& source);
	void emit_load_into(int offset, const std::string& register_label);
	void emit_load_into(int offset, Register* reg);

	Placeholder emit_placeholder();
	void emit_replace_placeholder(const Placeholder& placeholder, const std::string& val);

	int current_placeholder_value;
	int current_line();

	void push_env();
	void push_env(const std::string& name);
	void pop_env();

	Pass pass = Pass::GlobalLinkage;
	StackEnvironment* env;
	StackEnvironment top_env;
	RegisterAllocator allocator;
	std::string code;
	Compiler& compiler;
	TypeCheckedProgram& m_program;
};

