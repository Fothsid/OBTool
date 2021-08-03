#include "tim2utils.h"

#include <stdio.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))
#define CLAMP(x, upper, lower) (MIN(upper, MAX(x, lower)))

int OutbreakTm2ToPng(void* input, const char* outputFileName)
{
	TIM2Header* head = (TIM2Header*) input;
	if (head->tbp)
		printf("[tim2utils] This TM2 file contains a TBP value! -> %X\n", head->tbp);
	if (head->imageCount != 1)
	{
		fprintf(stderr, "[tim2utils] Image count isn't 1. Exiting out.\n");
		return 0;
	}

	int i = 0;	
	TIM2Image* img = TIM2_GetImage((TIM2Header*) input, i);
	
	int clutCount = 1;
	if (img->imageType == TIM2_INDEXED4)
	{
		printf("[tim2utils] INDEXED4\n");
		clutCount = img->clutColorCount / 16;
	}
	else if (img->imageType == TIM2_INDEXED8)
	{
		clutCount = img->clutColorCount / 256;
	}

	if (clutCount > 1)
	{
		printf("[tim2utils] This TM2 file has %d palettes\n", clutCount);
	}
	
	int c = 0;
	size_t imageSize = img->imageWidth * img->imageHeight * 4;
	void* outputData = malloc(imageSize);
	
	TIM2_ConvToRGBA32(img, outputData, c);
	
	if (stbi_write_png(outputFileName, img->imageWidth, img->imageHeight, 4, outputData, 0) == 0)
	{
		fprintf(stderr, "[tim2utils] Failed to convert to PNG\n");
		free(outputData);
		return 0;
	}
	free(outputData);
	return 1;
}

static void stb_to_tim2(void* input, int size, const char* output, uint16_t tbp)
{
	int width, height, _chnlCount;
	uint32_t* data = (uint32_t*) stbi_load_from_memory(input, size, &width, &height, &_chnlCount, 4);
	if (!data)
	{
		printf("ERROR: stb_image failed to load the input file.\n");
		exit(1);
	}
	
	uint32_t resultFileSize = sizeof(TIM2Header) + sizeof(TIM2Image) +  width * height + 1024;
	void* result = malloc(resultFileSize);
	memset(result, 0, resultFileSize);
	
	TIM2Header* header = (TIM2Header*) result;
	header->fourcc = TIM2_FOURCC;
	header->version = 4;
	header->largeHeader = 0;
	header->imageCount = 1;
	header->tbp = tbp;
	
	TIM2Image* img = TIM2_GetImage(header, 0);
	img->totalSize = width * height + 1024 + sizeof(TIM2Image);
	img->clutSize  = 1024;
	img->imageSize = width * height;
	img->headerSize = sizeof(TIM2Image);
	img->clutColorCount = 256;
	img->imageFormat = 0;
	img->mipmapLevels = 1;
	img->clutType = TIM2_RGBA32;
	img->imageType = TIM2_INDEXED8;
	img->imageWidth = width;
	img->imageHeight = height;
	
	printf("Width: %d\nHeight: %d\n", img->imageWidth, img->imageHeight);
	
	uint32_t* clut = TIM2_GetClutData(img);
	uint8_t* image = (uint8_t*) TIM2_GetImageData(img, 0);
	int clutColorId = 0;
	for (int px = 0; px < width*height; px++)
	{
		int found = 0;
		int foundId = 0;
		if (clutColorId > 0);
		for (int i = 0; i < clutColorId; i++)
		{
			if (clut[i] == data[px])
			{
				found = 1;
				foundId = i;
				break;
			}
		}
		
		if (found)
			image[px] = foundId;
		else
		{
			if (clutColorId > 255)
			{
				printf("ERROR: The image contains more than 256 colors.\n");
				exit(1);
			}
			image[px] = clutColorId;
			clut[clutColorId] = data[px];
			clutColorId++;
		}
	}
	
	/* Rearranging CLUT */
	int temp[8];
	for (int i = 0; i < 8; i++)      
	{
		int o2 = i*32+8;
		int o3 = i*32+16;
		for (int j = 0; j < 8; j++)
		{
			temp[j] = clut[o2+j];
			clut[o2+j] = clut[o3+j];
			clut[o3+j] = temp[j];
		}
	}
	
	
	FILE* fp = fopen(output, "wb");
	fwrite(result, resultFileSize, 1, fp);
	fclose(fp);
}