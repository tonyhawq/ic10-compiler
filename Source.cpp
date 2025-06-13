#include <stdio.h>

#include "src/Compiler.h"

int main(int argc, char* argv[])
{
	Compiler compiler;
	if (argc <= 1)
	{
		compiler.compile("test.txt");
		return 0;
	}
	char* arg = argv[1];
	compiler.compile(arg);
	return 0;
}