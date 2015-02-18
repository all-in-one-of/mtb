#include "material.hlsli"


float4 main(sPSModel pin, sPSPredefined predef) : SV_TARGET
{
	CTX ctx = set_ctx(pin);
	ctx = set_ctx_predefined(ctx, predef);
	ctx = sample_mask_alphatest(ctx);
	ctx = do_alphatest(ctx, 0.05);
	ctx = sample_base(ctx);
	ctx = apply_nmap(ctx);
	ctx.wnrm = ctx.wnrm * ctx.face;
	ctx = shade(ctx);
	float4 clr = combine(ctx);
	return clr;
}
