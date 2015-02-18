#include "shader.hlsli"


void main(sVSModelSolid vin, out sPSModel vout)
{
	float4 pos = float4(vin.pos.xyz, 1);
	float4 wpos = mul(pos, g_world);
	float4 cpos = mul(wpos, g_viewProj);

	float4 nrm = float4(vin.nrm, 0);
	float4 tgt = float4(vin.tgt.xyz, 0);
	float4 bitgt = float4(vin.bitgt, 0);
	float3 wnrm = mul(nrm, g_world).xyz;
	float3 wtgt = mul(tgt, g_world).xyz;
	float3 wbitgt = mul(bitgt, g_world).xyz;

	vout.cpos = cpos;
	vout.wpos = wpos;
	vout.wnrm = wnrm;
	vout.uv = vin.uv;
	vout.wtgt = float4(wtgt, vin.tgt.w);
	vout.wbitgt = wbitgt;
	vout.uv1 = vin.uv1;
	vout.clr = float4(vin.clr, 1);
}
