#include "commands.h"
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>

#define AlignTo32(X) (((X) + 0x1F) & ~0x1F)
void AlignFileTo32(FILE* fp)
{
	size_t pos = ftell(fp);
	uint32_t zeroesToWrite = AlignTo32(pos) - pos;
	for (int i = 0; i < zeroesToWrite; i++)
	{
		uint8_t zero = 0;
		fwrite(&zero, 1, 1, fp);
	}
}

struct MOMOPair
{
	uint32_t offset;
	uint32_t size;
};

int BuildMOMO(FILE* fp, int fileCount, char** files)
{
	size_t begin = ftell(fp);
	uint32_t fourcc = 0x4F4D4F4D;
	uint32_t count = (uint32_t) fileCount;
	fwrite(&fourcc, sizeof(uint32_t), 1, fp);
	fwrite(&count, sizeof(uint32_t), 1, fp);

	size_t pairsBegin = ftell(fp);
	size_t pairsSize = sizeof(MOMOPair) * fileCount;
	MOMOPair* pairs = (MOMOPair*) malloc(sizeof(MOMOPair) * fileCount);
	memset(pairs, 0, pairsSize);
	fwrite(pairs, pairsSize, 1, fp);
	AlignFileTo32(fp);

	for (int i = 0; i < fileCount; i++)
	{
		size_t currentOffset = ftell(fp);
		pairs[i].offset = currentOffset - begin;
		
		FILE* ifp = fopen(files[i], "rb");
		if (!ifp)
		{
			printf("Could not open file \"%s\"\n", files[i]);
			return 0;
		}
		fseek(ifp, 0, SEEK_END);
		pairs[i].size = (int32_t) ftell(ifp);
		fseek(ifp, 0, SEEK_SET);
		if (pairs[i].size)
		{
			void* data = malloc(pairs[i].size);
			fread(data, pairs[i].size, 1, ifp);
			fwrite(data, pairs[i].size, 1, fp);
			free(data);
		}
		fclose(ifp);
		AlignFileTo32(fp);

	}
	AlignFileTo32(fp);
	return 1;
}

int CmdBuildMOMO(int count, char** argv)
{
	if (count < 2)
	{
		fprintf(stderr, "[BuildMOMO] Incorrect amount of arguments\n");
		return 0;
	}

	FILE* fp = fopen(argv[0], "wb");
	if (!fp)
	{
		fprintf(stderr, "[BuildMOMO] Could not open \"%s\" for writing.\n", argv[0]);
		return 0;
	}

	int fileCount = count - 1;
	int r = BuildMOMO(fp, fileCount, &argv[1]);
	fclose(fp);
	return r;
}