#pragma once

#define FBXSDK_SHARED
#include <fbxsdk.h>

inline FbxVector4 operator*(const FbxAMatrix& m, const FbxVector4& v)
{
	FbxVector4 result;
	result[0] = v[0] * m.Get(0, 0) + v[1] * m.Get(0, 1) + v[2] * m.Get(0, 2) + v[3] * m.Get(0, 3);
	result[1] = v[0] * m.Get(1, 0) + v[1] * m.Get(1, 1) + v[2] * m.Get(1, 2) + v[3] * m.Get(1, 3);
	result[2] = v[0] * m.Get(2, 0) + v[1] * m.Get(2, 1) + v[2] * m.Get(2, 2) + v[3] * m.Get(2, 3);
	result[3] = v[0] * m.Get(3, 0) + v[1] * m.Get(3, 1) + v[2] * m.Get(3, 2) + v[3] * m.Get(3, 3);
	return result;
}