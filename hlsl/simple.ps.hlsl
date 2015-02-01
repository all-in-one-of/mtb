#include "shader.hlsl"

float4 main(sPSSimple pin) : SV_TARGET
{

	float4 clr = float4(1.0f, 1.0f, 1.0f, 1.0f);
	clr = pin.clr;
	return clr;
}
