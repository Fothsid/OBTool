#include "commands.h"
#include "tristrip.h"
#include "tim2utils.h"
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <algorithm>

struct TextureSize
{
	int width, height;
};

#define M_PI 3.14159265358979323846264338327950288

static bool LoadTextureFromFile(const FbxString& filePath, OBNbdTexture* texture, int* w, int* h)
{
	/* TODO: implement TBP export (Use user properties for textures perhaps?) */
	uint32_t dataSize;
	void* data = OutbreakPngToTm2(filePath.Buffer(), 0, w, h, &dataSize);
	if (data)
	{
		texture->size = (uint32_t) dataSize;
		texture->tim2Data = data;
		return true;
	}
	else
	{
		return false;
	}
}

static void ConvertTextures(OBNbd& nbd, FbxScene* scene, const char* fbxFilename, std::vector<TextureSize>& texSizes)
{
	nbd.textures.resize(scene->GetTextureCount());
	texSizes.resize(scene->GetTextureCount());
	for (int i = 0; i < scene->GetTextureCount(); i++)
	{
		FbxTexture* texture = scene->GetTexture(i);
		FbxFileTexture* fileTexture = FbxCast<FbxFileTexture>(texture);
		fileTexture->SetUserDataPtr((void*)i); // really dumb stuff here lmao
		
		if (fileTexture)
		{
			TextureSize* ts = &texSizes[i];
			const FbxString fileName = fileTexture->GetFileName();
			bool status = LoadTextureFromFile(fileName, &nbd.textures[i], &ts->width, &ts->height);
			const FbxString absFbxFileName = FbxPathUtils::Resolve(fbxFilename);
			const FbxString absFolderName = FbxPathUtils::GetFolderName(absFbxFileName);
			if (!status)
			{
				const FbxString resolvedFileName = FbxPathUtils::Bind(absFolderName, fileTexture->GetRelativeFileName());
				status = LoadTextureFromFile(resolvedFileName, &nbd.textures[i], &ts->width, &ts->height);
			}
			if (!status)
			{
				const FbxString textureFileName = FbxPathUtils::GetFileName(fileName);
				const FbxString resolvedFileName = FbxPathUtils::Bind(absFolderName, textureFileName);
				status = LoadTextureFromFile(resolvedFileName, &nbd.textures[i], &ts->width, &ts->height);
			}
			if (!status)
			{
				fprintf(stderr, "Failed to load texture file: %s\n", fileName.Buffer());
				continue;
			}
		}
	}
}

static void AddDate(OBModel& model)
{
	model.date = new OBDate();
	model.date->year = 2;
	model.date->month = 4;
	model.date->day = 23;
}

static void FillTextures(OBModel& model, FbxScene* scene, std::vector<TextureSize>& textureSizes)
{
	model.textureList = new OBTextureList();
	model.textureList->list.resize(scene->GetTextureCount());
	for (int i = 0; i < scene->GetTextureCount(); i++)
	{
		model.textureList->list[i].id = i;
		model.textureList->list[i].width = textureSizes[i].width;
		model.textureList->list[i].height = textureSizes[i].height;
		model.textureList->list[i].tiling = 0;
	}
}

static void FillMaterials(OBModel& model, FbxScene* scene)
{
	model.materialList = new OBMaterialList();
	model.materialList->list.resize(scene->GetMaterialCount());
	for (int i = 0; i < scene->GetMaterialCount(); i++)
	{
		OBMaterial* mat = &model.materialList->list[i];
		FbxSurfaceMaterial* material = scene->GetMaterial(i);

		material->SetUserDataPtr((void*) i);

		FbxSurfacePhong* phong = (FbxSurfacePhong*) material;
		FbxSurfaceLambert* lambert = (FbxSurfaceLambert*) material;

		/* Forcing Lambert because the game doesn't seem to like Phong that much */
		mat->type = OBType::MaterialLambert;

		if (material->ShadingModel.Get() == "Phong")
		{
			mat->specular.r = phong->Specular.Get()[0];
			mat->specular.g = phong->Specular.Get()[1];
			mat->specular.b = phong->Specular.Get()[2];
			mat->specular.a = 1.0f;//phong->SpecularFactor.Get();
			mat->specularDecay = phong->Shininess.Get();
		}
		else
		{
			mat->specular.r = 0.0f;
			mat->specular.g = 0.0f;
			mat->specular.b = 0.0f;
			mat->specular.a = 1.0f;
			mat->specularDecay = 5.0f;
		}
		mat->ambient.r = lambert->Ambient.Get()[0];
		mat->ambient.g = lambert->Ambient.Get()[1];
		mat->ambient.b = lambert->Ambient.Get()[2];
		mat->ambient.a = 1.0f;//lambert->AmbientFactor.Get();
		mat->diffuse.r = lambert->Diffuse.Get()[0];
		mat->diffuse.g = lambert->Diffuse.Get()[1];
		mat->diffuse.b = lambert->Diffuse.Get()[2];
		mat->diffuse.a = 1.0f ;//- lambert->TransparencyFactor.Get(); //lambert->DiffuseFactor.Get();
		if (lambert->Diffuse.GetSrcObjectCount() > 0)
		{
			mat->textures.resize(1);
			mat->textures[0] = (int)lambert->Diffuse.GetSrcObject()->GetUserDataPtr();
		} 
	}
}

static void FillRenderAttributes(OBMesh& mesh, FbxNode* node)
{
	int id = 0;
	FbxProperty p[18];
	p[id++] = node->FindProperty("RA_EnableRenderAttribs", false);
	p[id++] = node->FindProperty("RA_Material",            false);
	p[id++] = node->FindProperty("RA_Specular",            false);
	p[id++] = node->FindProperty("RA_Cull",                false);
	p[id++] = node->FindProperty("RA_Scissor",             false);
	p[id++] = node->FindProperty("RA_Light",               false);
	p[id++] = node->FindProperty("RA_Wrap",                false);
	p[id++] = node->FindProperty("RA_UVScroll",            false);
	p[id++] = node->FindProperty("RA_DisableFog",          false);
	p[id++] = node->FindProperty("RA_FadeColor",           false);
	p[id++] = node->FindProperty("RA_SpecialShader",       false);
	p[id++] = node->FindProperty("RA_AlphaSrc",            false);
	p[id++] = node->FindProperty("RA_AlphaDst",            false);
	p[id++] = node->FindProperty("RA_AlphaOp",             false);
	p[id++] = node->FindProperty("RA_DepthOp",             false);
	p[id++] = node->FindProperty("RA_DepthWrite",          false);
	p[id++] = node->FindProperty("RA_Filtering",           false);
	p[id++] = node->FindProperty("RA_AddressMode",         false);
	
	if (p[0].IsValid() && p[0].Get<FbxBool>())
	{
		mesh.renderAttribs = new OBRenderAttribs();
		int32_t* attribs = &mesh.renderAttribs->attribs.version;
		attribs[0] = 0x10000;
		for (int i = 1; i < 18; i++)
			attribs[i] = p[i].Get<FbxInt>();
	}
}

static void FillMesh(FbxManager* sdkManager, OBMesh& mesh, FbxNode* node)
{
	FbxMesh* fbxMesh = node->GetMesh();
	if (!fbxMesh->IsTriangleMesh())
	{
		FbxGeometryConverter gConv(sdkManager);
		fbxMesh = (FbxMesh*) gConv.Triangulate(fbxMesh, true);
	}

	bool hasNormals = false, 
		 hasUVs     = false, 
		 hasColors  = false,
		 hasWeights = false;

	FbxStringList UVSetNameList;
	fbxMesh->GetUVSetNames(UVSetNameList);
	const char* elementUVName = NULL;
	FbxGeometryElementUV* elementUV = NULL;
	if (UVSetNameList.GetCount() > 0)
	{
		elementUVName = UVSetNameList.GetStringAt(0);
		elementUV = fbxMesh->GetElementUV(elementUVName);
	}

	FbxGeometryElementNormal*      elementNormal   = fbxMesh->GetElementNormal();
	FbxGeometryElementVertexColor* elementColor    = fbxMesh->GetElementVertexColor();
	FbxGeometryElementMaterial*    elementMaterial = fbxMesh->GetElementMaterial(0);
	FbxSkin*                       skin            = (FbxSkin*) fbxMesh->GetDeformer(0);

	if (elementNormal)
	{
		hasNormals = true;
		mesh.normalList = new OBNormalList();
	}

	if (elementUV)
	{
		hasUVs = true;
		mesh.texCoordList = new OBTexCoordList();
	}

	if (elementColor)
	{
		hasColors = true;
	}
	mesh.colorList = new OBColorList();

	if (skin && skin->GetDeformerType() == FbxDeformer::eSkin)
	{
		hasWeights = true;
		mesh.weightList = new OBWeightList();
		mesh.jointRefList = new OBJointRefList();
	}


	std::vector<_Tri> triangles;
	std::vector<Vertex> vertices;
	for (int f = 0; f < fbxMesh->GetPolygonCount(); f++)
	{
		int startIndex = fbxMesh->GetPolygonVertexIndex(f);
		int* vertexIDs = &fbxMesh->GetPolygonVertices()[startIndex];
		_Tri t;

		t.material = 0;
		if (elementMaterial)
		{
			if (elementMaterial->GetMappingMode() == FbxLayerElement::eByPolygon)
				t.material = elementMaterial->GetIndexArray()[f];
			else
				t.material = elementMaterial->GetIndexArray()[0];
		}

		for (int v = 0; v < 3; v++)
		{
			int vertexId = vertexIDs[v]; // Control Point index
			float vx = fbxMesh->GetControlPoints()[vertexId][0];
			float vy = fbxMesh->GetControlPoints()[vertexId][1];
			float vz = fbxMesh->GetControlPoints()[vertexId][2];

			float nx = 0.0f, ny = 0.0f, nz = 1.0f;
			if (hasNormals)
			{
				FbxVector4 n;
				fbxMesh->GetPolygonVertexNormal(f, v, n);
				nx = n[0];
				ny = n[1];
				nz = n[2];
			}

			float tx = 0.0f, ty = 0.0f;
			if (hasUVs)
			{
				FbxVector2 t;
				bool unmappedUV;
				fbxMesh->GetPolygonVertexUV(f, v, elementUVName, t, unmappedUV);
				tx = t[0];
				ty = 1.0 - t[1];
			}

			float r = 0.0f, g = 0.0f, b = 0.0f, a = 1.0f;
			if (hasColors)
			{
				/* I hope I'm doing this correctly... */
				if (elementColor->GetMappingMode() == FbxGeometryElement::eByControlPoint)
				{
					FbxColor c(0.0f, 0.0f, 0.0f, 1.0f);
					if (elementColor->GetReferenceMode() == FbxGeometryElement::eDirect)
						c = elementColor->GetDirectArray().GetAt(vertexId);

					r = c[0];
					g = c[1];
					b = c[2];
					a = c[3];
				}
			}

			Vertex vert(vx, vy, vz, tx, ty, nx, ny, nz, r, g, b, a);
			if (hasWeights)
			{
				if (skin->GetClusterCount() > 0)
				{
					for (int i = 0; i < skin->GetClusterCount(); i++)
					{
						FbxCluster* cluster = skin->GetCluster(i);
						for (int j = 0; j < cluster->GetControlPointIndicesCount(); j++)
						{
							int vId = cluster->GetControlPointIndices()[j];
							if (vId == vertexId)
								vert.weights.push_back({(uint32_t) i, (float) cluster->GetControlPointWeights()[j]});
						}
					}
				}
			}

			if (vert.weights.size() > 4)
			{
				printf("Warning: There are more than 4 joints that influence a single vertex. Attempting to fix that.\n");
				
				std::sort(vert.weights.begin(), vert.weights.end(), 
					[](const OBWeight& a, const OBWeight& b) -> bool
				{ 
					return a.weight > b.weight;
				});

				vert.weights.resize(4);
			}
			
			float weightSum = 0.0f;
			for (int i = 0; i < vert.weights.size(); i++)
				weightSum += vert.weights[i].weight;
			for (int i = 0; i < vert.weights.size(); i++)
				vert.weights[i].weight = (vert.weights[i].weight / weightSum) * 100.0f;

			bool found = false;
			uint32_t fVert = 0;
			for (int i = 0; i < vertices.size(); i++)
			{
				if (vertices[i] == vert)
				{
					found = true;
					fVert = i;
					break;
				}
			}
			if (found)
			{
				t.tri.idx[v] = fVert;
			}
			else
			{
				vertices.push_back(vert);
				t.tri.idx[v] = vertices.size()-1;
			}
		}
		triangles.push_back(t);
	}

	StripGroupList sgList;
	ConvertMeshToTriangleStrips(sdkManager, sgList, node, triangles, vertices);

	mesh.primLists = new OBPrimLists();
	mesh.primLists->lists.resize(sgList.size());
	for (int i = 0; i < sgList.size(); i++)
	{
		OBTStripList* tsl = &mesh.primLists->lists[i];
		tsl->type = (i == 0 ? OBType::TStripListFW : OBType::TStripListPW);
		tsl->list.resize(sgList[i].size());
		for (int j = 0; j < sgList[i].size(); j++)
		{
			tsl->list[j].leftHand = sgList[i][j].leftHand;
			tsl->list[j].indices.resize(sgList[i][j].indices.size());
			for (int t = 0; t < sgList[i][j].indices.size(); t++)
				tsl->list[j].indices[t] = sgList[i][j].indices[t];
		}
	}

	mesh.materialRefList = new OBMaterialRefList();
	mesh.materialRefList->list.resize(node->GetMaterialCount());
	for (int i = 0; i < node->GetMaterialCount(); i++)
		mesh.materialRefList->list[i] = (int) node->GetMaterial(i)->GetUserDataPtr();

	int totalStripCount = sgList[0].size() + (sgList.size() > 1 ? sgList[1].size() : 0);
	mesh.tStripMaterialList = new OBTStripMaterialList();
	mesh.tStripMaterialList->list.reserve(totalStripCount);
	for (int i = 0; i < sgList[0].size(); i++)
		mesh.tStripMaterialList->list.push_back(sgList[0][i].material);
	if (sgList.size() > 1)
		for (int i = 0; i < sgList[1].size(); i++)
			mesh.tStripMaterialList->list.push_back(sgList[1][i].material);

	mesh.vertexList = new OBVertexList();
	mesh.vertexList->list.resize(vertices.size());
	for (int i = 0; i < vertices.size(); i++)
	{
		mesh.vertexList->list[i].x = vertices[i].x;
		mesh.vertexList->list[i].y = vertices[i].y;
		mesh.vertexList->list[i].z = vertices[i].z;
	}

	
	if (hasNormals)
	{
		mesh.normalList->list.resize(vertices.size());
		for (int i = 0; i < vertices.size(); i++)
		{
			mesh.normalList->list[i].x = vertices[i].nx;
			mesh.normalList->list[i].y = vertices[i].ny;
			mesh.normalList->list[i].z = vertices[i].nz;
		}
	}

	if (hasUVs)
	{
		mesh.texCoordList->list.resize(vertices.size());
		for (int i = 0; i < vertices.size(); i++)
		{
			mesh.texCoordList->list[i].u = vertices[i].tx;
			mesh.texCoordList->list[i].v = vertices[i].ty;
		}
	}
	
	mesh.colorList->list.resize(vertices.size());
	for (int i = 0; i < vertices.size(); i++)
	{
		mesh.colorList->list[i].r = vertices[i].r;
		mesh.colorList->list[i].g = vertices[i].g;
		mesh.colorList->list[i].b = vertices[i].b;
		mesh.colorList->list[i].a = vertices[i].a;
	}

	if (hasWeights)
	{		
		if (skin->GetClusterCount() > 0)
		{
			mesh.weightList->list.resize(vertices.size());
			for (int i = 0; i < vertices.size(); i++)
			{
				mesh.weightList->list[i].resize(vertices[i].weights.size());
				for (int j = 0; j < vertices[i].weights.size(); j++)
					mesh.weightList->list[i][j] = vertices[i].weights[j];
			}
			
			mesh.jointRefList->list.resize(skin->GetClusterCount());
			for (int i = 0; i < skin->GetClusterCount(); i++)
				mesh.jointRefList->list[i] = (int) skin->GetCluster(i)->GetLink()->GetUserDataPtr();
		}
	}
	FillRenderAttributes(mesh, node);
}

static void FillMeshes(FbxManager* sdkManager, OBModel& model, FbxScene* scene, std::vector<FbxMesh*>& meshes)
{
	model.meshList = new OBMeshList();
	model.meshList->list.resize(meshes.size());
	for (int i = 0; i < meshes.size(); i++)
	{
		int id = (int) meshes[i]->GetUserDataPtr();
		FillMesh(sdkManager, model.meshList->list[id], meshes[i]->GetNode());
	}
}

static void NumberMeshes(FbxScene* scene, std::vector<FbxMesh*>& meshes)
{
	int id = 0;
	for (int i = 0; i < scene->GetGeometryCount(); i++)
	{
		FbxGeometry* geometry = scene->GetGeometry(i);
		if (geometry->GetAttributeType() == FbxNodeAttribute::eMesh)
		{
			int idToUse = id;
			FbxNode* node = geometry->GetNode();
			std::string name = node->GetName();
			if (name.substr(0, 4) == "Mesh")
			{
				idToUse = atoi(name.substr(4).c_str());
			}

			geometry->SetUserDataPtr((void*) idToUse);
			meshes.push_back((FbxMesh*)geometry);
			id++;
		}
		else
		{
			printf("[ConvertToNBD] Found geometry that is not a mesh. Skipping.\n");
		}
	}
}

static void FillHierarchy(OBHierarchy& hie, FbxScene* scene, std::vector<FbxNode*>& nodes)
{
	int id = 0;
	FbxNode* node = scene->FindNodeByName("Node0");
	while (node)
	{
		node->SetUserDataPtr((void*) id);
		nodes.push_back(node);
		id++;
		node = scene->FindNodeByName((std::string("Node") + std::to_string(id)).c_str());
	}

	FbxNode* rootNode = scene->GetRootNode();
	hie.nodeList.resize(nodes.size());

	for (int i = 0; i < nodes.size(); i++)
	{
		node = nodes[i];
		OBNode* obNode = &hie.nodeList[i];
		obNode->localId = i;
		obNode->nodeList = &hie.nodeList[0];
		if (node->GetParent() == rootNode)
		{
			obNode->parent = NULL;
			hie.roots.push_back(obNode);
		}
		else
		{
			obNode->parent = &hie.nodeList[(int)node->GetParent()->GetUserDataPtr()];
		}

		if (node->GetChildCount() > 0)
			obNode->child = &hie.nodeList[(int)node->GetChild(0)->GetUserDataPtr()];
		for (int j = 0; j < node->GetChildCount(); j++)
		{
			id = (int)node->GetChild(j)->GetUserDataPtr();
			if (j == node->GetChildCount()-1)
			{
				hie.nodeList[id].next = NULL;
			}
			else
			{
				hie.nodeList[id].next = &hie.nodeList[(int)node->GetChild(j+1)->GetUserDataPtr()];
			}
		}

		int groupId = 0;
		FbxProperty groupProperty = node->FindProperty("GroupId", false);
		if (groupProperty.IsValid())
		{
			groupId = groupProperty.Get<FbxInt>();
		}
		else
		{
			printf("Did not find group id on a node %d\n", i);
		}

		obNode->groupId = groupId;
		obNode->meshId = -1;
		if (node->GetNodeAttribute())
		{
			if (node->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eSkeleton)
			{
				obNode->type = OBType::NodeJoint;
			}
			else if (node->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eMesh)
			{
				obNode->type = OBType::NodeMesh;
				obNode->groupId = -1;
				obNode->meshId = (int) node->GetNodeAttribute()->GetUserDataPtr();
			}
			else
			{
				obNode->type = OBType::Node;
			}
		}
		else
		{
			obNode->type = OBType::Node;
		}

		FbxAMatrix matrix = node->EvaluateLocalTransform();
		FbxAMatrix parentMatrix = node->GetParent()->EvaluateGlobalTransform();
		parentMatrix.SetT({0, 0, 0, 1});
		
		FbxVector4 t = parentMatrix.Inverse() * matrix.GetT();
		FbxVector4 r = matrix.GetR();
		FbxVector4 s = matrix.GetS();
		
		obNode->transform.translation[0] = t[0];
		obNode->transform.translation[1] = t[1];
		obNode->transform.translation[2] = t[2];
		obNode->transform.translation[3] = 1.0f;
		
		obNode->transform.rotation[0] = (r[0] / 180.0f) * M_PI;
		obNode->transform.rotation[1] = (r[1] / 180.0f) * M_PI;
		obNode->transform.rotation[2] = (r[2] / 180.0f) * M_PI;
		obNode->transform.rotation[3] = 1.0f;

		obNode->transform.scale[0] = s[0];
		obNode->transform.scale[1] = s[1];
		obNode->transform.scale[2] = s[2];
		obNode->transform.scale[3] = 1.0f;
	}
}


static void FillNbd(FbxManager* sdkManager, OBNbd& nbd, FbxScene* scene, const char* fbxFilename)
{
	std::vector<TextureSize> textureSizes;
	std::vector<FbxMesh*> meshes;
	std::vector<FbxNode*> nodes;
	ConvertTextures(nbd, scene, fbxFilename, textureSizes);

	AddDate(nbd.amo);
	FillTextures(nbd.amo, scene, textureSizes);
	FillMaterials(nbd.amo, scene);
	NumberMeshes(scene, meshes);
	FillHierarchy(nbd.ahi, scene, nodes);
	FillMeshes(sdkManager, nbd.amo, scene, meshes);
}

int CmdConvertToNBD(int count, char** argv)
{
	if (count != 2)
	{
		fprintf(stderr, "[ConvertToNBD] Incorrect amount of arguments\n");
		return 0;
	}

	char* f_nbd = argv[1];
	char* f_fbx = argv[0];

	FbxManager* sdkManager = FbxManager::Create();
	FbxIOSettings* ios = FbxIOSettings::Create(sdkManager, IOSROOT);
	sdkManager->SetIOSettings(ios);

	FbxImporter* importer = FbxImporter::Create(sdkManager, "");
	bool importStatus = importer->Initialize(f_fbx, -1, sdkManager->GetIOSettings());
	if (!importStatus) {
		fprintf(stderr, "Call to FbxImporter::Initialize() failed.\n");
		fprintf(stderr, "Error returned: %s\n\n", importer->GetStatus().GetErrorString());
		return 0;
	}
	FbxScene* scene = FbxScene::Create(sdkManager, "nbdScene");
	importer->Import(scene);

	OBNbd nbd;
	FillNbd(sdkManager, nbd, scene, f_fbx);

	std::filesystem::path path(f_nbd);
	std::string output = path.parent_path().string();
	if (output.size() != 0)
		output = output + "/";
	output = output + path.stem().string();
	std::ofstream nbdStream(output + ".nbd", std::ios_base::binary);
	if (nbdStream.fail())
	{
		fprintf(stderr, "[ConvertToNBD] Error opening \"%s\" (%s)\n", (output + ".nbd").c_str(), strerror(errno));
		return 0;
	}
	nbd.write(nbdStream);
	nbdStream.close();


	importer->Destroy();
	sdkManager->Destroy();
	return 1;
}