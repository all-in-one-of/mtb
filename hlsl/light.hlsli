
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
