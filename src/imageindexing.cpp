#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <vector>

// Median cut, basically

union Color
{
	uint32_t full;
	struct { uint8_t r, g, b, a; };
	uint8_t elements[4];

	inline int distanceSquared()
	{
		return r*r + g*g + b*b + a*a;
	}

	inline int average()
	{
		return (r + g + b + a) / 4;
	}
};

static int FindMatchingIndex(Color* palette, Color color)
{
	/*
		Searching for a color in the palette with least eucledian distance
		with the original color.
	*/
	int min = 255*255*5;
	int resultIndex = 0;
	for (int i = 0; i < 256; i++)
	{
		int r = color.r - palette[i].r;
		int g = color.g - palette[i].g;
		int b = color.b - palette[i].b;
		int a = color.a - palette[i].a;
		int d = r * r + g * g + b * b + a * a;
		if (d == 0)
			return i;
		if (min > d)
		{
			min = d;
			resultIndex = i;
		}
	}
	return resultIndex;
}

struct Bucket
{
	uint8_t mins[4];
	uint8_t ranges[4];
	int importance;
	std::vector<Color> colors;
};

static void CalcRanges(Bucket* bucket)
{
	int min[4], max[4];

	for (int i = 0; i < 4; i ++)
	{
		min[i] = 99999;
		max[i] = -1;
	}

	for (int i = 0; i < bucket->colors.size(); i++)
	{
		for (int j = 0; j < 4; j++)
		{
			if (min[j] > bucket->colors[i].elements[j])
				min[j] = bucket->colors[i].elements[j];

			if (max[j] < bucket->colors[i].elements[j])
				max[j] = bucket->colors[i].elements[j];
		}
	}

	for (int i = 0; i < 4; i++)
	{
		bucket->mins[i] = (uint8_t) min[i];
		bucket->ranges[i] = (uint8_t) (max[i] - min[i]);
	}

	/*
	   Alpha is less important than RGB, 
	   plus the bucket becomes more important depending on how many
	   colors there are in it
	*/

	int rgbRangeDistanceSquared = bucket->ranges[0] * bucket->ranges[0] + 
							      bucket->ranges[1] * bucket->ranges[1] + 
							      bucket->ranges[2] * bucket->ranges[2];
	int alphaRangeSquared = bucket->ranges[3] * bucket->ranges[3];
	bucket->importance = (1 + bucket->colors.size() / 32) * 
	                     (3 * rgbRangeDistanceSquared + 2 * alphaRangeSquared);
}

static void SplitBucket(std::vector<Bucket>& buckets, Bucket* bucket, int channel)
{
	int splitPoint = (2 * bucket->mins[channel] + bucket->ranges[channel]) / 2;
	int bucketId = buckets.size();

	buckets.resize(bucketId+1);
	
	std::vector<Color> oldColors(bucket->colors);
	bucket->colors.resize(0);
	for (int i = 0; i < oldColors.size(); i++)
	{
		if (oldColors[i].elements[channel] > splitPoint)
			buckets[bucketId].colors.push_back(oldColors[i]);
		else
			bucket->colors.push_back(oldColors[i]);
	}

	CalcRanges(bucket);
	CalcRanges(&buckets[bucketId]);
}

static void ReduceColors(std::vector<Color> colors, Color* palette)
{
	std::vector<Bucket> buckets(1);
	buckets.reserve(256);
	buckets[0].colors = colors;

	CalcRanges(&buckets[0]);
	while (buckets.size() < 256)
	{
		/* Find a bucket with highest importance */
		int maxImportance = -1;
		int bucketId = 0;
		for (int i = 0; i < buckets.size(); i++)
		{
			if (maxImportance < buckets[i].importance)
			{
				maxImportance = buckets[i].importance;
				bucketId = i;
			}
		}

		/* Find most important channel in the bucket */
		int channelId = 0;
		int range = -1;
		for (int i = 0; i < 4; i++)
		{
			int cRange = buckets[bucketId].ranges[i];
			if (cRange > range)
			{
				range = cRange;
				channelId = i;
			}
		}
		SplitBucket(buckets, &buckets[bucketId], channelId);
	}

	for (int i = 0; i < 256; i++)
	{
		int channels[4];
		for (int j = 0; j < 4; j++)
			channels[j] = 0;
		
		/* Getting the average color */
		for (int j = 0; j < buckets[i].colors.size(); j++)
		{
			for (int c = 0; c < 4; c++)
				channels[c] += buckets[i].colors[j].elements[c];
		}
		for (int j = 0; j < 4; j++)
			channels[j] /= buckets[i].colors.size();

		Color color;
		for (int j = 0; j < 4; j++)
			color.elements[j] = (uint8_t) channels[j];

		palette[i] = color;
	}
}

extern "C" void ImIdx_IndexImage(uint32_t* image, int width, int height, int* resultIndices, uint32_t* resultPalette)
{
	int colorCount = width*height;
	memset(resultPalette, 0, sizeof(uint32_t)*256);

	std::vector<std::vector<Color>> colors(256);

	Color* imageColors = (Color*) image;
	for (int i = 0; i < colorCount; i++)
	{
		Color color = imageColors[i];
		int id = color.average();

		bool found = false;
		for (int j = 0; j < colors[id].size(); j++)
		{
			if (colors[id][j].full == color.full)
			{
				found = true;
				break;
			}
		}

		if (!found)
			colors[id].push_back(imageColors[i]);
	}

	std::vector<Color> uniqueColors;
	for (int i = 0; i < 256; i++)
	{
		for (int j = 0; j < colors[i].size(); j++)
			uniqueColors.push_back(colors[i][j]);
	}

	Color* palette = (Color*) resultPalette;

	if (uniqueColors.size() > 256)
	{
		printf("[imageindexing] Image has more than 256 colors (%d), color reduction will be applied.\n", (int) uniqueColors.size());
		ReduceColors(uniqueColors, palette);
	}
	else
	{
		for (int i = 0; i < uniqueColors.size(); i++)
			palette[i] = uniqueColors[i];
	}

	for (int i = 0; i < width*height; i++)
	{
		int index = FindMatchingIndex(palette, imageColors[i]);
		resultIndices[i] = index;
	}
}