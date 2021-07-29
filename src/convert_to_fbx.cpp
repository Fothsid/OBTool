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
		texture->SetSwapUV(false);
		texture->SetTranslation(0.0, 0.0);
		texture->SetScale(1.0, 1.0);
		texture->SetRotation(0.0, 0.0);

		textures.push_back(texture);
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

		materials.push_back(material);
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

void CreateMeshes(FbxScene* scene, OBMeshList* meshList, 
				  std::vector<FbxSurfacePhong*>& materials, 
				  std::vector<FbxNode*>& joints,
				  std::vector<FbxNode*>& meshes)
{
	FbxNode* mainNode = FbxNode::Create(scene, "Object");
	for (int i = 0; i < meshList->list.size(); i++)
	{
		OBMesh* m = &meshList->list[i];
		std::string meshName = std::string("Mesh") + std::to_string(i);

		FbxNode* meshNode = FbxNode::Create(scene, meshName.c_str());
		FbxMesh* mesh = FbxMesh::Create(scene, meshName.c_str());

		mesh->InitControlPoints(m->vertexList->list.size());
		FbxVector4* controlPoints = mesh->GetControlPoints();
		for (int j = 0; j < m->vertexList->list.size(); j++)
		{
			OBVertex v = m->vertexList->list[j];
			controlPoints[j].Set(v.x, v.y, v.z);
		}

		if (m->normalList)
		{
			FbxGeometryElementNormal* normalElement = mesh->CreateElementNormal();
			normalElement->SetMappingMode(FbxGeometryElement::eByControlPoint);
			normalElement->SetReferenceMode(FbxGeometryElement::eDirect);
			for (int j = 0; j < m->normalList->list.size(); j++)
			{
				OBNormal v = m->normalList->list[j];
				FbxVector4 n(v.x, v.y, v.z);
				normalElement->GetDirectArray().Add(n);
			}
		}

		if (m->texCoordList)
		{
			FbxGeometryElementUV* uvElement = mesh->CreateElementUV("MainUVMap");
			uvElement->SetMappingMode(FbxGeometryElement::eByControlPoint);
			uvElement->SetReferenceMode(FbxGeometryElement::eDirect);
			for (int j = 0; j < m->texCoordList->list.size(); j++)
			{
				OBTexCoord v = m->texCoordList->list[j];
				FbxVector2 t(v.u, 1.0f - v.v);
				uvElement->GetDirectArray().Add(t);
			}
		}

		if (m->colorList)
		{
			FbxGeometryElementVertexColor* vertexColorElement = mesh->CreateElementVertexColor();
			vertexColorElement->SetMappingMode(FbxGeometryElement::eByControlPoint);
			vertexColorElement->SetReferenceMode(FbxGeometryElement::eDirect);
			for (int j = 0; j < m->colorList->list.size(); j++)
			{
				OBColor v = m->colorList->list[j];
				FbxColor c(v.r / 255.0f, v.g / 255.0f, v.b / 255.0f, v.a / 255.0f);
				vertexColorElement->GetDirectArray().Add(c);
			}
		}

		if (m->jointRefList)
		{
			FbxSkin* skin = FbxSkin::Create(scene, "");
			mesh->AddDeformer(skin);

			for (int j = 0; j < m->jointRefList->list.size(); j++)
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
		meshes.push_back(meshNode);

		mainNode->AddChild(meshNode);
	}
	
	FbxNode* rootNode = scene->GetRootNode();
	rootNode->AddChild(mainNode);
}

void CreateSkeleton(FbxScene* scene, OBHierarchy* hie, std::vector<FbxNode*>& joints)
{
	FbxNode* mainNode = FbxNode::Create(scene, "Skeleton");
	for (int i = 0; i < hie->nodeList.size(); i++)
	{
		OBNode* node = &hie->nodeList[i];
		std::string boneName = std::string("Joint") + std::to_string(node->localId);

		FbxSkeleton* skeletonAttr = FbxSkeleton::Create(scene, "SkeletonAttr");
		if (!node->parent)
			skeletonAttr->SetSkeletonType(FbxSkeleton::eRoot);
		else
			skeletonAttr->SetSkeletonType(FbxSkeleton::eLimbNode);

		FbxNode* skeletonNode = FbxNode::Create(scene, boneName.c_str());
		skeletonNode->SetNodeAttribute(skeletonAttr);
		skeletonNode->LclTranslation.Set(FbxVector4(node->transform.translation[0], 
													node->transform.translation[1], 
													node->transform.translation[2]));

		FbxProperty groupProperty = FbxProperty::Create(skeletonNode, FbxIntDT, "JointGroupId", "Joint Group ID");
		groupProperty.ModifyFlag(FbxPropertyFlags::eUserDefined, true);
		groupProperty.Set(node->groupId);

		joints.push_back(skeletonNode);
	}

	for (int i = 0; i < hie->nodeList.size(); i++)
	{
		if (hie->nodeList[i].parent)
			joints[hie->nodeList[i].parent->localId]->AddChild(joints[i]);
	}

	for (int i = 0; i < hie->roots.size(); i++)
	{
		mainNode->AddChild(joints[i]);
	}

	FbxNode* rootNode = scene->GetRootNode();
	rootNode->AddChild(mainNode);
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
		FbxPose* pose = FbxPose::Create(scene, poseName.c_str());
		pose->SetIsBindPose(true);
		pose->Add(meshes[i], meshes[i]->EvaluateGlobalTransform());
		FbxMesh* mesh = (FbxMesh*) meshes[i]->GetNodeAttribute();
		FbxSkin* skin = (FbxSkin*) mesh->GetDeformer(0);
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

	for (int i = 0; i < joints.size(); i++)
	{
		OBNode* node = &nbd->ahi.nodeList[i];
		joints[i]->LclScaling.Set(FbxVector4(node->transform.scale[0], 
											 node->transform.scale[1], 
											 node->transform.scale[2]));
		joints[i]->LclRotation.Set(FbxVector4((-node->transform.rotation[0] / M_PI) * 180.0f,
											  (-node->transform.rotation[1] / M_PI) * 180.0f,
											  (-node->transform.rotation[2] / M_PI) * 180.0f));
	}
}

static void FillScene(std::string output, FbxScene* scene, OBNbd* nbd)
{
	std::vector<FbxNode*> joints;
	std::vector<FbxFileTexture*> textures;
	std::vector<FbxSurfacePhong*> materials;
	std::vector<FbxNode*> meshes;
	CreateTextures(output, scene, nbd->amo.textureList, textures);
	CreateMaterials(output, scene, nbd->amo.materialList, materials, textures);
	CreateSkeleton(scene, &nbd->ahi, joints);
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