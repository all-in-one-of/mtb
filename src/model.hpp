#include <memory>
#include <string>

struct sModelVtx;
class cShader;

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

	//cModelData& operator=(cModelData&) = delete;

	bool load(cstr filepath);
	void unload();

	bool load_assimp(cstr filepath);
	bool load_hou_geo(cstr filepath);
};


struct sGroupMaterial {
	sTestMtlCBuf params;
	std::string texBaseName;
	std::string texNmapName;
public:
	void apply(ID3D11DeviceContext* pCtx) const;
	void set_default();

	template <class Archive>
	void serialize(Archive& arc);
};

struct sGroupMtlRes {
	cTexture* mpTexBase = nullptr;
	ID3D11SamplerState* mpSmpBase = nullptr;
	cTexture* mpTexNmap = nullptr;
	ID3D11SamplerState* mpSmpNmap = nullptr;

	void apply(ID3D11DeviceContext* pCtx);
};

class cModelMaterial {
public:
	cModelData const* mpMdlData = nullptr;
	std::unique_ptr<sGroupMaterial[]> mpGrpMtl;
	std::unique_ptr<sGroupMtlRes[]> mpGrpRes;

	std::string mFilepath;

	cModelMaterial() {}
	cModelMaterial(cModelMaterial&& o) :
		mpMdlData(o.mpMdlData),
		mpGrpMtl(std::move(o.mpGrpMtl))
	{}
	cModelMaterial& operator=(cModelMaterial&& o) {
		mpMdlData = o.mpMdlData;
		mpGrpMtl = std::move(o.mpGrpMtl);
		return *this;
	}

	void apply(ID3D11DeviceContext* pCtx, int grp);

	bool load(ID3D11Device* pDev, cModelData const& mdlData, cstr filepath);
	bool save(cstr filepath = nullptr);
	void unload();

	cstr get_grp_name(uint32_t i) const { return mpMdlData->mpGrpNames[i].c_str(); }

protected:
	bool serialize(cstr filepath);
	bool deserialize(cstr filepath);
};

class cModel {
	cModelData const* mpData = nullptr;
	cModelMaterial* mpMtl = nullptr;

	cShader* mpVS = nullptr;
	cShader* mpPS = nullptr;
	com_ptr<ID3D11InputLayout> mpIL;

public:
	
	
	cModel() {}
	~cModel() {}

	bool init(cModelData const& mdlData, cModelMaterial& mtl);
	void deinit();

	void disp();

	void dbg_ui();
};
