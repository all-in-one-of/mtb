#pragma pack_matrix(row_major)

struct sVSSimple {
	float4 pos : POSITION;
	float4 clr : COLOR;
};

struct sPSSimple{
	float4 cpos : SV_POSITION;
	float4 clr : COLOR0;
};

