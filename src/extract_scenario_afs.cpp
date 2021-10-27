#include "commands.h"
#include "afs.h"
#include "tim2utils.h"
#include <thanatos/OBSld.h>
#include <cstring>
#include <string>
#include <cstdlib>
#include <cstddef>

void ExtractScenarioAFS(FILE* fp, int begin, AFSLdEntry* scenarioAfs, AFSLdEntry* scenarioTocEntry, char* outputPath)
{
	if (scenarioAfs && scenarioTocEntry)
	{
		int* scenarioTocInfo = (int*) AFSReadFileData(fp, scenarioTocEntry);
		int textureCount = scenarioTocInfo[0];
		int fileCount = scenarioTocInfo[1];
		int totalFileCount;
		size_t scenarioBegin = begin + scenarioAfs->offset;
		fseek(fp, scenarioBegin, SEEK_SET);

		AFSLdEntry* scenarioToc = AFSReadTOC(fp, &totalFileCount);
		if (scenarioToc)
		{
			char buffer[2048];
			snprintf(buffer, 2048, "%s/ScenarioFileList.txt", outputPath);
			FILE* scenarioFileList = fopen(buffer, "wb");
			if (scenarioFileList)
			{
				snprintf(buffer, 2048, "%s/ScenarioTextureList.txt", outputPath);
				FILE* scenarioTextureList = fopen(buffer, "wb");
				
				if (scenarioTextureList)
				{
					for (int i = 0; i < totalFileCount; i++)
					{
						bool isTexture = i < textureCount;
						if (isTexture)
						{
							char* pch = strstr(scenarioToc[i].name, ".sld");
							if (pch != NULL)
								strncpy(pch, ".png", 4);
							else
							{
								if (strlen(scenarioToc[i].name) <= (31-4))
								{
									strcat(scenarioToc[i].name, ".png");
								}
							}
						}
						int length = snprintf(buffer, 2048, "%s\n", scenarioToc[i].name);
						fwrite(buffer, length, 1, isTexture ? scenarioTextureList : scenarioFileList);
						snprintf(buffer, 2048, "%s/%s", outputPath, scenarioToc[i].name);

						printf("%s\n", scenarioToc[i].name);
						void* data = (void*) AFSReadFileData(fp, &scenarioToc[i]);
						if (isTexture)
						{
							uint32_t realSize = OBSld::decompress(data, scenarioToc[i].size, NULL, 0);
							void* realData = malloc(realSize);
							OBSld::decompress(data, scenarioToc[i].size, realData, realSize);
							OutbreakTm2ToPng(realData, buffer);
							free(realData);
						}
						else
						{
							FILE* ofp = fopen(buffer, "wb");
							if (ofp)
							{
								fwrite(data, scenarioToc[i].size, 1, ofp);
								fclose(ofp);
							}
							else
							{
								fprintf(stderr, "[ExtractScenarioAFS] Could not open %s for writing.\n", scenarioToc[i].name);
							}
						}
						free(data);
					}
					fclose(scenarioTextureList);
				}
				else
				{
					fprintf(stderr, "[ExtractScenarioAFS] Could not open ScenarioTextureList.txt for writing.\n");
				}
				fclose(scenarioFileList);
			}
			else
			{
				fprintf(stderr, "[ExtractScenarioAFS] Could not open ScenarioFileList.txt for writing.\n");
			}

			free(scenarioToc);
		}
		else
		{
			fprintf(stderr, "[ExtractScenarioAFS] Could not get the table of contents for scenario.\n");
		}
	}
	else
	{
		fprintf(stderr, "[ExtractScenarioAFS] Could not open the scenario AFS.\n");
	}
}

int CmdExtractScenarioAFS(int count, char** argv)
{
	if (count != 3)
	{
		fprintf(stderr, "[CmdExtractScenarioAFS] Incorrect amount of arguments\n");
		return 0;
	}

	char* netbio00Path = argv[0];
	int   scenarioId   = atoi(argv[1]);
	char* outputPath   = argv[2];

	FILE* fp = fopen(netbio00Path, "rb");

	size_t netbioBegin = 0;
	int netbioCount = 0;
	AFSLdEntry* netbioToc = AFSReadTOC(fp, &netbioCount);
	if (netbioToc)
	{
		char scenarioName[32];
		char scenarioTocName[32];
		snprintf(scenarioName, 32, "r%03d.afs", scenarioId);
		snprintf(scenarioTocName, 32, "r%03d_h.bin", scenarioId);

		AFSLdEntry* scenarioAfs = AFSFindFile(netbioToc, netbioCount, scenarioName);
		AFSLdEntry* scenarioTocEntry = AFSFindFile(netbioToc, netbioCount, scenarioTocName);

		ExtractScenarioAFS(fp, netbioBegin, scenarioAfs, scenarioTocEntry, outputPath);

		free(netbioToc);
	}
	else
	{
		fprintf(stderr, "[CmdExtractScenarioAFS] Could not get the table of contents for NETBIO00.DAT\n");
	}
	return 1;
}