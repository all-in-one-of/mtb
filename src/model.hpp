#include <memory>
#include <string>

struct sModelVtx;

struct sGroup {
	uint32_t mVtxOffset;
	uint32_t mIdxOffset;
	uint32_t mIdxCount;
	uint32_t mPolyType;
};

class cModelData : noncopyable {
public:
	uint32_t mGrpNum;
	std::unique_ptr<sGroup[]> mpGroups;
	std::unique_ptr<std::string[]> mpGrpNames;

	cVertexBuffer mVtx;
	cIndexBuffer mIdx;

public:
	cModelData() {}
	cModelData(cModelData&& o) : 
		mpGroups(std::move(o.mpGroups)),
		mpGrpNames(std::move(o.mpGrpNames)),
		mVtx(std::move(o.mVtx)),
		mIdx(std::move(o.mIdx))
	{}
	cModelData& operator=(cModelData&& o) {
		mpGroups = std::move(o.mpGroups);
		mpGrpNames = std::move(o.mpGrpNames);
		mVtx = std::move(o.mVtx);
		mIdx = std::move(o.mIdx);
		return *this;
	}

	cModelData& operator=(cModelData&) = delete;

	bool load(cstr filepath);
	void unload();
};


class cModel {
	cModelData const* mpData = nullptr;

	cShader* mpVS = nullptr;
	cShader* mpPS = nullptr;
	ID3D11InputLayout* mpIL = nullptr;

	struct sMeshCBuf {
		DirectX::XMMATRIX wmtx;
	};
	cConstBuffer<sMeshCBuf> mConstBuf;
public:
	cModel() {}
	~cModel() {}

	bool init(cModelData const& mdlData);
	void deinit();

	void disp();
};