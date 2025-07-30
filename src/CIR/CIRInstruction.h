#pragma once

#include <string>
#include <vector>
#include <memory>
#include <optional>

#include "../strutil.h"
#include "CIRRegister.h"

namespace cir
{

	struct Instruction
	{
	public:
		typedef std::unique_ptr<Instruction> ref;

		static Instruction::ref from(Instruction* instruction);

		virtual ~Instruction() = default;
		virtual std::string to_string() const = 0;
		virtual std::vector<Temporary> inputs() const = 0;
		virtual std::vector<Temporary> outputs() const = 0;

		struct Move;
		struct MoveImm;
		struct Print;
		struct Add;
		struct Sub;
		struct Div;
		struct Mul;
		struct Call;
		struct Return;
	private:
	};

	struct Instruction::Return : public Instruction
	{
		Return(std::optional<Temporary> val) :val(val) {};
		virtual std::string to_string() const override { return std::string("return ") + (this->val.has_value() ? this->val.value().to_string() : std::string("void")); }
		virtual std::vector<Temporary> inputs() const override { if (this->val.has_value()) { return { this->val.value() }; } return {}; };
		virtual std::vector<Temporary> outputs() const override { return { }; };
		
		std::optional<Temporary> val;
	};

	struct Instruction::Call : public Instruction
	{
		Call(const std::string& func, Temporary into, const std::vector<Temporary>& args) :function(func), into(into), args(args) {};
		virtual std::string to_string() const override { return std::string("call ") + this->into.to_string() + " " + this->function + "[TODO]"; }
		virtual std::vector<Temporary> inputs() const override { return this->args; };
		virtual std::vector<Temporary> outputs() const override { return { this->into }; };

		std::string function;
		Temporary into;
		std::vector<Temporary> args;
	};

	struct Instruction::Move : public Instruction
	{
		Move(Temporary into, Temporary from) :into(into), from(from) {};
		virtual std::string to_string() const override { return std::string("move ") + this->into.to_string() + " " + this->from.to_string(); }
		virtual std::vector<Temporary> inputs() const override { return { this->from }; };
		virtual std::vector<Temporary> outputs() const override { return { this->into }; };

		Temporary into;
		Temporary from;
	};

	struct Instruction::MoveImm : public Instruction
	{
		MoveImm(Temporary into, Number from) :into(into), from(from) {};
		virtual std::string to_string() const override { return std::string("moveimm ") + this->into.to_string() + " " + this->from.to_string(); }
		virtual std::vector<Temporary> inputs() const override { return { }; };
		virtual std::vector<Temporary> outputs() const override { return { this->into }; };
		Temporary into;
		Number from;
	};

	struct Instruction::Print : public Instruction
	{
		Print(Temporary from) :from(from) {};
		virtual std::string to_string() const override { return std::string("print ") + this->from.to_string(); }
		virtual std::vector<Temporary> inputs() const override { return { this->from }; };
		virtual std::vector<Temporary> outputs() const override { return { }; };
		Temporary from;
	};

	struct Instruction::Add : public Instruction
	{
		Add(Temporary into, Temporary a, Temporary b) :into(into), a(a), b(b) {};
		virtual std::string to_string() const override { return std::string("add ") + this->into.to_string() + " " + this->a.to_string() + " " + this->b.to_string(); }
		virtual std::vector<Temporary> inputs() const override { return { this->a, this->b }; };
		virtual std::vector<Temporary> outputs() const override { return { this->into }; };
		Temporary into;
		Temporary a;
		Temporary b;
	};

	struct Instruction::Sub : public Instruction
	{
		Sub(Temporary into, Temporary a, Temporary b) :into(into), a(a), b(b) {};
		virtual std::string to_string() const override { return std::string("sub ") + this->into.to_string() + " " + this->a.to_string() + " " + this->b.to_string(); }
		virtual std::vector<Temporary> inputs() const override { return { this->a, this->b }; };
		virtual std::vector<Temporary> outputs() const override { return { this->into }; };
		Temporary into;
		Temporary a;
		Temporary b;
	};

	struct Instruction::Mul : public Instruction
	{
		Mul(Temporary into, Temporary a, Temporary b) :into(into), a(a), b(b) {};
		virtual std::string to_string() const override { return std::string("mul ") + this->into.to_string() + " " + this->a.to_string() + " " + this->b.to_string(); }
		virtual std::vector<Temporary> inputs() const override { return { this->a, this->b }; };
		virtual std::vector<Temporary> outputs() const override { return { this->into }; };
		Temporary into;
		Temporary a;
		Temporary b;
	};

	struct Instruction::Div : public Instruction
	{
		Div(Temporary into, Temporary a, Temporary b) :into(into), a(a), b(b) {};
		virtual std::string to_string() const override { return std::string("div ") + this->into.to_string() + " " + this->a.to_string() + " " + this->b.to_string(); }
		virtual std::vector<Temporary> inputs() const override { return { this->a, this->b }; };
		virtual std::vector<Temporary> outputs() const override { return { this->into }; };
		Temporary into;
		Temporary a;
		Temporary b;
	};
}