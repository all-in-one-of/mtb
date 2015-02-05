#include "shader.hlsli"

struct sLightCtx {
	float3 El;
	float3 l;
};

struct CTX {
	float4 cpos;
	float4 wpos;
	float3 wnrm;
	float2 uv;
	float3 base;
	float3 diff;
	float3 spec;
	float  alpha;
	float3 clr;
	

	sLightCtx lctx;
};

sLightCtx set_light_ctx() {
	sLightCtx lc = (sLightCtx)0;
	lc.El = float3(1, 1, 1);
	lc.l = -float3(0, -1, -1);
	return lc;
}

CTX set_ctx(sPSModel pin) {
	CTX ctx = (CTX)0;
	ctx.cpos = pin.cpos;
	ctx.wpos = pin.wpos;
	ctx.wnrm = normalize(pin.wnrm);
	ctx.uv   = pin.uv;
	//ctx.nrm = pin.nrm;
	ctx.base = float3(1, 1, 1);
	ctx.diff = float3(0, 0, 0);
	ctx.spec = float3(0, 0, 0);
	ctx.alpha = 1.0;
	ctx.clr = float3(0, 0, 0);
	
	ctx.lctx = set_light_ctx();
	return ctx;
}



CTX shade(CTX ctx) {
	float3 dir = normalize(ctx.wpos - g_camPos).xyz;
	float3 h = normalize(dir + ctx.lctx.l);
	float cosTh = saturate(dot(ctx.wnrm, h));
	float cosTi = saturate(dot(ctx.wnrm, ctx.lctx.l));

	float3 kd = { 0, 0, 0 };
	float3 ks = { 1, 1, 1 };
	float m = 2;
	ctx.diff += (kd + ks * pow(cosTh, m)) * ctx.lctx.El * cosTi;

	return ctx;
}

CTX noshade(CTX ctx) {
	ctx.diff = float3(1, 1, 1);
	return ctx;
}



CTX sample_base(CTX ctx) {
	ctx.base = g_meshDiffTex.Sample(g_meshDiffSmp, ctx.uv).rgb;
	return ctx;
}

float4 main(sPSModel pin) : SV_TARGET
{
	CTX ctx = set_ctx(pin);
	//float4 clr = float4(1.0f, 1.0f, 1.0f, 1.0f);
	//clr = float4(pin.nrm, 1);
	//float3 rgb = pin.clr.rgb;
	//rgb = rgb * 0.55 + 0.225;
	//clr.rgb = rgb;
	//clr.a = pin.clr.a;

	ctx = sample_base(ctx);
	ctx = noshade(ctx);
	
	ctx.clr = ctx.base * ctx.diff + ctx.spec;
	//ctx.clr = ctx.wnrm;


	return float4(ctx.clr, ctx.alpha);
}
