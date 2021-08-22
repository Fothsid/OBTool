#include "commands.h"
#include "tim2utils.h"
#include <filesystem>
#include <string>
#include <fstream>
#include <vector>
#include <cmath>
#include <thanatos.h>
#include "fbx.h"

#define M_PI 3.14159265358979323846264338327950288

struct Tri
{
	int idx[3];
};

void CreateTextures(std::string output, FbxScene* scene, OBTextureList* texList, 
							std::vector<FbxFileTexture*>& textures)
{
	for (int i = 0; i < texList->list.size(); i++)
	{
		std::string fileName = output + ".texture" + std::to_string(texList->list[i].id) + ".png";
		std::string texName = std::string("Texture") + std::to_string(i);

		FbxFileTexture* texture = FbxFileTexture::Create(scene, texName.c_str());

		texture->SetFileName(fileName.c_str());
		texture->SetTextureUse(FbxTexture::eStandard);
		texture->SetMappingType(FbxTexture::eUV);
		texture->SetMaterialUse(FbxFileTexture::eModelMaterial);
		texture->SetAlphaSource(FbxTexture::eBlack);
		texture->SetSwapUV(false);
		texture->SetTranslation(0.0, 0.0);
		texture->SetScale(1.0, 1.0);
		texture->SetRotation(0.0, 0.0);

		textures[i] = texture;
	}
}

void CreateMaterials(std::string output, FbxScene* scene, OBMaterialList* matList, 
					std::vector<FbxSurfaceLambert*>& materials,
					std::vector<FbxFileTexture*>& textures)
{
	for (int i = 0; i < matList->list.size(); i++)
	{
		OBMaterial* m = &matList->list[i];

		std::string matName = std::string("Material") + std::to_string(i);
		FbxSurfaceLambert* material = FbxSurfaceLambert::Create(scene, matName.c_str());
		
		FbxDouble3 colorBlack(0.0, 0.0, 0.0);
		FbxDouble3 ambient(m->ambient.r, m->ambient.g, m->ambient.b);
		FbxDouble3 diffuse(m->diffuse.r, m->diffuse.g, m->diffuse.b);
		FbxDouble3 specular(m->specular.r, m->specular.g, m->specular.b);

		material->Emissive.Set(colorBlack);
		
		material->Ambient.Set(ambient);
		material->AmbientFactor.Set(1.0f);

		material->Diffuse.Set(diffuse);
		material->DiffuseFactor.Set(1.0f);
		material->TransparencyFactor.Set(1.0f - m->diffuse.a);
		material->Diffuse.ConnectSrcObject(textures[m->textures[0]]);
		
		/*material->Specular.Set(specular);
		material->SpecularFactor.Set(1.0f);
		material->Shininess.Set(m->specularDecay);*/
		
		material->ShadingModel.Set("Lambert");

		materials[i] = material;
	}
}

static void SortOutDuplicateVertices(std::vector<OBVertex>& vertices, std::vector<std::vector<OBWeight>>& weights, 
									 OBVertexList* vertexList, OBWeightList* weightList)
{
	for (int i = 0; i < vertexList->list.size(); i++)
	{
		bool found = false;
		for (int j = 0; j < vertices.size(); j++)
		{
			if (vertices[j] == vertexList->list[i] &&
				weights[j] == weightList->list[i])
			{
				found = true;
				break;
			}
		}
		if (!found)
		{
			vertices.push_back(vertexList->list[i]);
			weights.push_back(weightList->list[i]);
		}
	}
}

template <typename T, typename L>
static void SortOutDuplicates(std::vector<T>& result, L* list)
{
	for (int i = 0; i < list->list.size(); i++)
	{
		bool found = false;
		for (int j = 0; j < result.size(); j++)
		{
			if (result[j] == list->list[i])
			{
				found = true;
				break;
			}
		}
		if (!found)
			result.push_back(list->list[i]);
	}
}

template <typename T>
static int FindElementIndex(std::vector<T>& list, T value)
{
	int foundId = -1;
	for (int i = 0; i < list.size(); i++)
	{
		if (value == list[i])
		{
			foundId = i;
			break;
		}
	}
	return foundId;
}

void AddTStripListToMesh(FbxMesh* mesh, std::vector<OBVertex>& originalVertices, 
						 std::vector<OBVertex>& vertices, std::vector<Tri>& triangles, 
						 OBTStripList* list, int start, OBTStripMaterialList* mat)
{
	for (int tsi = 0; tsi < list->list.size(); tsi++)
	{
		OBTStrip* ts = &list->list[tsi];
		for (uint32_t t = 0; t < ts->indices.size()-2; t++)
		{
			Tri triangle;
			if (t & 1)
			{
				triangle.idx[0] = ts->indices[t+0];
				triangle.idx[1] = ts->indices[t+2];
				triangle.idx[2] = ts->indices[t+1];
			}
			else
			{
				triangle.idx[0] = ts->indices[t+0];
				triangle.idx[1] = ts->indices[t+1];
				triangle.idx[2] = ts->indices[t+2];
			}

			triangles.push_back(triangle);

			mesh->BeginPolygon(mat->list[start+tsi]);
			mesh->AddPolygon(FindElementIndex(vertices, originalVertices[triangle.idx[0]]));
			mesh->AddPolygon(FindElementIndex(vertices, originalVertices[triangle.idx[1]]));
			mesh->AddPolygon(FindElementIndex(vertices, originalVertices[triangle.idx[2]]));
			mesh->EndPolygon();
		}
	}
}

void CreateRenderAttribs(FbxScene* scene, FbxNode* meshNode, OBRenderAttribs* attribs)
{
	int32_t* attr;
	if (attribs)
		attr = (int32_t*) &attribs->attribs.version;

	int id = 0;
	FbxProperty p[18];
	p[id++] = FbxProperty::Create(meshNode, FbxBoolDT, "RA_EnableRenderAttribs", "Enable Render Attributes");
	p[id++] = FbxProperty::Create(meshNode, FbxIntDT,  "RA_Material",            "Attribs.Material");
	p[id++] = FbxProperty::Create(meshNode, FbxIntDT,  "RA_Specular",            "Attribs.Specular");
	p[id++] = FbxProperty::Create(meshNode, FbxIntDT,  "RA_Cull",                "Attribs.Cull");
	p[id++] = FbxProperty::Create(meshNode, FbxIntDT,  "RA_Scissor",             "Attribs.Scissor");
	p[id++] = FbxProperty::Create(meshNode, FbxIntDT,  "RA_Light",               "Attribs.Light");
	p[id++] = FbxProperty::Create(meshNode, FbxIntDT,  "RA_Wrap",                "Attribs.Wrap");
	p[id++] = FbxProperty::Create(meshNode, FbxIntDT,  "RA_UVScroll",            "Attribs.UVScroll");
	p[id++] = FbxProperty::Create(meshNode, FbxIntDT,  "RA_DisableFog",          "Attribs.DisableFog");
	p[id++] = FbxProperty::Create(meshNode, FbxIntDT,  "RA_FadeColor",           "Attribs.FadeColor");
	p[id++] = FbxProperty::Create(meshNode, FbxIntDT,  "RA_SpecialShader",       "Attribs.SpecialShader");
	p[id++] = FbxProperty::Create(meshNode, FbxIntDT,  "RA_AlphaSrc",            "Attribs.AlphaSrc");
	p[id++] = FbxProperty::Create(meshNode, FbxIntDT,  "RA_AlphaDst",            "Attribs.AlphaDst");
	p[id++] = FbxProperty::Create(meshNode, FbxIntDT,  "RA_AlphaOp",             "Attribs.AlphaOp");
	p[id++] = FbxProperty::Create(meshNode, FbxIntDT,  "RA_DepthOp",             "Attribs.DepthOp");
	p[id++] = FbxProperty::Create(meshNode, FbxBoolDT, "RA_DepthWrite",          "Attribs.DepthWrite");
	p[id++] = FbxProperty::Create(meshNode, FbxIntDT,  "RA_Filtering",           "Attribs.Filtering");
	p[id++] = FbxProperty::Create(meshNode, FbxIntDT,  "RA_AddressMode",         "Attribs.AddressMode");
	
	for (int i = 0; i < 18; i++)
		p[i].ModifyFlag(FbxPropertyFlags::eUserDefined, true);
	
	if (attribs)
	{
		p[0].Set(true);
		for (int i = 1; i < 18; i++)
			p[i].Set(attr[i]);
	}
	else
	{
		p[0].Set(false);
	}
}

void CreateMesh(FbxScene* scene, const char* name, OBMesh* m, FbxNode* meshNode,
				std::vector<FbxSurfaceLambert*>& materials, 
				std::vector<FbxNode*>& joints)
{
	FbxMesh* mesh = FbxMesh::Create(scene, name);

	CreateRenderAttribs(scene, meshNode, m->renderAttribs);

	FbxLayer* layer = mesh->GetLayer(0);
	if (layer == NULL)
	{
		mesh->CreateLayer();
		layer = mesh->GetLayer(0);
	}

	/*
	 *
	 *	Vertices
	 *
	 */
	std::vector<OBVertex> vertices;
	std::vector<std::vector<OBWeight>> weights;
	if (m->jointRefList && m->weightList)
		SortOutDuplicateVertices(vertices, weights, m->vertexList, m->weightList);
	else
		SortOutDuplicates(vertices, m->vertexList);
	mesh->InitControlPoints(vertices.size());
	FbxVector4* controlPoints = mesh->GetControlPoints();
	for (int i = 0; i < vertices.size(); i++)
	{
		OBVertex v = vertices[i];
		controlPoints[i].Set(v.x, v.y, v.z);
	}

	/*
	 *
	 *	Triangle strips
	 *
	 */
	FbxGeometryElementMaterial* materialElement = mesh->CreateElementMaterial();
	materialElement->SetMappingMode(FbxGeometryElement::eByPolygon);
	materialElement->SetReferenceMode(FbxGeometryElement::eIndexToDirect);

	OBTStripMaterialList* tsml = m->tStripMaterialList;
	std::vector<Tri> triangles;

	AddTStripListToMesh(mesh, m->vertexList->list, vertices, triangles, &m->primLists->lists[0], 0, m->tStripMaterialList);
	if (m->primLists->lists.size() > 1)
		AddTStripListToMesh(mesh, m->vertexList->list, vertices, triangles, &m->primLists->lists[1], m->primLists->lists[0].list.size(), m->tStripMaterialList);

	for (int i = 0; i < m->materialRefList->list.size(); i++)
		meshNode->AddMaterial(materials[m->materialRefList->list[i]]);

	/*
	 *
	 *	Normals
	 *
	 */
	if (m->normalList)
	{
		std::vector<OBNormal> normals;
		SortOutDuplicates(normals, m->normalList);

		FbxLayerElementNormal* normalElement = FbxLayerElementNormal::Create(mesh, "");
		normalElement->SetMappingMode(FbxGeometryElement::eByPolygonVertex);
		normalElement->SetReferenceMode(FbxGeometryElement::eIndexToDirect);
		for (int i = 0; i < normals.size(); i++)
		{
			OBNormal v = normals[i];
			FbxVector4 n(v.x, v.y, v.z);
			normalElement->GetDirectArray().Add(n);
		}
		for (int i = 0; i < triangles.size(); i++)
		{
			Tri t = triangles[i];
			for (int j = 0; j < 3; j++)
				normalElement->GetIndexArray().Add(FindElementIndex(normals, m->normalList->list[t.idx[j]]));
		}
		layer->SetNormals(normalElement);
	}

	/*
	 *
	 *	Texture Coordinates
	 *
	 */

	if (m->texCoordList)
	{
		std::vector<OBTexCoord> texCoords;
		SortOutDuplicates(texCoords, m->texCoordList);

		FbxLayerElementUV* uvElement = FbxLayerElementUV::Create(mesh, "");
		uvElement->SetMappingMode(FbxGeometryElement::eByPolygonVertex);
		uvElement->SetReferenceMode(FbxGeometryElement::eIndexToDirect);
		for (int i = 0; i < texCoords.size(); i++)
		{
			OBTexCoord v = texCoords[i];
			FbxVector2 tc(v.u, 1.0f - v.v);
			uvElement->GetDirectArray().Add(tc);
		}
		for (int i = 0; i < triangles.size(); i++)
		{
			Tri t = triangles[i];
			for (int j = 0; j < 3; j++)
				uvElement->GetIndexArray().Add(FindElementIndex(texCoords, m->texCoordList->list[t.idx[j]]));
		}
		layer->SetUVs(uvElement);
	}

	/*
	 *
	 *	Color Vertices
	 *
	 */

	if (m->colorList)
	{
		std::vector<OBColor> colors;
		SortOutDuplicates(colors, m->colorList);

		FbxLayerElementVertexColor* vertexColorElement = FbxLayerElementVertexColor::Create(mesh, "");
		vertexColorElement->SetMappingMode(FbxGeometryElement::eByPolygonVertex);
		vertexColorElement->SetReferenceMode(FbxGeometryElement::eIndexToDirect);
		for (int i = 0; i < colors.size(); i++)
		{
			OBColor v = colors[i];
			FbxColor c(v.r, v.g, v.b, v.a);
			vertexColorElement->GetDirectArray().Add(c);
		}
		for (int i = 0; i < triangles.size(); i++)
		{
			Tri t = triangles[i];
			for (int j = 0; j < 3; j++)
				vertexColorElement->GetIndexArray().Add(FindElementIndex(colors, m->colorList->list[t.idx[j]]));
		}
		layer->SetVertexColors(vertexColorElement);
	}


	/*
	 *
	 *	Joint Weight
	 *
	 */
	if (m->jointRefList && m->weightList)
	{
		FbxSkin* skin = FbxSkin::Create(scene, "");
		mesh->AddDeformer(skin);

		for (int j = 0; j < m->jointRefList->list.size(); j++)
		{
			/* This bound check is needed because apparently in OBJSxx.NBD bone ids go out of bounds..? */
			if (joints.size() > m->jointRefList->list[j])
			{
				FbxCluster* cluster = FbxCluster::Create(scene,"");
				cluster->SetLink(joints[m->jointRefList->list[j]]);
				cluster->SetLinkMode(FbxCluster::eTotalOne);
				for (int v = 0; v < weights.size(); v++)
				{
					for (int b = 0; b < weights[v].size(); b++)
					{
						if (weights[v][b].id == j)
							cluster->AddControlPointIndex(v, weights[v][b].weight / 100.0f);
					}
				}
				cluster->SetTransformLinkMatrix(joints[m->jointRefList->list[j]]->EvaluateGlobalTransform());
				skin->AddCluster(cluster);
			}
		}
	}

	meshNode->SetNodeAttribute(mesh);

}

void CreateMeshes(FbxScene* scene, OBMeshList* meshList, 
				  std::vector<FbxSurfaceLambert*>& materials, 
				  std::vector<FbxNode*>& joints,
				  std::vector<FbxNode*>& meshes)
{
	FbxNode* rootNode = scene->GetRootNode();
	for (int i = 0; i < meshList->list.size(); i++)
	{
		if (meshes[i] == NULL)
		{
			OBMesh* m = &meshList->list[i];
			std::string meshName = std::string("Mesh") + std::to_string(i);

			FbxNode* meshNode = FbxNode::Create(scene, meshName.c_str());
			CreateMesh(scene, meshName.c_str(), m, meshNode, materials, joints);

			meshes[i] = meshNode;
			rootNode->AddChild(meshNode);
		}
	}
}

void AddChildrenToHierarchy(std::vector<FbxNode*>& joints, OBNode* node)
{
	if (node->next)
	{
		joints[node->parent->localId]->AddChild(joints[node->next->localId]);
		AddChildrenToHierarchy(joints, node->next);
	}

	if (node->child)
	{
		joints[node->localId]->AddChild(joints[node->child->localId]);
		AddChildrenToHierarchy(joints, node->child);
	}
}

void CreateSkeleton(FbxScene* scene, OBHierarchy* hie, OBMeshList* meshList, 
					std::vector<FbxSurfaceLambert*>& materials,
					std::vector<FbxNode*>& joints,
					std::vector<FbxNode*>& meshes)
{
	FbxNode* rootNode = scene->GetRootNode();
	for (int i = 0; i < hie->nodeList.size(); i++)
	{
		std::string nodeName = std::string("Node") + std::to_string(hie->nodeList[i].localId);
		joints[i] = FbxNode::Create(scene, nodeName.c_str());
	}

	for (int i = 0; i < hie->roots.size(); i++)
	{
		rootNode->AddChild(joints[hie->roots[i]->localId]);
		AddChildrenToHierarchy(joints, hie->roots[i]);
	}

	for (int i = 0; i < hie->nodeList.size(); i++)
	{
		OBNode* node = &hie->nodeList[i];
		FbxNode* fNode = joints[i];

		fNode->SetUserDataPtr((void*) node);

		FbxProperty groupProperty = FbxProperty::Create(fNode, FbxIntDT, "GroupId", "");
		groupProperty.ModifyFlag(FbxPropertyFlags::eUserDefined, true);
		groupProperty.Set(node->groupId);

		switch (node->getType())
		{
			case OBType::NodeJoint:
			{
				FbxSkeleton* skeletonAttr = FbxSkeleton::Create(scene, "");
				skeletonAttr->SetSkeletonType(FbxSkeleton::eLimbNode);
				fNode->SetNodeAttribute(skeletonAttr);
			} break;
			case OBType::NodeMesh:
			{
				if (node->meshId >= 0 && meshes.size() > node->meshId)
				{
					OBMesh* m = &meshList->list[node->meshId];
					std::string meshName = std::string("Mesh") + std::to_string(node->meshId);
					CreateMesh(scene, meshName.c_str(), m, fNode, materials, joints);
					meshes[node->meshId] = fNode;
				}
			} break;
			default: break;
		}

		/*
			Force node to be part of a skeleton if the parent has a skeleton attribute
			and the current node does not.

			Basically a hacky way to make Maya correctly import nodes like Node22.
		*/
		if (node->parent && node->parent->getType() == OBType::NodeJoint && node->getType() == OBType::Node)
		{
			FbxSkeleton* skeletonAttr = FbxSkeleton::Create(scene, "");
			skeletonAttr->SetSkeletonType(FbxSkeleton::eLimbNode);
			fNode->SetNodeAttribute(skeletonAttr);
		}

		fNode->LclTranslation.Set(FbxVector4(node->transform.translation[0], 
											 node->transform.translation[1], 
											 node->transform.translation[2]));

		joints[i] = fNode;
	}

	/* Hacky way to fix models with rotated bones */
	for (int i = 0; i < hie->nodeList.size(); i++)
	{
		FbxNode* fNode = joints[i];

		if (fNode->GetSkeleton() && fNode->GetParent()->GetUserDataPtr())
		{
			OBNode* node = (OBNode*) fNode->GetParent()->GetUserDataPtr();
			FbxAMatrix parentMatrix = fNode->GetParent()->EvaluateGlobalTransform();
			parentMatrix.SetTRS(FbxVector4(node->transform.translation[0], 
										   node->transform.translation[1], 
										   node->transform.translation[2]),
								FbxVector4((node->transform.rotation[0] / M_PI) * 180.0f,
										   (node->transform.rotation[1] / M_PI) * 180.0f,
										   (node->transform.rotation[2] / M_PI) * 180.0f),
								FbxVector4(node->transform.scale[0], 
										   node->transform.scale[1], 
										   node->transform.scale[2]));
			while (node->parent)
			{
				node = node->parent;
				FbxAMatrix matrix;
				matrix.SetTRS(FbxVector4(node->transform.translation[0], 
										 node->transform.translation[1], 
										 node->transform.translation[2]),
							  FbxVector4((node->transform.rotation[0] / M_PI) * 180.0f,
										 (node->transform.rotation[1] / M_PI) * 180.0f,
										 (node->transform.rotation[2] / M_PI) * 180.0f),
							  FbxVector4(node->transform.scale[0], 
										 node->transform.scale[1], 
										 node->transform.scale[2]));
				parentMatrix *= matrix;
			}
			fNode->LclTranslation.Set(parentMatrix * fNode->LclTranslation.Get());
		}
	}
}

static void RecursivelyAddPoseBinds(FbxScene* scene, FbxPose* pose, FbxNode* node, std::vector<FbxNode*>& joints)
{
	FbxMatrix matrix = node->EvaluateGlobalTransform();
	pose->Add(node, matrix);
	FbxNode* parent = node->GetParent();
	if (parent && parent != scene->GetRootNode())
		RecursivelyAddPoseBinds(scene, pose, parent, joints);
}

static void CreateBindPoses(FbxScene* scene, OBHierarchy* hie, std::vector<FbxNode*>& meshes, std::vector<FbxNode*>& joints)
{
	for (int i = 0; i < meshes.size(); i++)
	{
		std::string poseName = std::string("bindPoseMesh") + std::to_string(i);
		FbxMesh* mesh = (FbxMesh*) meshes[i]->GetNodeAttribute();
		FbxSkin* skin = (FbxSkin*) mesh->GetDeformer(0);
		if (skin)
		{
			FbxPose* pose = FbxPose::Create(scene, poseName.c_str());
			pose->SetIsBindPose(true);
			pose->Add(meshes[i], meshes[i]->EvaluateGlobalTransform());
			
			for (int c = 0; c < skin->GetClusterCount(); c++)
			{
				FbxNode* node = skin->GetCluster(c)->GetLink();
				bool found = false;
				int id = 0;
				for (int i = 0; i < joints.size(); i++)
				{
					if (joints[i] == node)
					{
						found = true;
						id = i;
						break;
					}
				}
				RecursivelyAddPoseBinds(scene, pose, node, joints);
			}
			scene->AddPose(pose);
		}
	}

}

static void FillScene(std::string output, FbxScene* scene, OBNbdScene* nbdScene, bool isRoom)
{
	std::vector<FbxNode*> joints(nbdScene->hierarchy->nodeList.size());
	std::vector<FbxFileTexture*> textures(nbdScene->model->textureList->list.size());
	std::vector<FbxSurfaceLambert*> materials(nbdScene->model->materialList->list.size());
	std::vector<FbxNode*> meshes(nbdScene->model->meshList->list.size());
	CreateTextures(output, scene, nbdScene->model->textureList, textures);
	CreateMaterials(output, scene, nbdScene->model->materialList, materials, textures);
	CreateSkeleton(scene, nbdScene->hierarchy, nbdScene->model->meshList, materials, joints, meshes);
	CreateMeshes(scene, nbdScene->model->meshList, materials, joints, meshes);
	if (!isRoom)
		CreateBindPoses(scene, nbdScene->hierarchy, meshes, joints);

	for (int i = 0; i < joints.size(); i++)
	{
		OBNode* node = &nbdScene->hierarchy->nodeList[i];
		joints[i]->LclRotation.Set(FbxVector4((node->transform.rotation[0] / M_PI) * 180.0f,
											  (node->transform.rotation[1] / M_PI) * 180.0f,
											  (node->transform.rotation[2] / M_PI) * 180.0f));
		joints[i]->LclScaling.Set(FbxVector4(node->transform.scale[0],
											 node->transform.scale[1],
											 node->transform.scale[2]));
	}
}

static int OutputTextures(std::string output, std::vector<OBNbdTexture>& textures)
{
	int r = 1;
	for (int i = 0; i < textures.size(); i++)
	{
		std::string fileName = output + ".texture" + std::to_string(i) + ".png";
		printf("%s\n", fileName.c_str());
		r &= OutbreakTm2ToPng(textures[i].data.tim2Data, fileName.c_str());
	}
	return r;
}

int CmdConvertToFBX(int count, char** argv)
{
	if (count != 2)
	{
		fprintf(stderr, "[ConvertToFBX] Incorrect amount of arguments\n");
		return 0;
	}

	char* f_nbd = argv[0];
	char* f_fbx = argv[1];

	std::ifstream nbdStream(f_nbd, std::ios_base::binary);
	if (nbdStream.fail())
	{
		fprintf(stderr, "[ConvertToFBX] Error opening \"%s\" (%s)\n", f_nbd, strerror(errno));
		return 0;
	}

	OBNbd nbd;
	if (!nbd.read(nbdStream))
	{
		fprintf(stderr, "[ConvertToFBX] Error parsing the NBD file\n");
		return 0;
	}

	std::filesystem::path path(f_fbx);
	std::string output = path.parent_path().string();
	if (output.size() != 0)
		output = output + "/";
	output = output + path.stem().string();

	FbxManager* sdkManager = FbxManager::Create();
	FbxIOSettings * ios = FbxIOSettings::Create(sdkManager, IOSROOT);
	sdkManager->SetIOSettings(ios);
	FbxExporter* exporter = FbxExporter::Create(sdkManager, "");
	bool exportStatus = exporter->Initialize((output + ".fbx").c_str(), -1, sdkManager->GetIOSettings());
	if (!exportStatus)
	{
		fprintf(stderr, "Call to FbxExporter::Initialize() failed.\n");
		fprintf(stderr, "Error returned: %s\n\n", exporter->GetStatus().GetErrorString());
		return 0;
	}

	FbxScene* scene = FbxScene::Create(sdkManager, "nbdMainScene");
	FillScene(output, scene, &nbd.model, nbd.getType() == OBType::RoomNBD);
	exporter->Export(scene);
	if (!exportStatus)
	{
		fprintf(stderr, "Call to FbxExporter::Export() failed.\n");
		fprintf(stderr, "Error returned: %s\n\n", exporter->GetStatus().GetErrorString());
		return 0;
	}
	exporter->Destroy();

	if (nbd.addon.isPresent())
	{
		std::string name = "";
		if (nbd.getType() == OBType::RoomNBD)
			name = output + ".effect.fbx";
		else
			name = output + ".addon.fbx";

		exporter = FbxExporter::Create(sdkManager, "");
		exportStatus = exporter->Initialize(name.c_str(), -1, sdkManager->GetIOSettings());
		if (!exportStatus)
		{
			fprintf(stderr, "Call to FbxExporter::Initialize() failed.\n");
			fprintf(stderr, "Error returned: %s\n\n", exporter->GetStatus().GetErrorString());
			return 0;
		}

		FbxScene* addonScene = FbxScene::Create(sdkManager, "nbdAddonScene");
		FillScene(output, addonScene, &nbd.addon, nbd.getType() == OBType::RoomNBD);
		exporter->Export(addonScene);
		if (!exportStatus)
		{
			fprintf(stderr, "Call to FbxExporter::Export() failed.\n");
			fprintf(stderr, "Error returned: %s\n\n", exporter->GetStatus().GetErrorString());
			return 0;
		}
		exporter->Destroy();
	}

	sdkManager->Destroy();

	if (nbd.shadow.sdwData)
	{
		std::string name = output + ".sdw";
		FILE* fp = fopen(name.c_str(), "wb");
		fwrite(nbd.shadow.sdwData, nbd.shadow.size, 1, fp);
		fclose(fp);
	}

	if (nbd.getTextureStorageType() == OBType::TextureData)
		OutputTextures(output, nbd.textures);

	return 1;
}