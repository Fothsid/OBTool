#pragma once

#include <cstdint>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

	typedef int8_t			sbyte;
	typedef uint8_t		ubyte;
	typedef int16_t		sword;
	typedef uint16_t		uword;
	typedef int32_t			sdword;
	typedef uint32_t		udword;
	typedef int64_t		sqword;
	typedef uint64_t	uqword;
	typedef float				sfloat;

	#define	null	NULL
	#define RELEASE(x)		{ if (x != null) delete x;		x = null; }
	#define RELEASEARRAY(x)	{ if (x != null) delete []x;	x = null; }

	inline void ZeroMemory(void* addr, udword size)
	{
		memset(addr, 0, size);
	}

	inline void CopyMemory(void* dest, const void* src, udword size)
	{
		memcpy(dest, src, size);
	}

	inline void FillMemory(void* dest, udword size, ubyte val)
	{
		memset(dest, val, size);
	}

#include "RevisitedRadix.h"
#include "CustomArray.h"
#include "Adjacency.h"
#include "Striper.h"