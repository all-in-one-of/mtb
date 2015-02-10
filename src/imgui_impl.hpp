class cGfx;
class cShader;
struct ImDrawList;
struct ID3D11InputLayout;

class cImgui : noncopyable {
	cShader* mpVS = nullptr;
	cShader* mpPS = nullptr;
	ID3D11InputLayout* mpIL = nullptr;
	cTexture mFontTex;
	cVertexBuffer mVtx;
public:
	cImgui(cGfx& gfx);
	~cImgui();

	cImgui& operator=(cImgui&& o) {
		mFontTex = std::move(o.mFontTex);
		return *this;
	}

	void update();
	void disp();

	static cImgui& get();

protected:
	static void render_callback_st(ImDrawList** const draw_lists, int count);
	void render_callback(ImDrawList** const draw_lists, int count);

	void load_fonts();
};