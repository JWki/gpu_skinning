#include "Common.hlslh"

float4 main(VS_SurfaceOutput input) : SV_TARGET
{
    float3 n = normalize(input.normal.xyz);
    float3 l = -normalize(float3(0.5, -0.5, 1));
    float lambert = saturate(dot(n, l));

	return float4(lambert, lambert, lambert, 1.0f);
}