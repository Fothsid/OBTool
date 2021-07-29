#include "commands.h"

/*
	Warning:
	THIS IS A C FILE,
					  NOT C++		
 */

int CommandCount = 2;
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
	}
};