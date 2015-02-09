#include "common.hpp"
#include "math.hpp"
#include "rdr.hpp"
#include "gfx.hpp"
#include "model.hpp"
#include "texture.hpp"


#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/config.h>

#include <cassert>


static vec3 as_vec3(aiVector3D const& v) {
	return { { v.x, v.y, v.z } };
}
static vec2f as_vec2f(aiVector3D const& v) {
	return {v.x, v.y};
}

struct sAIMeshInfo {
	aiMesh* mpMesh;
	cstr mName;
};
static void list_meshes(aiNode* pNode, std::vector<sAIMeshInfo>& meshes, aiScene const* pScene) {
	if (pNode->mNumMeshes > 0) {
		meshes.push_back({ pScene->mMeshes[pNode->mMeshes[0]], pNode->mName.C_Str()});
	}
	for (uint32_t i = 0; i < pNode->mNumChildren; ++i) {
		list_meshes(pNode->mChildren[i], meshes, pScene);
	}
}

bool cModelData::load(cstr filepath) {
	Assimp::Importer importer;

	importer.SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE,
		aiPrimitiveType_LINE | aiPrimitiveType_POINT | aiPrimitiveType_POLYGON);

	aiScene const* pScene = importer.ReadFile(filepath, 
		/*aiProcess_FlipUVs |*/ aiProcess_FlipWindingOrder | aiProcess_SortByPType);
	if (!pScene) {
		// todo: log importer.GetErrorString()
		return false;
	}
	
	std::vector<sAIMeshInfo> meshes;
	meshes.reserve(pScene->mNumMeshes);
	list_meshes(pScene->mRootNode, meshes, pScene);

	int numVtx = 0;
	int numIdx = 0;
	int numGrp = (int)meshes.size();

	for (auto& mi : meshes) {
		aiMesh* pMesh = mi.mpMesh;

		numVtx += pMesh->mNumVertices;

		int idx = pMesh->mNumFaces;
		numIdx += idx * 3;
	}

	auto pGroups = std::make_unique<sGroup[]>(numGrp);
	auto pVtx = std::make_unique<sModelVtx[]>(numVtx);
	auto pIdx = std::make_unique<uint16_t[]>(numIdx);
	auto pNames = std::make_unique<std::string[]>(numGrp);

	const uint32_t vtxSize = sizeof(sModelVtx);
	const DXGI_FORMAT idxFormat = DXGI_FORMAT_R16_UINT;

	auto pVtxItr = pVtx.get();
	auto pIdxItr = pIdx.get();
	auto pGrpItr = pGroups.get();
	auto pNamesItr = pNames.get();

	for (auto& mi : meshes) {
		aiMesh* pMesh = mi.mpMesh;

		auto pVtxGrpStart = pVtxItr;
		auto pIdxGrpStart = pIdxItr;

		const int meshVtx = pMesh->mNumVertices;
		aiVector3D const* pAIVtx = pMesh->mVertices;
		aiVector3D const* pAINrm = pMesh->mNormals;
		aiVector3D const* pAITex = pMesh->mTextureCoords[0];

		for (int vtx = 0; vtx < meshVtx; ++vtx) {
			pVtxItr->pos = as_vec3(*pAIVtx);
			++pAIVtx;

			pVtxItr->nrm = as_vec3(*pAINrm);
			++pAINrm;

			if (pAITex) {
				pVtxItr->uv = as_vec2f(*pAITex);
				pVtxItr->uv.y = -pVtxItr->uv.y;
				++pAITex;
			} else {
				pVtxItr->uv = { 0.0f, 0.0f };
			}

			++pVtxItr;
		}

		const int meshFace = pMesh->mNumFaces;
		aiFace const* pFace = pMesh->mFaces;
		for (int face = 0; face < meshFace; ++face) {
			assert(pFace->mNumIndices == 3);
			pIdxItr[0] = pFace->mIndices[0];
			pIdxItr[1] = pFace->mIndices[1];
			pIdxItr[2] = pFace->mIndices[2];

			pIdxItr += 3;
			++pFace;
		}

		pGrpItr->mVtxOffset = static_cast<uint32_t>((pVtxGrpStart - pVtx.get()) * sizeof(pVtxGrpStart[0]));
		pGrpItr->mIdxCount = static_cast<uint32_t>((pIdxItr - pIdxGrpStart));
		pGrpItr->mIdxOffset = static_cast<uint32_t>((pIdxGrpStart - pIdx.get()) * sizeof(pIdxGrpStart[0]));
		pGrpItr->mPolyType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

		if (mi.mName.starts_with("g ")) {
			*pNamesItr = &mi.mName.p[2];
		} else {
			*pNamesItr = mi.mName;
		}
		

		++pGrpItr;
		++pNamesItr;
	}

	auto pDev = get_gfx().get_dev();
	mVtx.init(pDev, pVtx.get(), numVtx, vtxSize);
	mIdx.init(pDev, pIdx.get(), numIdx, idxFormat);

	mGrpNum = numGrp;
	mpGroups = std::move(pGroups);
	mpGrpNames = std::move(pNames);

	return true;
}

void cModelData::unload() {
	mVtx.deinit();
	mIdx.deinit();
	mpGroups.release();
	mpGrpNames.release();
}


bool cModel::init(cModelData const& mdlData) {
	mpData = &mdlData;
		
	auto& ss = get_shader_storage();
	mpVS = ss.load_VS("model_solid.vs.cso");
	mpPS = ss.load_PS("model.ps.cso");

	D3D11_INPUT_ELEMENT_DESC vdsc[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(sModelVtx, pos), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(sModelVtx, nrm), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(sModelVtx, uv), D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	auto pDev = get_gfx().get_dev();
	auto& code = mpVS->get_code();
	HRESULT hr = pDev->CreateInputLayout(vdsc, SIZEOF_ARRAY(vdsc), code.get_code(), code.get_size(), &mpIL);
	if (!SUCCEEDED(hr)) throw sD3DException(hr, "CreateInputLayout failed");

	return true;
}

void cModel::deinit() {
	mpData = nullptr;
	if (mpIL) {
		mpIL->Release();
		mpIL = nullptr;
	}
}

void cModel::disp() {
	if (!mpData) return;

	auto pCtx = get_gfx().get_ctx();

	auto& meshCBuf = cConstBufStorage::get().mMeshCBuf;
	//meshCBuf.mData.wmtx = dx::XMMatrixIdentity();
	//meshCBuf.mData.wmtx = DirectX::XMMatrixTranslation(0, 0, 0);
	//meshCBuf.mData.wmtx = DirectX::XMMatrixRotationY(DEG2RAD(180));
	meshCBuf.mData.wmtx = DirectX::XMMatrixScaling(0.3f, 0.3f, 0.3f);
	meshCBuf.update(pCtx);
	meshCBuf.set_VS(pCtx);

	pCtx->IASetInputLayout(mpIL);

	pCtx->VSSetShader(mpVS->asVS(), nullptr, 0);
	pCtx->PSSetShader(mpPS->asPS(), nullptr, 0);

	auto grpNum = mpData->mGrpNum;
	for (uint32_t i = 0; i < grpNum; ++i) {
		sGroup const& grp = mpData->mpGroups[i];
		sGroupMaterial const& mtl = mpGrpMtl[i];

		if (mtl.mpTexBase && mtl.mpSmpBase) {
			mtl.mpTexBase->set_PS(pCtx, 0);
			pCtx->PSSetSamplers(0, 1, &mtl.mpSmpBase);
		}

		mpData->mVtx.set(pCtx, 0, grp.mVtxOffset);
		mpData->mIdx.set(pCtx, grp.mIdxOffset);
		pCtx->IASetPrimitiveTopology((D3D11_PRIMITIVE_TOPOLOGY)grp.mPolyType);
		//pCtx->Draw(grp.mIdxCount, 0);
		pCtx->DrawIndexed(grp.mIdxCount, 0, 0);

		//break;
	}

}
