#include "shader.hlsli"

struct sLightCtx {
	float3 lpos;
	float3 ldir;
	float3 lclr;
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
	lc.ldir = -float3(0, 0, -1);
	lc.lclr = float3(1, 1, 1) * 1.0;
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

float3 fresnel(float3 f0, float HL) {
	return f0 + (1 - f0) * pow(1 - HL, 5);
}

float3 lambert_brdf(float3 c) {
	//return c / PI;
	return c;
}

float3 phong_brdf(float3 c, float RV, float shin) {
	return c * pow(RV, shin);
}

float3 blinn_phong_brdf(float3 c, float HN, float shin) {
	return c * pow(HN, shin);
}

CTX shade(CTX ctx) {
	float3 N = ctx.wnrm;
	float3 L = ctx.lctx.ldir;
	float3 R = reflect(-L, N);
	float LN = saturate(dot(L, N));

	float3 V = normalize(g_camPos - ctx.wpos).xyz;
	float3 H = normalize(V + L);
		
	float HN = saturate(dot(H, N));
	float HV = saturate(dot(H, V));
	float HL = saturate(dot(H, L));
	float RV = saturate(dot(R, V));

	float shin = g_shin;
	float3 f0 = g_fresnel.xyz;
	//float3 f0 = g_fresnel.xxx;

	ctx.diff += lambert_brdf(ctx.lctx.lclr) * LN;
	//ctx.spec += /*ctx.lctx.lclr * pow(max(0.001, HN), shin) * */fresnel(F0, HV)/* * LN*/;
	//ctx.spec = fresnel(f0, HL) * LN;
	//ctx.spec = V;
	//ctx.spec = phong_brdf(ctx.lctx.lclr, RV, shin) * LN;
	ctx.spec = blinn_phong_brdf(ctx.lctx.lclr, HN, shin) * fresnel(f0, HL) * LN;

	return ctx;
}

CTX noshade(CTX ctx) {
	ctx.diff = float3(1, 1, 1);
	return ctx;
}

float4 combine(CTX ctx){
	ctx.clr = ctx.base * ctx.diff + ctx.spec;
	//ctx.clr = ctx.spec;

	ctx.clr = pow(ctx.clr, 1.0 / g_gamma);

	return float4(ctx.clr, ctx.alpha);
}

CTX sample_base(CTX ctx) {
	//ctx.base = g_meshDiffTex.Sample(g_meshDiffSmp, ctx.uv).rgb;
	//ctx.base = float3(1.0, 1.0, 1.0);
	ctx.base = float3(1.0, 1.0, 1.0);
	return ctx;
}

float4 main(sPSModel pin) : SV_TARGET
{
	CTX ctx = set_ctx(pin);
	ctx = sample_base(ctx);
	ctx = shade(ctx);
	float4 clr = combine(ctx);
	return clr;
}
