#pragma once
#include <string>

namespace cir
{
	class Temporary
	{
	public:
		Temporary(size_t i);

		std::string to_string() const;
		size_t num() const;
	private:
		size_t i;
	};

	class Number
	{
	public:
		Number(double val);

		std::string to_string() const;
		double value() const;
	private:
		double val;
	};
}

