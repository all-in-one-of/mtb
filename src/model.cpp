#include "common.hpp"
#include "math.hpp"
#include "rdr.hpp"
#include "gfx.hpp"
#include "texture.hpp"
#include "model.hpp"
#include "hou_geo.hpp"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/config.h>

#include <cassert>
#include <imgui.h>


static vec4 as_vec4_1(aiVector3D const& v) {
	return { { v.x, v.y, v.z, 1.0f } };
}
static vec3 as_vec3(aiVector3D const& v) {
	return { { v.x, v.y, v.z } };
}
static vec2f as_vec2f(aiVector3D const& v) {
	return {v.x, v.y};
}
static vec3 as_vec3(aiColor4D const& v) {
	return { { v.r, v.g, v.b } };
}


static vec4 as_vec4(cHouGeoAttrib const* pa, int idx) {
	float tmp[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	if (pa) {
		float const* val = pa->get_float_val(idx);
		for (int i = 0; i < 4 && i < pa->mAttribCount; ++i) {
			tmp[i] = val[i];
		}
	}
	return { { tmp[0], tmp[1], tmp[2], tmp[3] } };
}


static vec3 as_vec3(cHouGeoAttrib const* pa, int idx) {
	float tmp[3] = { 0.0f, 0.0f, 0.0f };
	if (pa) {
		float const* val = pa->get_float_val(idx);
		for (int i = 0; i < 3 && i < pa->mAttribCount; ++i) {
			tmp[i] = val[i];
		}
	}
	return { { tmp[0], tmp[1], tmp[2] } };
}

static vec2f as_vec2f(cHouGeoAttrib const* pa, int idx) {
	float tmp[] = { 0.0f, 0.0f };
	if (pa) {
		float const* val = pa->get_float_val(idx);
		for (int i = 0; i < 2 && i < pa->mAttribCount; ++i) {
			tmp[i] = val[i];
		}
	}
	return { tmp[0], tmp[1] };
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
	if (filepath.ends_with(".geo")) {
		return load_hou_geo(filepath);
	} else {
		return load_assimp(filepath);
	}
}

bool cModelData::load_hou_geo(cstr filepath) {
	cHouGeoLoader geo;
	if (!geo.load(filepath))
		return false;

	int numVtx = geo.mPointCount;
	int numIdx = geo.mVertexCount;
	int numGrp = geo.mNonemptyGroups;

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

	cHouGeoAttrib const* pPosAttr = nullptr;
	cHouGeoAttrib const* pNrmAttr = nullptr;
	cHouGeoAttrib const* pUVAttr = nullptr;
	cHouGeoAttrib const* pTngUAttr = nullptr;
	cHouGeoAttrib const* pTngVAttr = nullptr;
	cHouGeoAttrib const* pUV1Attr = nullptr;
	cHouGeoAttrib const* pCdAttr = nullptr;

	for (int i = 0; i < geo.mPointAttribCount; ++i) {
		auto const& attr = geo.mpPointAttribs[i];
		if (attr.mName == "P" && attr.mType == cHouGeoAttrib::E_TYPE_fpreal32) {
			pPosAttr = &attr;
		}
		else if (attr.mName == "N" && attr.mType == cHouGeoAttrib::E_TYPE_fpreal32) {
			pNrmAttr = &attr;
		}
		else if (attr.mName == "uv" && attr.mType == cHouGeoAttrib::E_TYPE_fpreal32) {
			pUVAttr = &attr;
		}
		else if (attr.mName == "tangentu" && attr.mType == cHouGeoAttrib::E_TYPE_fpreal32) {
			pTngUAttr = &attr;
		}
		else if (attr.mName == "tangentv" && attr.mType == cHouGeoAttrib::E_TYPE_fpreal32) {
			pTngVAttr = &attr;
		}
		else if (attr.mName == "uv1" && attr.mType == cHouGeoAttrib::E_TYPE_fpreal32) {
			pUV1Attr = &attr;
		}
		else if (attr.mName == "Cd" && attr.mType == cHouGeoAttrib::E_TYPE_fpreal32) {
			pCdAttr = &attr;
		}
	}

	for (int vtx = 0; vtx < numVtx; ++vtx) {
		pVtxItr->pos = as_vec3(pPosAttr, vtx);
		pVtxItr->nrm = as_vec3(pNrmAttr, vtx);
		pVtxItr->uv = as_vec2f(pUVAttr, vtx);
		pVtxItr->uv.y = -pVtxItr->uv.y;
		pVtxItr->tgt = as_vec4(pTngUAttr, vtx);
		pVtxItr->bitgt = as_vec3(pTngVAttr, vtx);
		pVtxItr->uv1 = as_vec2f(pUV1Attr, vtx);
		pVtxItr->uv1.y = -pVtxItr->uv1.y;
		pVtxItr->clr = as_vec3(pCdAttr, vtx);
		++pVtxItr;
	}

	auto const& poly = geo.mPoly;
	auto const vmap = geo.mpVertexMap.get();

	for (int igrp = 0; igrp < geo.mGroupsCount; ++igrp) {
		auto const& grp = geo.mpGroups[igrp];
		if (grp.mEmpty) { continue; }

		auto pIdxGrpStart = pIdxItr;

		for (int i = 0; i < grp.mIntervalsCount; ++i) {
			auto const& iv = grp.mpIntervals[i];
			for (int j = iv.start; j < iv.end; ++j) {
				if (!poly[j].valid) continue;
				for (int k = 0; k < 3; ++k) {
					int pidx = poly[j].v[k];
					*pIdxItr = (uint16_t)vmap[pidx];
					pIdxItr++;
				}
			}
		}

		pGrpItr->mPolyType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		pGrpItr->mVtxOffset = 0;
		pGrpItr->mIdxCount = static_cast<uint32_t>((pIdxItr - pIdxGrpStart));
		pGrpItr->mIdxOffset = static_cast<uint32_t>((pIdxGrpStart - pIdx.get()) * sizeof(pIdxGrpStart[0]));
		
		*pNamesItr = grp.mName;

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

template <typename T>
struct sReadItr {
	T const* mpItr;
	int mStep;

	sReadItr(T const* p, T const* pDef) {
		if (p) {
			mpItr = p;
			mStep = 1;
		} else {
			mpItr = pDef;
			mStep = 0;
		}
	}

	T const& read() {
		T const& res = *mpItr;
		mpItr += mStep;
		return res;
	}
};

bool cModelData::load_assimp(cstr filepath) {
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

	const aiColor4D defColor = { 1.0f, 1.0f, 1.0f, 1.0f };
	const aiVector3D defV3Zero = { 0.0f, 0.0f, 0.0f };
	const aiVector3D defTgt = { 1.0f, 0.0f, 0.0f };
	const aiVector3D defBitgt = { 0.0f, 1.0f, 0.0f };
	const aiVector3D defNrm = { 0.0f, 0.0f, 1.0f };

	for (auto& mi : meshes) {
		aiMesh* pMesh = mi.mpMesh;

		auto pVtxGrpStart = pVtxItr;
		auto pIdxGrpStart = pIdxItr;

		const int meshVtx = pMesh->mNumVertices;

		sReadItr<aiVector3D> posItr(pMesh->mVertices, &defV3Zero);
		sReadItr<aiVector3D> nrmItr(pMesh->mNormals, &defNrm);
		sReadItr<aiVector3D> uvItr(pMesh->mTextureCoords[0], &defV3Zero);
		sReadItr<aiVector3D> tgtItr(nullptr, &defTgt);
		sReadItr<aiVector3D> bitgtItr(nullptr, &defBitgt);
		sReadItr<aiVector3D> uv1Itr(nullptr, &defV3Zero);
		sReadItr<aiColor4D> clrItr(pMesh->mColors[0], &defColor);

		for (int vtx = 0; vtx < meshVtx; ++vtx) {
			pVtxItr->pos = as_vec3(posItr.read());
			pVtxItr->nrm = as_vec3(nrmItr.read());
			pVtxItr->uv = as_vec2f(uvItr.read());
			pVtxItr->uv.y = -pVtxItr->uv.y;
			pVtxItr->tgt = as_vec4_1(tgtItr.read());
			pVtxItr->bitgt = as_vec3(bitgtItr.read());
			pVtxItr->uv1 = as_vec2f(uv1Itr.read());
			pVtxItr->clr = as_vec3(clrItr.read());
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


bool cModel::init(cModelData const& mdlData, cModelMaterial& mtl) {
	mpData = &mdlData;
	mpMtl = &mtl;

	auto& ss = cShaderStorage::get();
	auto pVS = ss.load_VS("model_solid.vs.cso");

	D3D11_INPUT_ELEMENT_DESC vdsc[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(sModelVtx, pos), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(sModelVtx, nrm), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(sModelVtx, uv), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(sModelVtx, tgt), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(sModelVtx, bitgt), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(sModelVtx, uv1), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(sModelVtx, clr), D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	auto pDev = get_gfx().get_dev();
	auto& code = pVS->get_code();
	HRESULT hr = pDev->CreateInputLayout(vdsc, SIZEOF_ARRAY(vdsc), code.get_code(), code.get_size(), mpIL.pp());
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

	cBlendStates::get().set_opaque(pCtx);

	auto grpNum = mpData->mGrpNum;
	for (uint32_t i = 0; i < grpNum; ++i) {
		sGroup const& grp = mpData->mpGroups[i];

		mpMtl->apply(pCtx, i);

		mpData->mVtx.set(pCtx, 0, grp.mVtxOffset);
		mpData->mIdx.set(pCtx, grp.mIdxOffset);
		pCtx->IASetPrimitiveTopology((D3D11_PRIMITIVE_TOPOLOGY)grp.mPolyType);
		//pCtx->Draw(grp.mIdxCount, 0);
		pCtx->DrawIndexed(grp.mIdxCount, 0, 0);

	}

}

bool ImguiSlideFloat3_1(char const* label,float v[3], float v_min, float v_max, const char* display_format = "%.3f", float power = 1.0f) {
	char keyBuf[64];
	::sprintf_s(keyBuf, "issng%s", label);
	ImGuiID id = ImGui::GetID(keyBuf);
	bool res;
	auto* pStorage = ImGui::GetStateStorage();
	auto& style = ImGui::GetStyle();
	bool isSingle = !!pStorage->GetInt(id);
	if (isSingle) {
		::sprintf_s(keyBuf, "##%s", label);
		res = ImGui::SliderFloat(keyBuf, v, v_min, v_max, display_format, power);
		if (res) {
			for (int i = 1; i < 3; ++i) {
				v[i] = v[0];
			}
		}
		ImGui::SameLine(0, (int)style.ItemInnerSpacing.x);
		ImGui::TextUnformatted(label);
	} else {
		res = ImGui::SliderFloat3(label, v, v_min, v_max, display_format, power);
	}

	if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0))	{
		int v = !isSingle;
		pStorage->SetInt(id, v);
	}

	return res;
}

void cModel::dbg_ui() {
	if (!mpData) return;
	auto grpNum = mpData->mGrpNum;
	char buf[64];
	ImGui::Begin("model");
	for (uint32_t i = 0; i < grpNum; ++i) {
		sGroup const& grp = mpData->mpGroups[i];
		auto const& name = mpData->mpGrpNames[i];
		sGroupMaterial& mtl = mpMtl->mpGrpMtl[i];

		::sprintf_s(buf, "grp #%d:%s", i, name.c_str());
		if (ImGui::CollapsingHeader(buf)) {
			ImGui::PushID(&mtl);

			ImguiSlideFloat3_1("F0", mtl.params.fresnel, 0.0f, 1.0f);
			ImGui::SliderFloat("shin", &mtl.params.shin, 0.0f, 1000.0f);
			ImGui::SliderFloat("nmap0Power", &mtl.params.nmap0Power, 0.0f, 2.0f);
			ImGui::SliderFloat("nmap1Power", &mtl.params.nmap1Power, 0.0f, 2.0f);

			ImGui::PopID();
		}
	}
	if (ImGui::Button("Save")) {
		mpMtl->save();
	}
	
	ImGui::End();
}

static void apply_tex(ID3D11DeviceContext* pCtx, cTexture* pTex, ID3D11SamplerState* pSmp, uint32_t slot) {
	if (pTex && pSmp) {
		pTex->set_PS(pCtx, slot);
		pCtx->PSSetSamplers(slot, 1, &pSmp);
	}
	else {
		cTexture::set_null_PS(pCtx, slot);
	}
}

void sGroupMtlRes::apply(ID3D11DeviceContext* pCtx) {
	apply_tex(pCtx, mpTexBase, mpSmpBase, 0);
	apply_tex(pCtx, mpTexNmap0, mpSmpNmap0, 1);
	apply_tex(pCtx, mpTexNmap1, mpSmpNmap1, 2);
	apply_tex(pCtx, mpTexMask, mpSmpMask, 3);

	pCtx->VSSetShader(mpVS->asVS(), nullptr, 0);
	pCtx->PSSetShader(mpPS->asPS(), nullptr, 0);

	cRasterizerStates::set(pCtx, mpRSState);
}

void sGroupMaterial::apply(ID3D11DeviceContext* pCtx) const {
	auto& cbs = cConstBufStorage::get();
	cbs.mTestMtlCBuf.mData = params;
	cbs.mTestMtlCBuf.update(pCtx);
	cbs.mTestMtlCBuf.set_PS(pCtx);
}

void cModelMaterial::apply(ID3D11DeviceContext* pCtx, int i) {
	mpGrpMtl[i].apply(pCtx);
	mpGrpRes[i].apply(pCtx);
}

void sGroupMaterial::set_default() {
	params.fresnel[0] = 0.025f;
	params.fresnel[1] = 0.025f;
	params.fresnel[2] = 0.025f;
	params.shin = 9.25f;
	params.nmap0Power = 1.0f;
	params.nmap1Power = 0.0f;

	twosided = false;
	vsProg = "model_solid.vs.cso";
	psProg = "model.ps.cso";
}


bool cModelMaterial::load(ID3D11Device* pDev, cModelData const& mdlData, cstr filepath) {
	mpMdlData = &mdlData;
	auto grpNum = mpMdlData->mGrpNum;
	mpGrpMtl = std::make_unique<sGroupMaterial[]>(grpNum);
	mpGrpRes = std::make_unique<sGroupMtlRes[]>(grpNum);

	if (filepath) { mFilepath = filepath; }
	
	if (!deserialize(filepath)) {
		for (uint32_t i = 0; i < grpNum; ++i) {
			mpGrpMtl[i].set_default();
		}
		//return true;
	}

	auto& ts = cTextureStorage::get();
	auto& ss = cShaderStorage::get();

	auto loadTex = [&ts, pDev](std::string const& name, cTexture*& pTex, ID3D11SamplerState*& pSmp, cTexture* pDefT) {
		cTexture* p = nullptr;
		if (!name.empty()) {
			p = ts.load(pDev, name.c_str());
		}
		if (!p) { p = pDefT; }
		pTex = p;
		pSmp = cSamplerStates::get().linear();
	};
	auto loadNmap = [&ts, pDev, &loadTex](std::string const& name, cTexture*& pTex, ID3D11SamplerState*& pSmp) {
		loadTex(name, pTex, pSmp, ts.get_def_nmap());
	};
	auto loadClr = [&ts, pDev, &loadTex](std::string const& name, cTexture*& pTex, ID3D11SamplerState*& pSmp) {
		loadTex(name, pTex, pSmp, ts.get_def_white());
	};

	for (uint32_t i = 0; i < grpNum; ++i) {
		sGroupMaterial& mtl = mpGrpMtl[i];
		sGroupMtlRes& res = mpGrpRes[i];

		loadClr(mtl.texBaseName, res.mpTexBase, res.mpSmpBase);
		loadNmap(mtl.texNmap0Name, res.mpTexNmap0, res.mpSmpNmap0);
		loadNmap(mtl.texNmap1Name, res.mpTexNmap1, res.mpSmpNmap1);
		loadClr(mtl.texMaskName, res.mpTexMask, res.mpSmpMask);

		res.mpVS = ss.load_VS(mtl.vsProg.c_str());
		if (!res.mpVS) { return false; }
		res.mpPS = ss.load_PS(mtl.psProg.c_str());
		if (!res.mpPS) { return false; }

		if (mtl.twosided) {
			res.mpRSState = cRasterizerStates::get().twosided();
		} else {
			res.mpRSState = cRasterizerStates::get().def();
		}
	}

	return true;
}

bool cModelMaterial::save(cstr filepath) {
	if (!mpMdlData || !mpGrpMtl) return false;
	if (!filepath) { filepath = mFilepath.c_str(); }
	if (!filepath) { return false; }
	return serialize(filepath);
}
