struct sLightPoint {
	vec4 pos;
	vec4 clr;
	bool enabled;

	template <class Archive>
	void serialize(Archive& arc);
};


class cLightMgr : noncopyable {
	sSHCoef mSH;
	sLightPoint mPointLights[4];
public:
	static cLightMgr& get();

	cLightMgr();

	void update();
	void dbg_ui();
	
	void set_default();

	bool load(cstr filepath);
	bool save(cstr filepath);

	template <class Archive>
	void serialize(Archive& arc);
};