#include <DirectXMath.h>

#define constexpr

inline constexpr float DEG2RAD(float deg) {
	return deg * DirectX::XM_PI / 180.f;
}

inline constexpr float RAD2DEG(float rad) {
	return rad * 180.f / DirectX::XM_PI;
}


struct vec3 {
	DirectX::XMFLOAT3 mVal;
};