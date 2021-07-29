#include <iostream>
#include <cstdio>
#include <cstring>

#include "commands.h"

void PrintUsage(char* argv0)
{
	printf("Usage: %s [command] <command inputs...>\n"
		   "Commands:\n", argv0);
	for (int i = 0; i < CommandCount; i++)
	{
		printf("\t%s ", CommandList[i].name);
		for (int j = 0; j < CommandList[i].inputCount; j++)
			printf("<%s> ", CommandList[i].inputs[j].name);
		printf("- %s\n", CommandList[i].description);
	}
}

int main(int argc, char* argv[])
{
	if (argc < 3)
	{
		PrintUsage(argv[0]);
		return 0;
	}
	
	bool found = false;
	for (int i = 0; i < CommandCount; i++)
	{
		if (strcmp(argv[1], CommandList[i].name) == 0)
		{
			CommandList[i].function(argc-2, &argv[2]);	
			found = true;
		}
	}
	if (!found)
		PrintUsage(argv[0]);

	return 0;
}