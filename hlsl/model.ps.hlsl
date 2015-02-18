#include "material.hlsli"


float4 main(sPSModel pin) : SV_TARGET
{
	CTX ctx = set_ctx(pin);
	ctx = sample_base(ctx);
	ctx = apply_nmap(ctx);
	ctx = shade(ctx);
	float4 clr = combine(ctx);
	return clr;
}
