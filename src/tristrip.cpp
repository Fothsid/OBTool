#include "tristrip.h"
#include <stack>
#include "striper/StdAfx.h"

/*
	The algorithm in the striper folder is taken from
	     
	     http://www.codercorner.com/Strips.htm

	I tried writing one myself but it became a disaster so I scrapped it.
*/

static void GroupTrianglesByMaterial(TriangleGroupList& result, STriList& triList)
{
	for (int i = 0; i < triList.size(); i++)
	{
		int groupId = triList[i].material;
		result[groupId].push_back(triList[i].tri);
	}
}

static bool IsFullWeight(int i, std::vector<_Tri>& triangles, std::vector<Vertex>& vertices)
{
	Triangle t = triangles[i].tri;
	
	bool ok = true;
	for (int i = 0; i < 3; i++)
	{
		if (vertices[t.idx[i]].weights.size() > 1)
			ok = false;
	}

	return ok;
}

static void GroupTrianglesByWeight(STriGroupList& result, std::vector<_Tri>& triangles, std::vector<Vertex>& vertices)
{
	for (int i = 0; i < triangles.size(); i++)
	{
		int groupId = 1;
		if (IsFullWeight(i, triangles, vertices))
			groupId = 0;

		result[groupId].push_back(triangles[i]);
	}
}

static void FillTriangles(STriGroupList& result, std::vector<_Tri>& triangles, std::vector<Vertex>& vertices)
{
	for (int i = 0; i < triangles.size(); i++)
	{
		result[0].push_back(triangles[i]);
	}
}

static void TriangleListToTriangleStrips(StripList& result, STriList& triList, int materialCount)
{
	TriangleGroupList groupList;
	groupList.resize(materialCount);
	GroupTrianglesByMaterial(groupList, triList);

	int stripIndex = 0;
	for (int g = 0; g < groupList.size(); g++)
	{
		if (groupList[g].size() < 1)
			continue;

		STRIPERCREATE sc;
		sc.DFaces			= &groupList[g][0].idx[0];
		sc.NbFaces			= groupList[g].size();
		sc.AskForWords		= false;
		sc.ConnectAllStrips	= false;
		sc.OneSided			= true;
		sc.SGIAlgorithm		= false; // This option sometimes results in wrong triangle index order

		Striper striper;
		if (!striper.Init(sc))
		{
			printf("[tristrip] Could not initialize the striper. (%d; %d)\n", g, groupList[g].size());
			continue;
		}

		STRIPERRESULT sr;
		if (!striper.Compute(sr))
		{
			printf("[tristrip] Could not create triangle strips. (%d; %d)\n", g, groupList[g].size());
			continue;
		}

		result.resize(result.size() + sr.NbStrips);

		uint32_t* stripStart = (uint32_t*) sr.StripRuns;
		for (int t = 0; t < sr.NbStrips; ++t)
		{
			uint32_t length = sr.StripLengths[t];
			result[stripIndex + t].leftHand = false;
			result[stripIndex + t].material = g;
			result[stripIndex + t].indices.resize(length);
			for (int k = 0; k < length; k++)
				result[stripIndex + t].indices[k] = stripStart[k];
			stripStart += length;
		}
		stripIndex += sr.NbStrips;
	}
}

void ConvertMeshToTriangleStrips(FbxManager* sdkManager, StripGroupList& result, FbxNode* meshNode,
								 std::vector<_Tri>& triangles, std::vector<Vertex>& vertices)
{

	/* Resident Evil Outbreak has support for this sort-of optimisation
	   where triangle strips are divided into two lists:
	      - List where all vertices have ~100% weight on one bone (apparently not always?)
	      - List where vertex weight may vary
	*/
	STriGroupList groupList;
	FbxMesh* mesh = meshNode->GetMesh();
	FbxSkin* skin = (FbxSkin*) mesh->GetDeformer(0);
	if (skin)
	{
		if (skin->GetClusterCount() > 1)
			groupList.resize(2);
		else
			groupList.resize(1);
	}
	else
	{
		groupList.resize(1);
	}

	if (groupList.size() == 2)
		GroupTrianglesByWeight(groupList, triangles, vertices);
	else
		FillTriangles(groupList, triangles, vertices);

	if (groupList.size() == 2)
	{
		if (groupList[1].size() == 0)
			groupList.resize(1);
	}

	result.resize(groupList.size());

	for (int i = 0; i < groupList.size(); i++)
		TriangleListToTriangleStrips(result[i], groupList[i], meshNode->GetMaterialCount());

}