class cRigData;
class cRig;
struct sXform;

struct sKeyframe {
	float frame;
	float value;
	float inSlope;
	float outSlope;
};

class cChannel : noncopyable {
public:
	enum eChannelType : uint8_t {
		E_CH_COMMON = 0,
		E_CH_EULER = 1,
		E_CH_QUATERNION = 2,

		E_CH_LAST
	};

	enum eExpressionType : uint8_t {
		E_EXPR_CONSTANT = 0,
		E_EXPR_LINEAR = 1,
		E_EXPR_CUBIC = 2,
		E_EXPR_QLINEAR = 3,
		
		E_EXPR_LAST
	};

	enum eRotOrder : uint8_t {
		E_ROT_XYZ = 0,

		E_ROT_LAST
	};

	int* mpKeyframesNum = nullptr;
	sKeyframe** mpComponents = nullptr;
	int mComponentsNum = 0;
	eChannelType mType = E_CH_COMMON;
	eExpressionType mExpr = E_EXPR_CONSTANT;
	eRotOrder mRotOrd = E_ROT_XYZ;

	std::string mName;
	std::string mSubname;
public:
	~cChannel();

	void eval(DirectX::XMVECTOR& vec, float frame) const;
private:

	void find_keyframe(int comp, float frame, sKeyframe const*& pKfrA, sKeyframe const*& pKfrB) const;
};

class cAnimationData : noncopyable {
public:
	cChannel* mpChannels = nullptr;
	int mChannelsNum = 0;
	float mLastFrame = 0.0f;

	std::string mName;
public:
	~cAnimationData();
	bool load(cstr filepath);

private:

	friend class cAnimJsonLoaderImpl;
};


class cAnimation : noncopyable {
	struct sLink {
		int16_t chIdx;
		int16_t jntIdx;
	};

	cAnimationData const* mpAnimData = nullptr;
	cRigData const* mpRigData = nullptr;
	sLink* mpLinks = nullptr;
	int mLinksNum = 0;

public:
	~cAnimation();
	void init(cAnimationData const& animData, cRigData const& rigData);

	void eval(cRig& rig, float frame);
};

