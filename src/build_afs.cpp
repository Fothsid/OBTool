#include "commands.h"
#include "afs.h"
#include "tim2utils.h"
#include <thanatos/OBSld.h>
#include <filesystem>
#include <cstring>
#include <string>
#include <sstream>
#include <fstream>
#include <cstdlib>
#include <cstddef>
#include <vector>

void* __AFSBuildScenarioFile(AFSTOCEntry* out, const char* name, int id);
int FillFileList(std::vector<std::string>& list, const char* path);

int BuildAFS(FILE* afs, char* folder)
{
	std::vector<std::string> fileNames;

	char buffer[2048];

	snprintf(buffer, 2048, "%s/AFSFileList.txt", folder);
	if (!FillFileList(fileNames, buffer))
		return 0;

	std::vector<AFSBuildEntry> afsEntries;

	for (int i = 0; i < fileNames.size(); i++)
	{
		fileNames[i] = std::string(folder) + "/" + fileNames[i];
		AFSBuildEntry entry = {(const char*) fileNames[i].c_str(), __AFSBuildScenarioFile, free};
		afsEntries.push_back(entry);
	}

	if (afs)
	{
		if (!AFSCreate(afs, &afsEntries[0], afsEntries.size()))
		{
			printf("AFS creation failed.\n");
		}
	}

	return 1;
}

int CmdBuildAFS(int count, char** argv)
{
	if (count != 2)
	{
		fprintf(stderr, "[BuildAFS] Incorrect amount of arguments\n");
		return 0;
	}

	char* in_folder = argv[0];
	char* out_afs   = argv[1];

	FILE* afs = fopen(out_afs, "wb");
	int r = BuildAFS(afs, in_folder);
	fclose(afs);
	return r;
}