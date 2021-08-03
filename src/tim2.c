#include "tim2.h"
#include <stddef.h>

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))
#define CLAMP(x, upper, lower) (MIN(upper, MAX(x, lower)))

#define BUILD_RGBA(r, g, b, a) (((a)<< 24)|((b)<<16)|((g)<<8)|(r))

TIM2Image* TIM2_GetImage(TIM2Header* header, int index)
{
	TIM2Image* img = NULL;
	if (header->fourcc != TIM2_FOURCC)
		return NULL;
	if (index >= header->imageCount)
		return NULL;
	
	
	if (header->largeHeader)
		img = (TIM2Image*) ((char*) header + 0x80);
	else
		img = (TIM2Image*) ((char*) header + sizeof(TIM2Header));

	
	for (int i = 0; i < index; i++)
		img = (TIM2Image*) ((char*) img + img->totalSize);
	
	return img;
}

int TIM2_GetMipmapSize(TIM2Image* img, int level, int* width, int* height)
{
	int w = img->imageWidth  >> level;
	int h = img->imageHeight >> level;
	
	if (width)
		*width = w;
	
	if (height)
		*height = h;

	int byteCount = w * h;
	
	switch (img->imageType)
	{
		case TIM2_RGB16:    byteCount *= 2; break;
		case TIM2_RGB24:    byteCount *= 3; break;
		case TIM2_RGBA32:   byteCount *= 4; break;
		case TIM2_INDEXED4:	byteCount /= 2; break;
		case TIM2_INDEXED8: break;
	}
	/* Always aligned to 16 bytes. */
	byteCount = (byteCount + 15) & ~15;
	return byteCount;
}

TIM2Mipmap* TIM2_GetMipmap(TIM2Image* img)
{
	TIM2Mipmap* mipmap = NULL;

	if (img->mipmapLevels > 1)
		mipmap = (TIM2Mipmap*)((char*) img + sizeof(TIM2Image));

	return mipmap;
}

void* TIM2_GetImageData(TIM2Image* img, int mipmapLevel)
{
	if (mipmapLevel >= img->mipmapLevels)
		return NULL;

	void* imageData = (void *)((char *) img + img->headerSize);
	if (img->mipmapLevels == 1)
		return imageData;

	TIM2Mipmap* mipmap = (TIM2Mipmap*) ((char*) img + sizeof(TIM2Image));

	for (int i = 0; i < mipmapLevel; i++)
		imageData = (void*)((char*) imageData + mipmap->imageSizes[i]);

	return imageData;
}

void* TIM2_GetClutData(TIM2Image* img)
{
	void* clut = NULL;

	if (img->clutColorCount > 0)
		clut = (void*)((char*) img + img->headerSize + img->imageSize);

	return clut;
}

uint32_t TIM2_GetClutColor(TIM2Image* img, int clutId, int id)
{
	uint8_t* clut = (uint8_t*) TIM2_GetClutData(img);
	if (!clut)
		return 0;
	
	int i;
	// getting 'global' color id starting from the first clut
	switch (img->imageType)
	{
		case TIM2_INDEXED4: i = clutId * 16  + id; break;
		case TIM2_INDEXED8: i = clutId * 256 + id; break;
		default: return 0;
	}
	
	if (i > img->clutColorCount)
		return 0;

	/* checking whether the clut is rearranged (kinda like swizzling I guess? except less complicated) 
	   and getting the right color id 
		(CSM2 doesn't need rearranging)
	*/
	if (img->imageType == TIM2_INDEXED8)
	{
		int csm1 = img->clutType & 0x80 ? 0 : 1;
		if (csm1)
		{
			if ((i & 0x1F) >= 8)
			{
				if ((i & 0x1F) < 16) 
					i += 8;
				else if ((i & 0x1F) < 24)
					i -= 8;
			}
		}
	}

	uint8_t r, g, b, a;
	switch (img->clutType & 0x3F)
	{
		case TIM2_RGB16:
		{
			r = (uint8_t) ((((clut[i * 2 + 1] << 8) | clut[i * 2]) << 3) & 0xF8);
			g = (uint8_t) ((((clut[i * 2 + 1] << 8) | clut[i * 2]) >> 2) & 0xF8);
			b = (uint8_t) ((((clut[i * 2 + 1] << 8) | clut[i * 2]) >> 7) & 0xF8);
			a = (uint8_t) ((((clut[i * 2 + 1] << 8) | clut[i * 2]) >> 8) & 0x80);
		} break;
		case TIM2_RGB24:
		{
			r = clut[i * 3];
			g = clut[i * 3 + 1];
			b = clut[i * 3 + 2];
			a = 0xFF;
		} break;
		case TIM2_RGBA32:
		{
			r = clut[i * 4];
			g = clut[i * 4 + 1];
			b = clut[i * 4 + 2];
			a = clut[i * 4 + 3];
		} break;
		default:
		{
			r = 0;
			g = 0;
			b = 0;
			a = 0;
		} break;
	}
	return (uint32_t) BUILD_RGBA(r, g, b, a);
}

uint32_t TIM2_GetTexel(TIM2Image* img, int mipmapLevel, int x, int y, int clutId)
{
	uint8_t* image = (uint8_t*) TIM2_GetImageData(img, mipmapLevel);
	if (!image)
	   return 0;

	int width, height;
	TIM2_GetMipmapSize(img, mipmapLevel, &width, &height);
	x = CLAMP(x, width-1, 0);
	y = CLAMP(y, height-1, 0);

	int texelId = y * width + x;
	switch (img->imageType)
	{
		case TIM2_RGB16:
		{
			uint16_t texel = *(uint16_t*)(&image[texelId * 2]);
			return (uint32_t) BUILD_RGBA((texel << 3) & 0xF8, 
										 (texel >> 2) & 0xF8, 
										 (texel >> 7) & 0xF8,
										  texel & 0x8000 ? 0xFF : 0x00);
		} break;
		case TIM2_RGB24:    return ((*(uint32_t*)(&image[texelId * 3])) & 0x00FFFFFF) | 0xFF000000;
		case TIM2_RGBA32:   return ((uint32_t*) image)[texelId]; break;
		case TIM2_INDEXED4: return TIM2_GetClutColor(img, clutId, (texelId & 1) ? (image[texelId / 2] >> 4) : (image[texelId / 2] & 0x0F)); break;
		case TIM2_INDEXED8: return TIM2_GetClutColor(img, clutId, image[texelId]); break;
	}
	return 0;
}

void TIM2_ConvToRGBA32 (TIM2Image* img, void* output, int clutId)
{
	if (img->imageType >= 6 || img->imageType == TIM2_NONE)
		return;

	uint32_t* outputColors = (uint32_t*) output;
	for (int x = 0; x < img->imageWidth; x++)
		for (int y = 0; y < img->imageHeight; y++)
			outputColors[y * img->imageWidth + x] = TIM2_GetTexel(img, 0, x, y, clutId);
}