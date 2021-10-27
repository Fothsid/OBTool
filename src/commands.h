#pragma once

#ifdef __cplusplus
extern "C" {
#endif

int CmdConvertToFBX(int count, char** argv);
int CmdConvertToNBD(int count, char** argv);
int CmdExtractScenarioAFS(int count, char** argv);
int CmdPatchNBDTextures(int count, char** argv);

enum InputType
{
	INPUT_INTEGER,
	INPUT_FLOAT,
	INPUT_STRING,
	INPUT_FILEOPEN,
	INPUT_FILESAVE,
	INPUT_FOLDER,
	INPUT_INTEGERARRAY
};

typedef struct
{
	int type;
	const char* name;
	const char* fullName;
} Input;

typedef struct
{
	const char* name;
	const char* description;
	int (*function) (int count, char** argv);

	int inputCount;
	const Input* inputs;
} Command;

extern int CommandCount;
extern Command CommandList[];

#ifdef __cplusplus
}
#endif