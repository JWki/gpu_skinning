#include "Common.hlslh"

cbuffer Skeleton : register(b0) {
    float4x4    BoneTransform[MAX_NUM_BONES];
};

cbuffer Object : register(b1) {
    float4x4    Transform;
};

cbuffer Frame : register(b2) {
    float4x4    Camera;
    float4x4    Projection;
    float4x4    CameraProjection;
};

struct Vertex
{
    float3  position : POSITION;
    float3  normal : NORMAL;
    float2  uv : TEXCOORD;
    float4  tangent : TANGENT;
    float4  blendWeights : BLENDWEIGHT;
    uint4   blendIndices : BLENDINDICES;
};

VS_SurfaceOutput main(Vertex vertex)
{
    VS_SurfaceOutput output;

    output.normal = mul(CameraProjection, mul(Transform, float4(vertex.normal, 0.0f)));
    output.texcoords = vertex.uv;

    output.worldPos = float4(0.0f, 0.0f, 0.0f, 0.0f); 
    float4 worldNormal = float4(0.0f, 0.0f, 0.0f, 0.0f);
    for (int i = 0; i < 4; ++i) {
        output.worldPos += mul(BoneTransform[vertex.blendIndices[i]], float4(vertex.position, 1.0f)) * vertex.blendWeights[i];
        worldNormal += mul(BoneTransform[vertex.blendIndices[i]], float4(vertex.normal, 0.0f)) * vertex.blendWeights[i];
    }
    output.worldPos = mul(Transform, output.worldPos);
    //output.worldPos = mul(Transform, float4(vertex.position, 1.0f));
    //worldNormal = mul(Transform, float4(vertex.normal, 0.0f));
    //output.worldPos.w = 1.0f;
    output.pos = mul(CameraProjection, output.worldPos);
    output.normal = normalize(mul(Transform, worldNormal));
    output.color = vertex.blendWeights;
    //output.pos = float4(vertex.position, 1.0f);
	return output;
}