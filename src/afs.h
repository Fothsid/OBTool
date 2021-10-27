#pragma once
#include <stdint.h>
#include <stdio.h>

typedef struct
{
	uint32_t fourcc;
	int32_t fileCount;
} AFSHeader;

typedef struct
{
	uint16_t year, month, day, hour, minute, second;
} AFSDate;

typedef struct
{
	uint32_t offset;
	uint32_t size;
} AFSPair;

typedef struct
{
	char name[32];
	AFSDate date;
	uint32_t fileSize;
} AFSTOCEntry;

typedef struct
{
	char name[32];
	AFSDate date;
	uint32_t size;
	uint32_t offset;
} AFSLdEntry;

#ifdef __cplusplus
extern "C" {
#endif

AFSLdEntry* AFSReadTOC(FILE* fp, int* count);
AFSLdEntry* AFSFindFile(AFSLdEntry* entries, int count, const char* name);
void* AFSReadFileData(FILE* fp, AFSLdEntry* entry);

#ifdef __cplusplus
}
#endif