#pragma once
#include "tim2.h"

#ifdef __cplusplus
extern "C" {
#endif

int OutbreakTm2ToPng(void* input, const char* outputFileName);
void* OutbreakPngToTm2(const char* input, uint16_t tbp, int* x, int* y, uint32_t* resultSize);

#ifdef __cplusplus
}
#endif