#include "commands.h"
#include "afs.h"
#include "tim2utils.h"
#include <cstring>
#include <string>
#include <cstdlib>
#include <cstddef>

int ExtractAFS(FILE* fp, char* outputPath)
{
	int totalFileCount;
	int result = 1;

	AFSLdEntry* afsToc = AFSReadTOC(fp, &totalFileCount);
	if (afsToc)
	{
		char buffer[2048];
		snprintf(buffer, 2048, "%s/AFSFileList.txt", outputPath);
		FILE* afsFileList = fopen(buffer, "wb");
		if (afsFileList)
		{
			for (int i = 0; i < totalFileCount; i++)
			{
				int length = snprintf(buffer, 2048, "%s\n", afsToc[i].name);
				fwrite(buffer, length, 1, afsFileList);
				snprintf(buffer, 2048, "%s/%s", outputPath, afsToc[i].name);

				printf("%s\n", afsToc[i].name, afsToc[i].size);
				void* data = (void*) AFSReadFileData(fp, &afsToc[i]);
				FILE* ofp = fopen(buffer, "wb");
				if (ofp && (afsToc[i].size > 0))
				{
					fwrite(data, afsToc[i].size, 1, ofp);
					fclose(ofp);
				}
				else
				{
					if (ofp)
						fclose(ofp);
					else
						fprintf(stderr, "[ExtractAFS] Could not open %s for writing.\n", afsToc[i].name);
				}
				free(data);
			}
			fclose(afsFileList);
		}
		else
		{
			fprintf(stderr, "[ExtractAFS] Could not open AFSFileList.txt for writing.\n");
			result = 0;
		}

		free(afsToc);
	}
	else
	{
		fprintf(stderr, "[ExtractAFS] Could not get the table of contents for the AFS.\n");
		result = 0;
	}
	return result;
}

int CmdExtractAFS(int count, char** argv)
{
	if (count != 2)
	{
		fprintf(stderr, "[ExtractAFS] Incorrect amount of arguments\n");
		return 0;
	}

	char* afsPath = argv[0];
	char* outputPath = argv[1];

	FILE* fp = fopen(afsPath, "rb");
	if (!fp)
	{
		fprintf(stderr, "[ExtractAFS] Could not open \"%s\" for writing.\n", argv[0]);
		return 0;
	}
	int r = ExtractAFS(fp, outputPath);
	fclose(fp);
	return r;
}