
struct LightInfo {
	float3 dir;
	float3 L;
	float3 H;
	
	float LN;
	float HN;
	float HL;

	float3 clrDiff;
	float3 clrSpec;
};

struct LightParam {
	float3 pos;
	float3 dir;
	float3 clr;
};

LightInfo get_light_info_omni(CTX ctx, LightParam prm) {
	LightInfo info;

	info.dir = prm.pos - ctx.wpos.xyz;
	info.L = normalize(info.dir);
	info.H = normalize(ctx.viewDir + info.L);

	float3 N = ctx.wnrm;
	info.LN = max(dot(info.L, N), 0);
	info.HN = max(dot(info.H, N), 0);
	info.HL = max(dot(info.H, info.L), 0);

	info.clrDiff = prm.clr;
	info.clrSpec = prm.clr;

	return info;
}

LightParam get_light_param(int i) {
	LightParam lp;

	lp.pos = g_lightPos[i];
	lp.dir = g_lightDir[i];
	lp.clr = g_lightClr[i];

	return lp;
}


// See "Stupid Spherical Harmonics (SH) Tricks" by Peter-Pike Sloan, Appendix A10
float3 get_light_sh(float4 SH[7], float3 wnrm) {
	float4 N = float4(wnrm, 1);

	float3 x1, x2, x3;
	x1.r = dot(SH[0], N);
	x1.g = dot(SH[1], N);
	x1.b = dot(SH[2], N);

	float4 NN = N.xyzz * N.yzzx;
	x2.r = dot(SH[3], NN);
	x2.g = dot(SH[4], NN);
	x2.b = dot(SH[5], NN);

	float c = N.x * N.x - N.y * N.y;
	x3 = SH[6].rgb * c;

	return x1 + x2 + x3;
}

float3 get_light_sh(float3 wnrm) {
	return get_light_sh(g_lightSH, wnrm);
}

