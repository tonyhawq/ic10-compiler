#include <stdio.h>

#include "src/Compiler.h"

int main()
{
	Compiler compiler;
	compiler.compile("test.txt");
	return 0;
}