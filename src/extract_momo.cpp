#include "commands.h"
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>

struct MOMOPair
{
	uint32_t offset;
	uint32_t size;
};

int ExtractMOMO(FILE* fp, char* baseName)
{
	size_t begin = ftell(fp);
	uint32_t fourcc;;
	uint32_t count;
	fread(&fourcc, sizeof(uint32_t), 1, fp);
	fread(&count, sizeof(uint32_t), 1, fp);

	if (fourcc != 0x4F4D4F4D)
	{
		fprintf(stderr, "[ExtractMOMO] Not a MOMO file.\n");
		return 0;
	}

	size_t pairsSize = sizeof(MOMOPair) * count;
	MOMOPair* pairs = (MOMOPair*) malloc(pairsSize);
	memset(pairs, 0, pairsSize);
	fread(pairs, pairsSize, 1, fp);
	
	for (int i = 0; i < count; i++)
	{
		char buffer[1024];
		snprintf(buffer, 1024, "%s.%d.bin", baseName, i);

		fseek(fp, begin + pairs[i].offset, SEEK_SET);
		FILE* output = fopen(buffer, "wb");
		if (pairs[i].size > 0)
		{
			void* data = malloc(pairs[i].size);
			fread(data, pairs[i].size, 1, fp);
			fwrite(data, pairs[i].size, 1, output);
			free(data);
		}
		fclose(output);
	}
	free(pairs);
	fseek(fp, begin, SEEK_SET);
	return 1;
}

int CmdExtractMOMO(int count, char** argv)
{
	if (count != 2)
	{
		fprintf(stderr, "[ExtractMOMO] Incorrect amount of arguments\n");
		return 0;
	}

	FILE* fp = fopen(argv[0], "rb");
	if (!fp)
	{
		fprintf(stderr, "[ExtractMOMO] Could not open \"%s\" for writing.\n", argv[0]);
		return 0;
	}
	int r = ExtractMOMO(fp, argv[1]);
	fclose(fp);
	return r;
}