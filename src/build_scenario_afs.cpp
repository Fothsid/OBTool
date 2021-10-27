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

void* __AFSBuildScenarioTexture(AFSTOCEntry* out, char* name, int id)
{
	*out = {0};
	printf("%s\n", name);
	std::filesystem::path path(name);
	std::string output = path.filename().string();
	strncpy(&out->name[0], output.c_str(), 32);
	if (output.size() >= 32)
		out->name[31] = 0;

	char* pch = strstr(out->name, ".png");
	if (pch != NULL)
		strncpy(pch, ".sld", 4);
	else
	{
		if (strlen(out->name) <= (31-4))
		{
			strcat(out->name, ".sld");
		}
	}

	int x, y;
	uint32_t tim2Size;
	void* tim2Data = OutbreakPngToTm2(name, 0, &x, &y, &tim2Size);

	void* data = malloc(tim2Size * 3);
	if (!data)
	{
		fprintf(stderr, "Could not allocate memory for texture compression\n");
		return NULL;
	}
	out->fileSize = OBSld::compress(tim2Data, tim2Size, data, tim2Size * 3);
	return data;
}

void* __AFSBuildScenarioFile(AFSTOCEntry* out, char* name, int id)
{
	*out = {0};
	printf("%s\n", name);
	std::filesystem::path path(name);
	std::string output = path.filename().string();
	strncpy(&out->name[0], output.c_str(), 32);
	if (output.size() >= 32)
		out->name[31] = 0;

	FILE* fp = fopen(name, "rb");
	if (!fp)
	{
		printf("Could not open file \"%s\"\n", name);
		return NULL;
	}
	fseek(fp, 0, SEEK_END);
	out->fileSize = (uint32_t) ftell(fp);
	fseek(fp, 0, SEEK_SET);
	void* data = malloc(out->fileSize);
	fread(data, out->fileSize, 1, fp);
	fclose(fp);

	return data;
}

int FillFileList(std::vector<std::string>& list, const char* path)
{
	std::ifstream t(path);
	if (!t.fail())
	{
		std::stringstream buffer;
		buffer << t.rdbuf();
		for (std::string line; std::getline(buffer, line);)
		{
			if (line[0] == '\n' || line[0] == '\r')
				continue;
			line.erase(std::remove(line.begin(), line.end(), '\n'), line.end());
			line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
			list.push_back(line);
		}
	}
	else
	{
		printf("%s is not found.\n", path);
		return 0;
	}
	return 1;
}

int BuildScenarioAFS(FILE* afs, FILE* toc, char* folder)
{
	std::vector<std::string> textureNames;
	std::vector<std::string> fileNames;

	char buffer[2048];
	snprintf(buffer, 2048, "%s/ScenarioTextureList.txt", folder);
	if (!FillFileList(textureNames, buffer))
		return 0;

	snprintf(buffer, 2048, "%s/ScenarioFileList.txt", folder);
	if (!FillFileList(fileNames, buffer))
		return 0;

	std::vector<AFSBuildEntry> afsEntries;
	for (int i = 0; i < textureNames.size(); i++)
	{
		textureNames[i] = std::string(folder) + "/" + textureNames[i];
		AFSBuildEntry entry = {(const char*) textureNames[i].c_str(), __AFSBuildScenarioTexture, free};
		afsEntries.push_back(entry);
	}

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

	if (toc)
	{
		uint32_t texCount = textureNames.size();
		uint32_t fileCount = fileNames.size();
		fwrite(&texCount, 4, 1, toc);
		fwrite(&fileCount, 4, 1, toc);

		for (int i = 0; i < fileCount; i++)
		{
			char nameBuffer[16];
			memset(nameBuffer, 0, 16);

			std::filesystem::path path(fileNames[i]);
			std::string output = path.filename().string();
			strncpy(nameBuffer, output.c_str(), 16);
			for (int j = 0; j < 16; j++)
			{
				if (nameBuffer[j])
					nameBuffer[j] = toupper(nameBuffer[j]);
				if (nameBuffer[j] == '.')
					nameBuffer[j] = '_';
			}

			fwrite(nameBuffer, 16, 1, toc);
		}
	}

	return 1;
}

int CmdBuildScenarioAFS(int count, char** argv)
{
	if (count < 2 || count > 3)
	{
		fprintf(stderr, "[BuildScenarioAFS] Incorrect amount of arguments\n");
		return 0;
	}

	char* in_folder = argv[0];
	char* out_afs   = argv[1];
	char* out_toc   = argv[2];

	FILE* afs = fopen(out_afs, "wb");
	FILE* toc = fopen(out_toc, "wb");
	BuildScenarioAFS(afs, toc, in_folder);
	fclose(toc);
	fclose(afs);

	return 1;
}