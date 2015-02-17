#include <memory>

#include "math.hpp"
#include "common.hpp"
#include "rdr.hpp"
#include "texture.hpp"
#include "model.hpp"
#include "camera.hpp"

#include <cereal/archives/json.hpp>
#include <fstream>

#define ARC(x) do { auto nvp = (x); try { arc(nvp); } catch (cereal::Exception& ex) { dbg_msg("%s: %s\n", nvp.name, ex.what()); } } while(0)
#define ARCD(x, def) \
	do { \
		auto&& nvp = (x); \
		try { arc(nvp); } \
		catch (cereal::Exception& ex) { \
			nvp.value = (def); \
			dbg_msg("%s: %s\n", nvp.name, ex.what()); \
		} \
	} while(0)

//template <class Archive, typename NVP>
//void ARCD(Archive& arc, NVP&& nvp, typename NVP::Value const& defVal) try {
//	arc(std::forward(nvp));
//} catch (cereal::Exception& ex) {
//	nvp.value = defVal;
//	dbg_msg("%s: %s\n", nvp.name, ex.what());
//}

template <class Archive>
void save(Archive& arc, DirectX::XMVECTOR const& m) {
	DirectX::XMVECTORF32 tmp;
	tmp.v = m;
	arc(tmp.f);
}

template <class Archive>
void load(Archive& arc, DirectX::XMVECTOR& m) {
	DirectX::XMVECTORF32 tmp;
	arc(tmp.f);
	m = tmp.v;
}

template <class Archive>
void sTestMtlCBuf::serialize(Archive& arc) {
	ARC(CEREAL_NVP(fresnel));
	ARC(CEREAL_NVP(shin));
	ARCD(CEREAL_NVP(nmapPower), 1.0f);
}

template <class Archive>
void sGroupMaterial::serialize(Archive& arc) {
	ARC(CEREAL_NVP(texBaseName));
	ARC(CEREAL_NVP(texNmapName));
	ARC(CEREAL_NVP(params));
}

template <size_t size>
static int build_grp_name_tag(char(&buf)[size], cstr name) {
	return ::sprintf_s(buf, "grp:%s", name);
}

template <class Archive>
void serialize(Archive& arc, cModelMaterial& m) {
	char buf[64];
	auto grpNum = m.mpMdlData->mGrpNum;
	for (uint32_t i = 0; i < grpNum; ++i) {
		sGroupMaterial& mtl = m.mpGrpMtl[i];
		build_grp_name_tag(buf, m.get_grp_name(i));
		ARC(cereal::make_nvp(buf, mtl));
	}
}

bool cModelMaterial::deserialize(cstr filepath) {
	if (!filepath) { return false; }

	std::ifstream ifs(filepath, std::ios::binary);
	if (!ifs.is_open()) { return false; }

	cereal::JSONInputArchive arc(ifs);
	arc(*this);

	return true;
}

bool cModelMaterial::serialize(cstr filepath) {
	if (!filepath) { return false; }

	std::ofstream ofs(filepath, std::ios::binary);
	if (!ofs.is_open())
		return false;
	cereal::JSONOutputArchive arc(ofs);
	arc(*this);
	return true;
}


template <class Archive>
void cCamera::serialize(Archive& arc) {
	ARC(CEREAL_NVP(mPos));
	ARC(CEREAL_NVP(mTgt));
	ARC(CEREAL_NVP(mUp));
	ARC(CEREAL_NVP(mFovY));
	ARC(CEREAL_NVP(mAspect));
	ARC(CEREAL_NVP(mNearZ));
	ARC(CEREAL_NVP(mFarZ));
}

bool cCamera::load(cstr filepath) {
	if (!filepath) { return false; }

	std::ifstream ifs(filepath, std::ios::binary);
	if (!ifs.is_open()) { return false; }

	cereal::JSONInputArchive arc(ifs);
	arc(*this);

	return true;
}

bool cCamera::save(cstr filepath) {
	if (!filepath) { return false; }

	std::ofstream ofs(filepath, std::ios::binary);
	if (!ofs.is_open())
		return false;
	cereal::JSONOutputArchive arc(ofs);
	arc(*this);
	return true;
}
