#include "shader.hlsli"


void main(sVSModelSolid vin, out sPSSimple vout)
{
	float4 pos = float4(vin.pos.xyz, 1);
	float4 wpos = mul(pos, g_world);
	float4 cpos = mul(wpos, g_viewProj);

	vout.cpos = cpos;
	vout.clr = float4(vin.nrm, 1.0);
}
