#pragma once
#include <vector>
#include <thanatos.h>
#include "fbx.h"

struct Vertex
{
	inline Vertex(float _x, float _y, float _z, 
				  float _tx, float _ty, 
				  float _nx, float _ny, float _nz,
				  float _r, float _g, float _b, float _a)
	{
		x = _x; y = _y; z = _z;
		tx = _tx; ty = _ty;
		nx = _nx; ny = _ny; nz = _nz;
		r = _r; g = _g; b = _b; a = _a;
	}
	float x, y, z;
	float tx, ty;
	float nx, ny, nz;
	float r, g, b, a;
	std::vector<OBWeight> weights;
};

inline bool operator==(const Vertex& a, const Vertex& b)
{
	if (a.x == b.x &&
		a.y == b.y &&
		a.z == b.z &&
		a.tx == b.tx &&
		a.ty == b.ty &&
		a.nx == b.nx &&
		a.ny == b.ny &&
		a.nz == b.nz &&
		a.r == b.r &&
		a.g == b.g &&
		a.b == b.b &&
		a.a == b.a)
	{
		if (a.weights.size() != b.weights.size())
		{
			return false;
		}
		else
		{
			for (int i = 0; i < a.weights.size(); i++)
			{
				if (a.weights[i].id != b.weights[i].id ||
					a.weights[i].weight != b.weights[i].weight)
				{
					return false;
				}
			}
			return true;
		}
	}
	else
	{
		return false;
	}
}

struct TStrip
{
	int material;
	bool leftHand;
	std::vector<uint32_t> indices;
};

struct Triangle
{
	uint32_t idx[3];
};

typedef std::vector<Triangle> TriList;
typedef std::vector<TriList> TriangleGroupList;

struct _Tri
{
	Triangle tri;
	int material;
};
typedef std::vector<_Tri> STriList;
typedef std::vector<STriList> STriGroupList;

typedef std::vector<TStrip> StripList;
typedef std::vector<StripList> StripGroupList;

void ConvertMeshToTriangleStrips(FbxManager* sdkManager, StripGroupList& result, FbxNode* meshNode,
								 std::vector<_Tri>& triangles, std::vector<Vertex>& vertices);