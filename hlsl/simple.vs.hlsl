#include "shader.hlsl"

cbuffer Test : register(b0) {
	float4 clrDiff;
};

void main(sVSSimple vin, out sPSSimple vout) {
	vout.cpos = vin.pos;
	vout.clr = vin.clr * clrDiff;
}
