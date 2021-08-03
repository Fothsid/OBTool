#include "commands.h"
#include "tim2utils.h"
#include <filesystem>
#include <string>
#include <fstream>
#include <vector>
#include <thanatos.h>

#include <cmath>

#define FBXSDK_SHARED
#include <fbxsdk.h>

#define M_PI 3.14159265358979323846264338327950288

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
					std::vector<FbxSurfacePhong*>& materials,
					std::vector<FbxFileTexture*>& textures)
{
	for (int i = 0; i < matList->list.size(); i++)
	{
		OBMaterial* m = &matList->list[i];

		std::string matName = std::string("Material") + std::to_string(i);
		FbxSurfacePhong* material = FbxSurfacePhong::Create(scene, matName.c_str());
		
		FbxDouble3 colorBlack(0.0, 0.0, 0.0);
		FbxDouble3 ambient(m->ambient.r, m->ambient.g, m->ambient.b);
		FbxDouble3 diffuse(m->diffuse.r, m->diffuse.g, m->diffuse.b);
		FbxDouble3 specular(m->specular.r, m->specular.g, m->specular.b);

		material->Emissive.Set(colorBlack);
		
		material->Ambient.Set(ambient);
		material->AmbientFactor.Set(m->ambient.a);

		material->Diffuse.Set(diffuse);
		material->DiffuseFactor.Set(m->diffuse.a);
		material->Diffuse.ConnectSrcObject(textures[m->textures[0]]);
		
		material->Specular.Set(specular);
		material->SpecularFactor.Set(m->specular.a);
		
		material->ShadingModel.Set("Phong");
		material->Shininess.Set(m->specularDecay);

		materials[i] = material;
	}
}

void AddTStripListToMesh(FbxMesh* mesh, OBTStripList* list, int start, OBTStripMaterialList* mat)
{
	for (int tsi = 0; tsi < list->list.size(); tsi++)
	{
		OBTStrip* ts = &list->list[tsi];
		for (uint32_t t = 0; t < ts->indices.size()-2; t++)
		{
			int ind[3];
			/*
				Ignoring the ts->leftHand flag, as using it results in broken normals
				Maybe I simply don't understand something?
			 */
			if (t & 1)
			{
				ind[0] = ts->indices[t+0];
				ind[1] = ts->indices[t+2];
				ind[2] = ts->indices[t+1];
			}
			else
			{
				ind[0] = ts->indices[t+0];
				ind[1] = ts->indices[t+1];
				ind[2] = ts->indices[t+2];
			}

			mesh->BeginPolygon(mat->list[start+tsi]);
			mesh->AddPolygon(ind[0]);
			mesh->AddPolygon(ind[1]);
			mesh->AddPolygon(ind[2]);
			mesh->EndPolygon();
		}
	}
}

void CreateRenderAttribs(FbxScene* scene, FbxNode* meshNode, OBRenderAttribs* attribs)
{
	int32_t* attr;
	if (attribs)
		attr = (int32_t*) &attribs->attribs.material;

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
				std::vector<FbxSurfacePhong*>& materials, 
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
	mesh->InitControlPoints(m->vertexList->list.size());
	FbxVector4* controlPoints = mesh->GetControlPoints();
	for (int j = 0; j < m->vertexList->list.size(); j++)
	{
		OBVertex v = m->vertexList->list[j];
		controlPoints[j].Set(v.x, v.y, v.z);
	}

	/*
	 *
	 *	Normals
	 *
	 */
	if (m->normalList)
	{
		FbxLayerElementNormal* normalElement = FbxLayerElementNormal::Create(mesh, "");
		normalElement->SetMappingMode(FbxGeometryElement::eByControlPoint);
		normalElement->SetReferenceMode(FbxGeometryElement::eDirect);
		for (int j = 0; j < m->normalList->list.size(); j++)
		{
			OBNormal v = m->normalList->list[j];
			FbxVector4 n(v.x, v.y, v.z);
			normalElement->GetDirectArray().Add(n);
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
		FbxLayerElementUV* uvElement = FbxLayerElementUV::Create(mesh, "");
		uvElement->SetMappingMode(FbxGeometryElement::eByControlPoint);
		uvElement->SetReferenceMode(FbxGeometryElement::eDirect);
		for (int j = 0; j < m->texCoordList->list.size(); j++)
		{
			OBTexCoord v = m->texCoordList->list[j];
			FbxVector2 t(v.u, 1.0f - v.v);
			uvElement->GetDirectArray().Add(t);
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
		FbxLayerElementVertexColor* vertexColorElement = FbxLayerElementVertexColor::Create(mesh, "");
		vertexColorElement->SetMappingMode(FbxGeometryElement::eByControlPoint);
		vertexColorElement->SetReferenceMode(FbxGeometryElement::eDirect);
		for (int j = 0; j < m->colorList->list.size(); j++)
		{
			OBColor v = m->colorList->list[j];
			FbxColor c(v.r, v.g, v.b, v.a);
			vertexColorElement->GetDirectArray().Add(c);
		}
		layer->SetVertexColors(vertexColorElement);
	}

	/*
	 *
	 *	Joint Weight
	 *
	 */
	if (m->jointRefList)
	{
		FbxSkin* skin = FbxSkin::Create(scene, "");
		mesh->AddDeformer(skin);

		for (int j = 0; j < m->jointRefList->list.size(); j++)
		{
			/* This bound check is needed because apparently in OBJSxx.NBD bone ids go out of bounds..? go out of bounds? */
			if (joints.size() > m->jointRefList->list[j])
			{
				FbxCluster* cluster = FbxCluster::Create(scene,"");
				cluster->SetLink(joints[m->jointRefList->list[j]]);
				cluster->SetLinkMode(FbxCluster::eTotalOne);
				for (int v = 0; v < m->weightList->list.size(); v++)
				{
					for (int b = 0; b < m->weightList->list[v].size(); b++)
					{
						if (m->weightList->list[v][b].bone == j)
							cluster->AddControlPointIndex(v, m->weightList->list[v][b].weight);
					}
				}
				cluster->SetTransformLinkMatrix(joints[m->jointRefList->list[j]]->EvaluateGlobalTransform());
				skin->AddCluster(cluster);
			}
		}
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

	AddTStripListToMesh(mesh, &m->primLists->lists[0], 0, m->tStripMaterialList);
	if (m->primLists->lists.size() > 1)
		AddTStripListToMesh(mesh, &m->primLists->lists[1], m->primLists->lists[0].list.size(), m->tStripMaterialList);

	for (int j = 0; j < m->materialRefList->list.size(); j++)
		meshNode->AddMaterial(materials[m->materialRefList->list[j]]);

	meshNode->SetNodeAttribute(mesh);

}

void CreateMeshes(FbxScene* scene, OBMeshList* meshList, 
				  std::vector<FbxSurfacePhong*>& materials, 
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

void CreateSkeleton(FbxScene* scene, OBHierarchy* hie, OBMeshList* meshList, 
					std::vector<FbxSurfacePhong*>& materials,
					std::vector<FbxNode*>& joints,
					std::vector<FbxNode*>& meshes)
{
	FbxNode* rootNode = scene->GetRootNode();
	for (int i = 0; i < hie->nodeList.size(); i++)
	{
		std::string nodeName = std::string("Node") + std::to_string(hie->nodeList[i].localId);
		joints[i] = FbxNode::Create(scene, nodeName.c_str());
	}

	for (int i = 0; i < hie->nodeList.size(); i++)
	{
		if (hie->nodeList[i].parent)
			joints[hie->nodeList[i].parent->localId]->AddChild(joints[i]);
	}

	for (int i = 0; i < hie->roots.size(); i++)
	{
		rootNode->AddChild(joints[hie->roots[i]->localId]);
	}

	for (int i = 0; i < hie->nodeList.size(); i++)
	{
		OBNode* node = &hie->nodeList[i];
		FbxNode* fNode = joints[i];

		switch (node->getType())
		{
			default: break;
			case OBType::NodeJoint:
			{
				FbxSkeleton* skeletonAttr = FbxSkeleton::Create(scene, "");
				if (!node->parent)
					skeletonAttr->SetSkeletonType(FbxSkeleton::eRoot);
				else
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
				else
				{
					FbxSkeleton* skeletonAttr = FbxSkeleton::Create(scene, "");
					if (!node->parent)
						skeletonAttr->SetSkeletonType(FbxSkeleton::eRoot);
					else
						skeletonAttr->SetSkeletonType(FbxSkeleton::eLimbNode);
					fNode->SetNodeAttribute(skeletonAttr);
				}
			} break;
		}

		fNode->LclTranslation.Set(FbxVector4(node->transform.translation[0], 
														node->transform.translation[1], 
														node->transform.translation[2]));

		FbxProperty groupProperty = FbxProperty::Create(fNode, FbxIntDT, "GroupId", "Node Group Id");
		groupProperty.ModifyFlag(FbxPropertyFlags::eUserDefined, true);
		groupProperty.Set(node->groupId);

		joints[i] = fNode;
	}
}

static void RecursivelyAddPoseBinds(FbxScene* scene, FbxPose* pose, FbxNode* node, OBNbd* nbd, std::vector<FbxNode*>& joints)
{
	FbxMatrix matrix = node->EvaluateGlobalTransform();
	pose->Add(node, matrix);
	FbxNode* parent = node->GetParent();
	if (parent && parent != scene->GetRootNode())
		RecursivelyAddPoseBinds(scene, pose, parent, nbd, joints);
}

static void CreateBindPoses(FbxScene* scene, OBNbd* nbd, std::vector<FbxNode*>& meshes, std::vector<FbxNode*>& joints)
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
				RecursivelyAddPoseBinds(scene, pose, node, nbd, joints);
			}
			scene->AddPose(pose);
		}
	}

	for (int i = 0; i < joints.size(); i++)
	{
		OBNode* node = &nbd->ahi.nodeList[i];
		joints[i]->LclScaling.Set(FbxVector4(node->transform.scale[0], 
											 node->transform.scale[1], 
											 node->transform.scale[2]));
		joints[i]->LclRotation.Set(FbxVector4((node->transform.rotation[0] / M_PI) * 180.0f,
											  (node->transform.rotation[1] / M_PI) * 180.0f,
											  (node->transform.rotation[2] / M_PI) * 180.0f));
	}
}

static void FillScene(std::string output, FbxScene* scene, OBNbd* nbd)
{
	std::vector<FbxNode*> joints(nbd->ahi.nodeList.size());
	std::vector<FbxFileTexture*> textures(nbd->amo.textureList->list.size());
	std::vector<FbxSurfacePhong*> materials(nbd->amo.materialList->list.size());
	std::vector<FbxNode*> meshes(nbd->amo.meshList->list.size());
	CreateTextures(output, scene, nbd->amo.textureList, textures);
	CreateMaterials(output, scene, nbd->amo.materialList, materials, textures);
	CreateSkeleton(scene, &nbd->ahi, nbd->amo.meshList, materials, joints, meshes);
	CreateMeshes(scene, nbd->amo.meshList, materials, joints, meshes);
	CreateBindPoses(scene, nbd, meshes, joints);
}

static int OutputTextures(std::string output, std::vector<OBNbdTexture>& textures)
{
	int r = 1;
	for (int i = 0; i < textures.size(); i++)
	{
		std::string fileName = output + ".texture" + std::to_string(i) + ".png";
		printf("%s\n", fileName.c_str());
		r &= OutbreakTm2ToPng(textures[i].tim2Data, fileName.c_str());
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

	FbxManager* lSdkManager = FbxManager::Create();
	FbxIOSettings * ios = FbxIOSettings::Create(lSdkManager, IOSROOT);
	lSdkManager->SetIOSettings(ios);
	FbxExporter* lExporter = FbxExporter::Create(lSdkManager, "");
	bool lExportStatus = lExporter->Initialize((output + ".fbx").c_str(), -1, lSdkManager->GetIOSettings());

	FbxScene* lScene = FbxScene::Create(lSdkManager, "nbdScene");
	FillScene(output, lScene, &nbd);
	OutputTextures(output, nbd.textures);

	lExporter->Export(lScene);
	if (!lExportStatus)
	{
		fprintf(stderr, "Call to FbxExporter::Initialize() failed.\n");
		fprintf(stderr, "Error returned: %s\n\n", lExporter->GetStatus().GetErrorString());
		return 0;
	}

	lExporter->Destroy();
	lSdkManager->Destroy();

	return 1;
}