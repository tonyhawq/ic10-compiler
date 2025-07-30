#pragma once

#include <string>
#include <vector>

namespace string
{
	inline std::string join(const std::vector<std::string>& in, const std::string& joiner)
	{
		if (in.empty())
		{
			return "";
		}
		std::string buf;
		size_t size = joiner.size() * (in.size() - 1);
		for (const auto& str : in)
		{
			size += str.size();
		}
		buf.reserve(size);
		for (size_t i = 0; i < in.size(); i++)
		{
			buf += in[i];
			if (i < in.size() - 1)
			{
				buf += joiner;
			}
		}
		return buf;
	}
}
