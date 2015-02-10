struct ID3D11Device;
struct ID3D11Resource;
struct ID3D11ShaderResourceView;
struct ID3D11DeviceContext;

class cTexture : noncopyable {
	moveable_ptr<ID3D11Resource> mpTex;
	moveable_ptr<ID3D11ShaderResourceView> mpView;
public:

	cTexture() {}
	cTexture(cTexture&& o) : mpTex(std::move(o.mpTex)), mpView(std::move(o.mpView)) {}
	cTexture(ID3D11Resource* pTex, ID3D11ShaderResourceView* pView) : mpTex(pTex), mpView(pView) {}
	~cTexture() { unload(); }
	cTexture& operator=(cTexture&& o) {
		mpTex = std::move(o.mpTex); 
		mpView = std::move(o.mpView);
		return *this;
	}

	bool create2d1_rgba8(ID3D11Device* pDev, void* data, uint32_t w, uint32_t h);
	bool load(ID3D11Device* pDev, cstr filepath);
	void unload();

	void set_PS(ID3D11DeviceContext* pCtx, uint32_t slot);
};
