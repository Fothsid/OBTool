#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define TIM2_FOURCC (0x324d4954)
#define TIM2_EXT_FOURCC (0x00745865)

enum TIM2_IMAGE_TYPE
{
	TIM2_NONE = 0,
	TIM2_RGB16,
	TIM2_RGB24,
	TIM2_RGBA32,
	TIM2_INDEXED4,
	TIM2_INDEXED8
};

typedef struct _tagTIM2Header
{
	uint32_t fourcc;
	uint8_t version,
	        largeHeader;
	uint16_t imageCount;
	uint16_t globalTextureId; /* Warning: out of TM2 spec. Exclusive to RE: Outbreak. */
	uint8_t padding[6];
} TIM2Header;

typedef struct _tagTIM2Image
{
	uint32_t totalSize,
	         clutSize,
	         imageSize;
	uint16_t headerSize,
	         clutColorCount;
	uint8_t imageFormat, /* I have no idea what this does */
	        mipmapLevels,
	        clutType,
	        imageType;
	uint16_t imageWidth,
	         imageHeight;

	uint64_t gsTex0,
	         gsTex1;
	uint32_t gsTexaFbaPabe,
	         gsTexClut;
} TIM2Image;

typedef struct _tagGsTex0
{
	uint32_t TBP0  : 14;
	uint32_t TBW   :  6;
	uint32_t PSM   :  6;
	uint32_t TW    :  4;
	uint32_t TH    :  4;
	uint32_t TCC   :  1;
	uint32_t TFX   :  2;
	uint32_t CBP   : 14;
	uint32_t CPSM  :  4;
	uint32_t CSM   :  1;
	uint32_t CSA   :  5;
	uint32_t CLD   :  3;
} GsTex0;

typedef struct _tagTIM2Mipmap
{
	uint64_t gsMipTbp1,
	         gsMipTbp2;
	uint32_t imageSizes[];
} TIM2Mipmap;

typedef struct _tagTIM2ExtHeader
{
	uint32_t fourcc,
	         userSpaceSize,
	         userDataSize,
	         reserved;
} TIM2ExtHeader;

TIM2Image*  TIM2_GetImage     (TIM2Header* header, int index);
int         TIM2_GetMipmapSize(TIM2Image* img, int level, int* width, int* height);
TIM2Mipmap* TIM2_GetMipmap    (TIM2Image* img);
void*       TIM2_GetImageData (TIM2Image* img, int mipmapLevel);
void*       TIM2_GetClutData  (TIM2Image* img);
uint32_t    TIM2_GetClutColor (TIM2Image* img, int clutId, int id);
uint32_t    TIM2_GetTexel     (TIM2Image* img, int mipmapLevel, int x, int y, int clutId);
void        TIM2_CorrectGsTex (TIM2Image* img);
void        TIM2_ConvToRGBA32 (TIM2Image* img, void* output, int clutId);

#ifdef __cplusplus
}
#endif