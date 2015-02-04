#include "shader.hlsli"


void main(sVSModelSolid vin, out sPSModel vout)
{
	float4 pos = float4(vin.pos.xyz, 1);
	float4 nrm = float4(vin.nrm, 0);
	float4 wpos = mul(pos, g_world);
	float3 wnrm = mul(nrm, g_world).xyz;
	float4 cpos = mul(wpos, g_viewProj);

	vout.cpos = cpos;
	vout.wpos = wpos;
	vout.wnrm = wnrm;
}
