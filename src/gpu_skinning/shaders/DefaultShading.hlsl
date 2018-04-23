#include "Common.hlslh"

float4 main(VS_SurfaceOutput input) : SV_TARGET
{
    float3 n = normalize(input.normal.xyz);
    float3 l = -normalize(float3(-1, -1, 1));
    float lambert = saturate(dot(n, l));

    return float4(n * 0.5f + 0.5f, 1.0f);
	return float4(lambert, lambert, lambert, 1.0f);
}