#include "afs.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

AFSLdEntry* AFSReadTOC(FILE* fp, int* count)
{
	if (!fp)
		return NULL;

	size_t begin = ftell(fp);

	AFSHeader header;
	fread(&header, sizeof(AFSHeader), 1, fp);

	if (header.fourcc != 0x534641)
		return NULL;

	if (count)
		*count = header.fileCount;

	AFSLdEntry* result = (AFSLdEntry*) malloc(sizeof(AFSLdEntry) * header.fileCount);
	if (!result)
		return NULL;

	memset(result, 0, sizeof(AFSLdEntry) * header.fileCount);

	size_t tocPos = begin + sizeof(AFSHeader) + (sizeof(AFSPair) * header.fileCount);
	fseek(fp, tocPos, SEEK_SET);

	AFSPair tocPair;
	fread(&tocPair, sizeof(AFSPair), 1, fp);

	if (tocPair.offset > 0 && tocPair.size > 0)
	{
		fseek(fp, begin + tocPair.offset, SEEK_SET);
		for (int i = 0; i < header.fileCount; i++)
			fread(&result[i].name[0], sizeof(AFSTOCEntry), 1, fp);
	}
	else
	{
		for (int i = 0; i < header.fileCount; i++)
			snprintf(&result[i].name[0], 32, "idx%04d.bin", i);
	}

	fseek(fp, begin + sizeof(AFSHeader), SEEK_SET);
	for (int i = 0; i < header.fileCount; i++)
	{
		AFSPair pair;
		fread(&pair, sizeof(AFSPair), 1, fp);
		result[i].offset = pair.offset;
		result[i].size = pair.size;
	}

	fseek(fp, begin, SEEK_SET);
	return result;
}

AFSLdEntry* AFSFindFile(AFSLdEntry* entries, int count, const char* name)
{
	for (int i = 0; i < count; i++)
	{
		printf("%s\n",entries[i].name);
		if (strcmp(entries[i].name, name) == 0)
			return &entries[i];
	}
	return NULL;
}

void* AFSReadFileData(FILE* fp, AFSLdEntry* entry)
{
	if (!fp)
		return NULL;

	size_t begin = ftell(fp);

	void* result = malloc(entry->size);
	if (!result)
		return NULL;

	fseek(fp, begin + entry->offset, SEEK_SET);
	fread(result, entry->size, 1, fp);

	fseek(fp, begin, SEEK_SET);
	return result;
}

#define AlignTo2048(X) (((X) + 0x7FF) & ~0x7FF)

int AFSCreate(FILE* fp, AFSBuildEntry* entries, int count)
{
	if (!fp)
		return 0;

	size_t begin = ftell(fp);

	AFSHeader header;
	header.fourcc = 0x534641;
	header.fileCount = count;

	fwrite(&header, sizeof(AFSHeader), 1, fp);
	size_t pairBegin = ftell(fp);
	size_t fullHeaderSize = sizeof(AFSHeader) + ((count + 1) * sizeof(AFSPair));
	size_t zeroesToWrite = AlignTo2048(fullHeaderSize) - fullHeaderSize;
	for (int i = 0; i < zeroesToWrite; i++)
	{
		uint8_t zero = 0;
		fwrite(&zero, 1, 1, fp);
	}

	size_t tocSize = sizeof(AFSTOCEntry) * count;
	AFSTOCEntry* tocBuffer = (AFSTOCEntry*) malloc(tocSize);

	for (int i = 0; i < count; i++)
	{
		size_t currentOffset = ftell(fp);
		AFSBuildEntry* e = &entries[i];
		AFSTOCEntry* t = &tocBuffer[i];
		void* data = e->readData(t, e->name, i);

		fwrite(data, t->fileSize, 1, fp);
		zeroesToWrite = AlignTo2048(t->fileSize) - t->fileSize;
		for (int i = 0; i < zeroesToWrite; i++)
		{
			uint8_t zero = 0;
			fwrite(&zero, 1, 1, fp);
		}

		size_t endOffset = ftell(fp);
		fseek(fp, pairBegin + (i * sizeof(AFSPair)), SEEK_SET);
		AFSPair pair = {currentOffset - begin, t->fileSize};
		fwrite(&pair, sizeof(AFSPair), 1, fp);
		fseek(fp, endOffset, SEEK_SET);
		e->free(data);
	}

	size_t lastDataOffset = ftell(fp);
	fseek(fp, pairBegin + (count * sizeof(AFSPair)), SEEK_SET);
	AFSPair pair = {lastDataOffset - begin, tocSize};
	fwrite(&pair, sizeof(AFSPair), 1, fp);
	fseek(fp, lastDataOffset, SEEK_SET);
	fwrite(tocBuffer, tocSize, 1, fp);
	zeroesToWrite = AlignTo2048(tocSize) - tocSize;
	for (int i = 0; i < zeroesToWrite; i++)
	{
		uint8_t zero = 0;
		fwrite(&zero, 1, 1, fp);
	}

	free(tocBuffer);
	return 1;
}