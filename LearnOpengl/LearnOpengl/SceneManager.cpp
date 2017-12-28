#include "SceneManager.h"

SceneManager* SceneManager::gpSceneManager = nullptr;

SceneManager* SceneManager::Instance()
{
	if (gpSceneManager != nullptr)
		return gpSceneManager;
	else
	{
		gpSceneManager = new SceneManager;
		assert(gpSceneManager != nullptr);
		return gpSceneManager;
	}
}

void SceneManager::Destroy()
{
	DestroyTree(&mRoot);
	for (int i = 0; i < mAllModels.size(); i++)
		delete(mAllModels[i]);
	mAllModels.clear();
	delete gpSceneManager;
}

void SceneManager::AddModel(Model* iModel)
{
	mAllModels.push_back(iModel);
	std::vector<Mesh> meshes = iModel->GetAllMeshes();
	for (int i = 0; i < meshes.size(); i++)
	{
		std::vector<Vertex> vertices = meshes[i].GetVertices();
		std::vector<unsigned int> indices = meshes[i].GetIndices();
		for (int j = 0; j < indices.size(); j += 3)
		{
			OctreeNode* node = new OctreeNode;
			node->faceIndex = j / 3;
			node->cubeSize = 0;
			node->cubeCenter = glm::vec3(0, 0, 0);
			node->faceCenter = (vertices[indices[j]].Position + vertices[indices[j + 1]].Position + vertices[indices[j + 2]].Position) / 3.0f;
			node->meshIndex = i;
			node->model = iModel;
			node->childern = nullptr;
			//for (int k = 0; k < 8; k++)
			//	node->childern[k] = { -1, 0, glm::vec3(0,0,0), nullptr, nullptr, nullptr };

			InsertNode(&mRoot, node);
		}
	}
}

bool SceneManager::Raycast(glm::vec3 iOriginal, glm::vec3 iDirection, float iLength, HitInfo & oHitInfo)
{
	/* No Octree
	//raycast with each model
	for (int i = 0; i < mAllModels.size(); i++)
	{
		std::vector<Mesh> meshes = mAllModels[i]->GetAllMeshes();
		//raycast with each mesh
		for (int j = 0; j < meshes.size(); j++)
		{
			std::vector<Vertex> vertices = meshes[j].GetVertices();
			std::vector<unsigned int> indices = meshes[j].GetIndices();

			//raycast with each face
			for (int k = 0; k < indices.size(); k += 3)
			{
				glm::vec3 point1, point2, point3, normal;
				point1 = vertices[indices[k]].Position;
				point2 = vertices[indices[k + 1]].Position;
				point3 = vertices[indices[k + 2]].Position;
				normal = vertices[indices[k]].Normal;

				glm::vec3 faceNormal = glm::normalize(glm::cross(point2 - point1, point3 - point1));
				if (glm::dot(faceNormal, normal) < 0)
					faceNormal = -faceNormal;
				float d = -glm::dot(point1, faceNormal);

				float t = -(glm::dot(iOriginal, faceNormal) + d) / (glm::dot(iDirection, faceNormal));
				glm::vec3 intersectionPoint = iOriginal + t * iDirection;

				//check if intersection point is inside triangle
				glm::vec3 vector1 = point3 - point1, vector2 = point2 - point1, vector = intersectionPoint - point1;
				float t1, t2;

				//rarely happen?
				if (abs(vector2.y *vector1.x - vector2.x * vector1.y) <= 0.00001)
					continue;

				t1 = (vector1.x * vector.y - vector.x * vector1.y) / (vector2.y * vector1.x - vector2.x * vector1.y);
				if (vector1.x == 0)
					t2 = (vector.y - t1* vector2.y) / vector1.y;
				else
					t2 = (vector.x - t1 * vector2.x) / vector1.x;
				if (t1 >= 0 && t1 <= 1 && t2 >= 0 && t2 <= 1 && t1 + t2 <= 1 && t < iLength)
				{
					oHitInfo.hitObject = mAllModels[i];
					oHitInfo.hitPosition = intersectionPoint;
					return true;
				}
			}
		}
	}
	*/

	/*Octree raycast*/
	if (RaycastNode(iOriginal, iDirection, iLength, &mRoot, oHitInfo) >= 0)
		return true;
	return false;
}

void SceneManager::Draw()
{
	for (int i = 0; i < mAllModels.size(); i++)
		mAllModels[i]->Draw();
}



float SceneManager::RaycastNode(const glm::vec3 & iOriginal, const glm::vec3 & iDirection, float iLength, OctreeNode * iNode, HitInfo& oHitInfo)
{
	//check if hit the node
	if (IntersectCube(iOriginal, iDirection,  iNode->cubeCenter, iNode->cubeSize))
	{
		//if it's a end node, intersect the face in it
		if (iNode->childern == nullptr)
		{
			const Mesh& mesh = iNode->model->GetAllMeshes()[iNode->meshIndex];
			glm::vec3 point1 = mesh.GetVertices()[mesh.GetIndices()[iNode->faceIndex * 3]].Position;
			glm::vec3 point2 = mesh.GetVertices()[mesh.GetIndices()[iNode->faceIndex * 3 + 1]].Position;
			glm::vec3 point3 = mesh.GetVertices()[mesh.GetIndices()[iNode->faceIndex * 3 + 2]].Position;
			glm::vec3 normal = mesh.GetVertices()[mesh.GetIndices()[iNode->faceIndex * 3 + 2]].Normal;

			glm::vec3 hitPosition;
			float t;
			t = IntersectFace(iOriginal, iDirection, iLength, point1, point2, point3, normal, hitPosition);
			if (t >= 0)
			{
				oHitInfo.hitPosition = hitPosition;
				oHitInfo.hitObject = iNode->model;
			}
			else
			{
				oHitInfo.hitPosition = glm::vec3(0, 0, 0);
				oHitInfo.hitObject = nullptr;
			}
			return t;
		}
		else
		{
			//raycast each child
			float minT = MAXSIZE;
			HitInfo minHitInfo = { nullptr, glm::vec3(0,0,0)};
			for (int i = 0; i < 8; i++)
			{
				if (iNode->childern[i] != nullptr)
				{
					HitInfo hitInfo;
					float t =RaycastNode(iOriginal, iDirection, iLength, iNode->childern[i], hitInfo);
					if (t >= 0 && t < minT)
					{
						minHitInfo = hitInfo;
						minT = t;
					}
				}
			}

			//doesn't intersect with any face
			if (minHitInfo.hitObject == nullptr)
				return -1;
			
			oHitInfo = minHitInfo;
			return minT;
		}
	}

	return -1;
}

bool SceneManager::IntersectCube(const glm::vec3 & iOriginal, const glm::vec3 & iDirection, const glm::vec3& cubeCenter, float cubeSize)
{
	cubeSize /= 2;
	glm::vec3 hitPosition;
	//up face
	if (IntersectFace(iOriginal, iDirection, MAXSIZE * 2, cubeCenter + glm::vec3(-1, 1, 1) * cubeSize, cubeCenter + glm::vec3(1, 1, 1) * cubeSize, cubeCenter + glm::vec3(-1, 1, -1) * cubeSize, glm::vec3(0, 1, 0), hitPosition, false) >= 0)
		return true;
	//down face
	if (IntersectFace(iOriginal, iDirection, MAXSIZE * 2, cubeCenter + glm::vec3(-1, -1, 1) * cubeSize, cubeCenter + glm::vec3(1, -1, 1) * cubeSize, cubeCenter + glm::vec3(-1, -1, -1) * cubeSize, glm::vec3(0, -1, 0), hitPosition, false) >= 0)
		return true;
	//left face
	if (IntersectFace(iOriginal, iDirection, MAXSIZE * 2, cubeCenter + glm::vec3(-1, -1, 1) * cubeSize, cubeCenter + glm::vec3(-1, -1, -1) * cubeSize, cubeCenter + glm::vec3(-1, 1, 1) * cubeSize, glm::vec3(-1, 0, 0), hitPosition, false) >= 0)
		return true;
	//right face
	if (IntersectFace(iOriginal, iDirection, MAXSIZE * 2, cubeCenter + glm::vec3(1, -1, 1) * cubeSize, cubeCenter + glm::vec3(1, -1, -1) * cubeSize, cubeCenter + glm::vec3(1, 1, 1) * cubeSize, glm::vec3(1, 0, 0), hitPosition, false) >= 0)
		return true;
	//front face
	if (IntersectFace(iOriginal, iDirection, MAXSIZE * 2, cubeCenter + glm::vec3(-1, -1, 1) * cubeSize, cubeCenter + glm::vec3(-1, 1, 1) * cubeSize, cubeCenter + glm::vec3(1, -1, 1) * cubeSize, glm::vec3(0, 0, 1), hitPosition, false) >= 0)
		return true;
	//back face
	if (IntersectFace(iOriginal, iDirection, MAXSIZE * 2, cubeCenter + glm::vec3(1, -1, -1) * cubeSize, cubeCenter + glm::vec3(1, 1, -1) * cubeSize, cubeCenter + glm::vec3(-1, -1, -1) * cubeSize, glm::vec3(0, 0, -1), hitPosition, false) >= 0)
		return true;

	return false;

}

float SceneManager::IntersectFace(const glm::vec3 & iOriginal, const glm::vec3 & iDirection, float iLength, const glm::vec3 & iPoint1, const glm::vec3 & iPoint2, const glm::vec3 & iPoint3, const glm::vec3 & iNormal, glm::vec3 & oHitPosition, bool isTriangle)
{
	glm::vec3 faceNormal = glm::normalize(glm::cross(iPoint2 - iPoint1, iPoint3 - iPoint1));
	if (glm::dot(faceNormal, iNormal) < 0)
		faceNormal = -faceNormal;
	float d = -glm::dot(iPoint1, faceNormal);

	float t = -(glm::dot(iOriginal, faceNormal) + d) / (glm::dot(iDirection, faceNormal));
	glm::vec3 intersectionPoint = iOriginal + t * iDirection;

	//check if intersection point is inside triangle
	glm::vec3 vector1 = iPoint3 - iPoint1, vector2 = iPoint2 - iPoint1, vector = intersectionPoint - iPoint1;
	float t1, t2;

	//
	if (abs(vector2.y *vector1.x - vector2.x * vector1.y) <= 0.00001)
	{
		if(abs(vector2.z * vector1.x - vector2.x * vector1.z) <= 0.00001)
			t1 = (vector1.z * vector.y - vector.z * vector1.y) / (vector2.y * vector1.z - vector2.z * vector1.y);
		else
			t1 = (vector1.z * vector.x - vector.z * vector1.x) / (vector2.x * vector1.z - vector2.z * vector1.x);
	}
	else
		t1 = (vector1.x * vector.y - vector.x * vector1.y) / (vector2.y * vector1.x - vector2.x * vector1.y);

	if (vector1.x != 0)
		t2 = (vector.x - t1* vector2.x) / vector1.x;
	else if(vector1.y != 0)
		t2 = (vector.y - t1 * vector2.y) / vector1.y;
	else
		t2 = (vector.z - t1 * vector2.z) / vector1.z;

	if (t1 >= 0 && t1 <= 1 && t2 >= 0 && t2 <= 1  && t < iLength)
	{
		if (!isTriangle || isTriangle &&  t1 + t2 <= 1)
		{
			oHitPosition = intersectionPoint;
			return t;
		}
	}

	return -1;
}

void SceneManager::InsertNode(OctreeNode * iParent, OctreeNode * iNode)
{
	//if parent is empty (root node), move the content of node to parent
	if (iParent->model == nullptr && iParent->childern == nullptr)
	{
		iParent->faceCenter = iNode->faceCenter;
		iParent->faceIndex = iNode->faceIndex;
		iParent->meshIndex = iNode->meshIndex;
		iParent->model = iNode->model;
		delete iNode;
	}
	//if parent has no child, make a new child node to keep the content of parent, and insert iNode again
	else if (iParent->childern == nullptr)
	{
		OctreeNode* childNode = new OctreeNode;
		*childNode = *iParent;
		int index = CalculateChildIndex(iParent, childNode);

		//todo
		if (index < 0)
			return;
		//assert(index >= 0);

		CreateChildNodes(iParent);
		iParent->childern[index] = childNode;

		//empty iParent
		iParent->faceIndex = -1;
		iParent->faceCenter = glm::zero<glm::vec3>();
		iParent->model = nullptr;
		iParent->meshIndex = -1;

		//
		InsertNode(iParent, iNode);
	}
	//find the index of iNode and make it be child of iParent
	else
	{
		int index = CalculateChildIndex(iParent, iNode);

		//todo
		if (index < 0)
			return;
		//assert(index >= 0);

		if (iParent->childern[index] == nullptr)
		{
			iParent->childern[index] = iNode;
		}
		else
			InsertNode(iParent->childern[index], iNode);
	}
}

void SceneManager::DestroyTree(OctreeNode* iNode)
{
	if (iNode->childern == nullptr)
		return;
	
	for (int i = 0; i < 8; i++)
	{
		if (iNode->childern[i] != nullptr)
		{
			DestroyTree(iNode->childern[i]);
			delete iNode->childern[i];
		}
	}
}

bool SceneManager::IsEmptyNode(const OctreeNode& iNode) const
{
	if (iNode.model == nullptr)
		return true;
	return false;
}

void SceneManager::CreateChildNodes(OctreeNode * iParent)
{
	iParent->childern = new OctreeNode*[8];
	for (int i = 0; i < 8; i++)
		iParent->childern[i] = nullptr;
}

int SceneManager::CalculateChildIndex(OctreeNode const* iParent, OctreeNode* iChild)
{
	glm::vec3 iChildFaceCenter = iChild->faceCenter;
	glm::vec3 iParentCubeCenter = iParent->cubeCenter;
	iChild->cubeSize = iParent->cubeSize / 2;

	//todo
	//assert(iChild->cubeSize != 0);
	if (iChild->cubeSize == 0)
		return -1;


	if (iChildFaceCenter.x <= iParentCubeCenter.x && iChildFaceCenter.y >= iParentCubeCenter.y && iChildFaceCenter.z <= iParentCubeCenter.z)
	{
		iChild->cubeCenter = iParent->cubeCenter + glm::vec3(-1, 1, -1) * (iParent->cubeSize / 4);
		return 0;
	}
	else if (iChildFaceCenter.x >= iParentCubeCenter.x && iChildFaceCenter.y >= iParentCubeCenter.y && iChildFaceCenter.z <= iParentCubeCenter.z)
	{
		iChild->cubeCenter = iParent->cubeCenter + glm::vec3(1, 1, -1) * (iParent->cubeSize / 4);
		return 1;
	}
	else if (iChildFaceCenter.x <= iParentCubeCenter.x && iChildFaceCenter.y >= iParentCubeCenter.y && iChildFaceCenter.z >= iParentCubeCenter.z)
	{
		iChild->cubeCenter = iParent->cubeCenter + glm::vec3(-1, 1, 1) * (iParent->cubeSize / 4);
		return 2;
	}
	else if (iChildFaceCenter.x >= iParentCubeCenter.x && iChildFaceCenter.y >= iParentCubeCenter.y && iChildFaceCenter.z >= iParentCubeCenter.z)
	{
		iChild->cubeCenter = iParent->cubeCenter + glm::vec3(1, 1, 1) * (iParent->cubeSize / 4);
		return 3;
	}
	else if(iChildFaceCenter.x <= iParentCubeCenter.x && iChildFaceCenter.y <= iParentCubeCenter.y && iChildFaceCenter.z <= iParentCubeCenter.z)
	{
		iChild->cubeCenter = iParent->cubeCenter + glm::vec3(-1, -1, -1) * (iParent->cubeSize / 4);
		return 4;
	}
	else if (iChildFaceCenter.x >= iParentCubeCenter.x && iChildFaceCenter.y <= iParentCubeCenter.y && iChildFaceCenter.z <= iParentCubeCenter.z)
	{
		iChild->cubeCenter = iParent->cubeCenter + glm::vec3(1, -1, -1) * (iParent->cubeSize / 4);
		return 5;
	}
	else if (iChildFaceCenter.x <= iParentCubeCenter.x && iChildFaceCenter.y <= iParentCubeCenter.y && iChildFaceCenter.z >= iParentCubeCenter.z)
	{
		iChild->cubeCenter = iParent->cubeCenter + glm::vec3(-1, -1, 1) * (iParent->cubeSize / 4);
		return 6;
	}
	else if (iChildFaceCenter.x >= iParentCubeCenter.x && iChildFaceCenter.y <= iParentCubeCenter.y && iChildFaceCenter.z >= iParentCubeCenter.z)
	{
		iChild->cubeCenter = iParent->cubeCenter + glm::vec3(1, -1, 1) * (iParent->cubeSize / 4);
		return 7;
	}

	return -1;
}