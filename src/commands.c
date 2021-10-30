#include "commands.h"

/*
	Warning:
	THIS IS A C FILE,
					  NOT C++		
 */

int CommandCount = 8;
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
		"BuildScenarioAFS",
		"Builds the scenario AFS from a folder",
		CmdBuildScenarioAFS,
		3,
		(Input[])
		{
			{INPUT_FOLDER, "in_folder", "Input folder with scenario data"},
			{INPUT_FILESAVE, "out_afs", "Output scenario AFS"},
			{INPUT_FILESAVE, "out_scenario_toc", "Output rXXX_h.bin table of contents file"},
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
	{
		"BuildMOMO",
		"Builds a MOMO file from a list of files.",
		CmdBuildMOMO,
		2,
		(Input[])
		{
			{INPUT_FILESAVE, "out_momo", "Output MOMO file"},
			{INPUT_INTEGERARRAY, "file0, file1, file2, ...", "File names to pack"},
		}
	},
	{
		"ExtractAFS",
		"Extracts files from an AFS.",
		CmdExtractAFS,
		2,
		(Input[])
		{
			{INPUT_FILEOPEN, "in_afs", "Input AFS file"},
			{INPUT_FOLDER, "out_folder", "Output folder"},
		}
	},
	{
		"BuildAFS",
		"Builds an AFS file from a folder",
		CmdBuildAFS,
		2,
		(Input[])
		{
			{INPUT_FOLDER, "in_folder", "Input folder"},
			{INPUT_FILEOPEN, "out_afs", "Output AFS"},
		}
	},
};