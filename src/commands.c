#include "commands.h"

/*
	Warning:
	THIS IS A C FILE,
					  NOT C++		
 */

int CommandCount = 4;
Command CommandList[] =
{
	{
		"ConvertToFBX", 
		"Converts an input .NBD file to .FBX",
		CmdConvertToFBX,
		2,
		(Input[])
		{
			{INPUT_FILEOPEN, "in_nbd", "Input .NBD file"},
			{INPUT_FILESAVE, "out_fbx", "Output .FBX file"},
		}
	},
	{
		"ConvertToNBD", 
		"Converts an input .FBX file to .NBD",
		CmdConvertToNBD,
		2, 
		(Input[])
		{
			{INPUT_FILEOPEN, "in_fbx", "Input .FBX file"},
			{INPUT_FILESAVE, "out_nbd", "Output .NBD file"},
		}
	},
	{
		"ExtractScenarioAFS",
		"Extracts the scenario AFS from NETBIO00.DAT",
		CmdExtractScenarioAFS,
		3,
		(Input[])
		{
			{INPUT_FILEOPEN, "in_netbio00", "Input NETBIO00.DAT"},
			{INPUT_INTEGER, "scenario_id", "Scenario ID"},
			{INPUT_FOLDER, "out_folder", "Output folder"},
		}
	},
	{
		"PatchNBDTextures",
		"Pathces global texture id values in an NBD file.",
		CmdPatchNBDTextures,
		3,
		(Input[])
		{
			{INPUT_FILEOPEN, "in_nbd", "Input NBD file"},
			{INPUT_FILESAVE, "out_nbd", "Output patched NBD file"},
			{INPUT_INTEGERARRAY, "id0, id1, id2, ...", "Global Texture IDs"},
		}
	},
};