#include "shader.hlsli"


void main(sVSModel vin, out sPSModel vout)
{
	float4x4 w0 = g_skin[vin.jidx[0]] * vin.jwgt[0];
	float4x4 w1 = g_skin[vin.jidx[1]] * vin.jwgt[1];
	float4x4 w2 = g_skin[vin.jidx[2]] * vin.jwgt[2];
	float4x4 w3 = g_skin[vin.jidx[3]] * vin.jwgt[3];


	float4x4 world = w0 + w1 + w2 + w3;
	//float4x4 world = w0 + w1;

	float4 pos = float4(vin.pos.xyz, 1);
	float4 wpos = mul(pos, world);
	float4 cpos = mul(wpos, g_viewProj);

	float4 nrm = float4(vin.nrm, 0);
	float4 tgt = float4(vin.tgt.xyz, 0);
	float4 bitgt = float4(vin.bitgt, 0);
	float3 wnrm = mul(nrm, world).xyz;
	float3 wtgt = mul(tgt, world).xyz;
	float3 wbitgt = mul(bitgt, world).xyz;

	vout.cpos = cpos;
	vout.wpos = wpos;
	vout.wnrm = wnrm;
	vout.uv = vin.uv;
	vout.wtgt = float4(wtgt, vin.tgt.w);
	vout.wbitgt = wbitgt;
	vout.uv1 = vin.uv1;
	vout.clr = float4(vin.clr, 1);
}
