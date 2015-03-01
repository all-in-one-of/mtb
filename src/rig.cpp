#include <string>
#include <memory>

#include "math.hpp"
#include "common.hpp"
#include "rig.hpp"
#include "rdr.hpp"

#include "json_helpers.hpp"

using nJsonHelpers::Document;
using nJsonHelpers::Value;
using nJsonHelpers::Size;

class cJsonLoaderImpl {
	cRigData& mRigData;
public:
	cJsonLoaderImpl(cRigData& rig) : mRigData(rig) {}

	bool operator()(Value const& doc) {
		CHECK_SCHEMA(doc.IsObject(), "doc is not an object\n");
		CHECK_SCHEMA(doc.HasMember("joints"), "no joints in doc\n");
		CHECK_SCHEMA(doc.HasMember("mtx"), "no mtx in doc\n");
		CHECK_SCHEMA(doc.HasMember("imtx"), "no imtx in doc\n");
		auto& joints = doc["joints"];
		auto& mtx = doc["mtx"];
		auto& imtx = doc["imtx"];

		auto jointsNum = joints.Size();
		auto imtxNum = imtx.Size();
		CHECK_SCHEMA(mtx.Size() == jointsNum, "joints and mtx num differ\n");

		auto pJoints = std::make_unique<sJointData[]>(jointsNum);
		auto pMtx = std::make_unique<DirectX::XMMATRIX[]>(jointsNum);
		auto pImtx = std::make_unique<DirectX::XMMATRIX[]>(imtxNum);
		auto pNames = std::make_unique<std::string[]>(jointsNum);

		for (auto pj = joints.Begin(); pj != joints.End(); ++pj) {
			auto const& j = *pj;
			CHECK_SCHEMA(j.IsObject(), "joint definition is not an object\n");

			CHECK_SCHEMA(j.HasMember("name"), "no joint's name\n");
			auto& jname = j["name"];
			CHECK_SCHEMA(j.HasMember("idx"), "no joint's idx\n");
			int idx = j["idx"].GetInt();
			CHECK_SCHEMA(j.HasMember("parIdx"), "no joint's parIdx\n");
			int parIdx = j["parIdx"].GetInt();
			CHECK_SCHEMA(j.HasMember("skinIdx"), "no joint's skinIdx\n");
			int skinIdx = j["skinIdx"].GetInt();
			
			auto name = jname.GetString();
			auto nameLen = jname.GetStringLength();

			pNames[idx] = std::string(name, nameLen);
			pJoints[idx] = { idx, parIdx, skinIdx };
		}

		auto readMtx = [](DirectX::XMMATRIX* pMtx, Value const& m) {
			CHECK_SCHEMA(m.IsArray(), "matrix is not an array\n");
			CHECK_SCHEMA(m.Size() == 16, "wrong matrix size\n");

			float tmp[16];
			for (int i = 0; i < 16; ++i) {
				tmp[i] = (float)m[i].GetDouble();
			}
			*pMtx = DirectX::XMMATRIX(tmp);
			return true;
		};

		for (Size i = 0; i < jointsNum; ++i) {
			auto const& m = mtx[i];
			if (!readMtx(&pMtx[i], m)) {
				return false;
			}
		}

		for (Size i = 0; i < imtxNum; ++i) {
			auto const& m = imtx[i];
			if (!readMtx(&pImtx[i], m)) {
				return false;
			}
		}

		mRigData.mJointsNum = jointsNum;
		mRigData.mIMtxNum = imtxNum;
		mRigData.mpJoints = pJoints.release();
		mRigData.mpLMtx = pMtx.release();
		mRigData.mpIMtx = pImtx.release();
		mRigData.mpNames = pNames.release();
		mRigData.mAllocatedArrays = true;

		return true;
	}
};

cRigData::~cRigData() {
	if (mAllocatedArrays) {
		delete[] mpJoints;
		delete[] mpLMtx;
		delete[] mpIMtx;
		delete[] mpNames;
	}
}

bool cRigData::load(cstr filepath) {
	if (filepath.ends_with(".rig")) {
		return load_json(filepath);
	}

	dbg_msg("cRigData::load(): Unknown file extension in <%s>", filepath.p);
	return false;
}

bool cRigData::load_json(cstr filepath) {
	cJsonLoaderImpl loader(*this);
	return nJsonHelpers::load_file(filepath, loader);
}

int cRigData::find_joint_idx(cstr name) const {
	for (int i = 0; i < mJointsNum; ++i) {
		if (0 == mpNames[i].compare(name)) {
			return i;
		}
	}
	return -1;
}


void cRig::init(cRigData const* pRigData) {
	if (!pRigData) { return; }
	
	const int jointsNum = pRigData->mJointsNum;
	auto pLMtx = std::make_unique<DirectX::XMMATRIX[]>(jointsNum);
	auto pWMtx = std::make_unique<DirectX::XMMATRIX[]>(jointsNum);
	auto pJoints = std::make_unique<cJoint[]>(jointsNum);
	auto pXforms = std::make_unique<sXform[]>(jointsNum);

	::memcpy(pLMtx.get(), pRigData->mpLMtx, sizeof(pRigData->mpLMtx[0]) * jointsNum);

	for (int i = 0; i < jointsNum; ++i) {
		auto const& jdata = pRigData->mpJoints[i];
		auto& jnt = pJoints[i];

		assert(jdata.idx == i);

		jnt.mpXform = &pXforms[i];
		jnt.mpLMtx = &pLMtx[i];
		jnt.mpWMtx = &pWMtx[i];
		
		jnt.mpXform->init(*jnt.mpLMtx);

		if (jdata.skinIdx >= 0) {
			jnt.mpIMtx = &pRigData->mpIMtx[jdata.skinIdx];
		}
		if (jdata.parIdx >= 0) {
			//jnt.mpParent = &pJoints[jdata.parIdx];
			jnt.mpParentMtx = pJoints[jdata.parIdx].mpWMtx;
		}
		else {
			jnt.mpParentMtx = &nMtx::g_Identity;
		}
	}
	
	mJointsNum = jointsNum;
	mpRigData = pRigData;
	mpJoints = pJoints.release();
	mpLMtx = pLMtx.release();
	mpWmtx = pWMtx.release();
	mpXforms = pXforms.release();

	calc_world();
}

void cRig::calc_local() {
	for (int i = 0; i < mJointsNum; ++i) {
		mpJoints[i].calc_local();
	}
}

void cRig::calc_world() {
	for (int i = 0; i < mJointsNum; ++i) {
		mpJoints[i].calc_world();
	}
}

void cRig::upload_skin(ID3D11DeviceContext* pCtx) {
	auto& skinCBuf = cConstBufStorage::get().mSkinCBuf;

	auto* pSkin = skinCBuf.mData.skin;

	for (int i = 0; i < mJointsNum; ++i) {
		auto pImtx = mpJoints[i].get_inv_mtx();
		if (!pImtx) { continue; }
		int skinIdx = mpRigData->mpJoints[i].skinIdx;

		auto const& pWmtx = mpJoints[i].get_world_mtx();

		pSkin[skinIdx] = (*pImtx) * pWmtx;
	}

	skinCBuf.update(pCtx);
}

cJoint* cRig::get_joint(int idx) const {
	if (!mpJoints) { return nullptr; }
	if (idx >= mJointsNum) { return nullptr; }
	return &mpJoints[idx];
}

cJoint* cRig::find_joint(cstr name) const {
	if (!mpJoints || !mpRigData) { return nullptr; }

	int idx = mpRigData->find_joint_idx(name);
	if (idx == -1)
		return nullptr;

	return &mpJoints[idx];
}



void cJoint::calc_world() {
	//(*mpWMtx) = (*mpParentMtx) * (*mpLMtx);
	(*mpWMtx) = (*mpLMtx) * (*mpParentMtx);
}

void cJoint::calc_local() {
	(*mpLMtx) = mpXform->build_mtx();
}

