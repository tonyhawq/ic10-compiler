#pragma once

#include <memory>
#include <string>
#include <vector>

#include "Token.h"

#define UNDEFINED_TYPE TypeName("undefined")
#define VOID_TYPE TypeName("void")

struct TypeName
{
	explicit TypeName(const std::string& type_name) :m_type_name(std::make_unique<std::string>(type_name)), constant(false), pointer(false), compile_time(false) {};
	TypeName(bool constant, const std::string& type_name) :m_type_name(std::make_unique<std::string>(type_name)), constant(constant), pointer(false), compile_time(false) {};
	static TypeName create_compile_time(const std::string& type_name);
	TypeName(const TypeName& other);
	TypeName& operator=(const TypeName& other);
	TypeName(TypeName&& other) noexcept = default;
	TypeName& operator=(TypeName&& other) noexcept = default;

	void make_pointer_type(bool constant, bool nullable);

	TypeName underlying() const;
	std::string type_name() const;
	bool nullable() const;

	int height() const;

	bool const_unqualified_equals(const TypeName& other) const;
	bool operator==(const TypeName& other) const;
	bool operator!=(const TypeName& other) const;

	bool compile_time;
	bool constant;
	bool pointer;
	std::unique_ptr<bool> m_nullable;
	std::unique_ptr<TypeName> m_pointed_to_type;
	std::unique_ptr<std::string> m_type_name;
private:
	TypeName() : constant(false), compile_time(false), pointer(false), m_pointed_to_type(nullptr), m_type_name(nullptr) {};
};

namespace std {
	template <>
	struct hash<TypeName> {
		std::size_t operator()(const TypeName& key) const {
			return std::hash<std::string>()(key.type_name());
		}
	};
}

struct FunctionParam
{
	TypeName type;
	std::string name;
};

enum class FunctionSource
{
	User,
	Builtin,
};

struct FunctionTypeID
{
	FunctionTypeID(const TypeName& return_type, const std::string& mangled_name, const std::string& unmangled_name, const std::vector<FunctionParam> arguments);
	FunctionTypeID(const TypeName& return_type, const std::string& mangled_name, const std::string& unmangled_name, const std::vector<FunctionParam> arguments, FunctionSource source);
	TypeName return_type;
	std::string mangled_name;
	std::string unmangled_name;
	std::vector<FunctionParam> arguments;
	FunctionSource source;
};

struct TypeID
{
	explicit TypeID(const TypeName& type);
	TypeID(const TypeName& type, const TypeName& return_type, const std::string& m_unmangled_name, const std::vector<FunctionParam>& m_arguments);
	explicit TypeID(const TypeID& other);

	TypeName type;
	TypeName& return_type();
	std::string& mangled_name();
	std::string& unmangled_name();
	std::vector<FunctionParam>& arguments();
	const TypeName& return_type() const;
	const std::string& mangled_name() const;
	const std::string& unmangled_name() const;
	const std::vector<FunctionParam>& arguments() const;
	Literal fixed_value;

	bool is_function = false;
	std::unique_ptr<FunctionTypeID> function_type;
};

namespace types
{
	std::string get_mangled_function_name(const std::string& name, const std::vector<FunctionParam> params, const TypeName& return_type);
	TypeName get_function_signature(const std::vector<FunctionParam>& params, const TypeName& return_type);
}
