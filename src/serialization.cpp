#include "math.hpp"
#include "common.hpp"
#include "rdr.hpp"
#include "texture.hpp"
#include "model.hpp"

#include <cereal/archives/json.hpp>
#include <fstream>

#define ARC(x) do { try { arc((x)); } catch (cereal::Exception& ex) { dbg_msg1(ex.what()); } } while(0)

template <class Archive>
void sTestMtlCBuf::serialize(Archive& arc) {
	ARC(CEREAL_NVP(fresnel));
	ARC(CEREAL_NVP(shin));
}

template <class Archive>
void sGroupMaterial::serialize(Archive& arc) {
	ARC(CEREAL_NVP(texBaseName));
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
