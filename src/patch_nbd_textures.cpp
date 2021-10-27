#include "commands.h"
#include "tim2.h"
#include <iostream>
#include <cstdlib>
#include <fstream>
#include <thanatos.h>

int CmdPatchNBDTextures(int count, char** argv)
{
	if (count < 3)
	{
		fprintf(stderr, "[PatchNBDTextures] Incorrect amount of arguments\n");
		return 0;
	}
	char* f_input = argv[0];
	char* f_output = argv[1];

	fprintf(stdout, "Opening the input NBD file...\n");
	std::ifstream inputStream(f_input, std::ios_base::binary);
	if (inputStream.fail())
	{
		fprintf(stderr, "[PatchNBDTextures] Error opening \"%s\" (%s)\n", f_input, strerror(errno));
		return 0;
	}

	OBNbd nbd;
	if (!nbd.read(inputStream))
	{
		fprintf(stderr, "[PatchNBDTextures] Error parsing the NBD file.\n");
		return 0;
	}

	inputStream.close();

	if (nbd.getType() == OBType::RoomNBD || nbd.getTextureStorageType() == OBType::TextureAFSRef)
	{
		fprintf(stderr, "Room NBDs don't need patching.\n");
		return 0;
	}

	if (nbd.textures.size() != count-2)
	{
		fprintf(stderr, "ID count doesn't match texture count.\n");
		return 0;
	}

	fprintf(stdout, "Patching...\n");
	for (int i = 0; i < nbd.textures.size(); i++)
	{
		TIM2Header* header = (TIM2Header*) nbd.textures[i].data.tim2Data;
		header->globalTextureId = strtol(argv[2+i], NULL, 16);
	}

	std::ofstream outputStream(f_output, std::ios_base::binary);
	if (outputStream.fail())
	{
		fprintf(stderr, "[PatchNBDTextures] Error opening \"%s\" (%s)\n", f_output, strerror(errno));
		return 0;
	}

	fprintf(stdout, "Saving the modified NBD...\n");
	nbd.write(outputStream);
	outputStream.close();

	fprintf(stdout, "Done!\n");

	return 1;
}