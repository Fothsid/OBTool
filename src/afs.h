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
	int32_t size;
} AFSPair;

typedef struct
{
	char name[32];
	AFSDate date;
	int32_t fileSize;
} AFSTOCEntry;

typedef struct
{
	char name[32];
	AFSDate date;
	int32_t size;
	uint32_t offset;
} AFSLdEntry;

typedef struct
{
	const char* name;
	void* (*readData)(AFSTOCEntry* out, const char* name, int id);
	void  (*free)(void*);
} AFSBuildEntry;

#ifdef __cplusplus
extern "C" {
#endif

AFSLdEntry* AFSReadTOC(FILE* fp, int* count);
AFSLdEntry* AFSFindFile(AFSLdEntry* entries, int count, const char* name);
void* AFSReadFileData(FILE* fp, AFSLdEntry* entry);
int AFSCreate(FILE* fp, AFSBuildEntry* entries, int count);

#ifdef __cplusplus
}
#endif