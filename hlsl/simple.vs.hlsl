#include "shader.hlsl"

void main(sVSSimple vin, out sPSSimple vout) {
	vout.cpos = vin.pos;
	vout.clr = vin.clr;
}
