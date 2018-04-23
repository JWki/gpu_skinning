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
    float4  blendWeights : BLENDWEIGHT;
    uint4   blendIndices : BLENDINDICES;
};

VS_SurfaceOutput main(Vertex vertex)
{
    VS_SurfaceOutput output;

    output.normal = mul(CameraProjection, mul(Transform, float4(vertex.normal, 0.0f)));
    output.texcoords = vertex.uv;

    output.worldPos = mul(BoneTransform[vertex.blendIndices[0]], float4(vertex.position, 1.0f)) * vertex.blendWeights[0];
    for (int i = 1; i < 4; ++i) {
        output.worldPos += mul(BoneTransform[vertex.blendIndices[i]], float4(vertex.position, 1.0f)) * vertex.blendWeights[i];
    }
    output.pos = mul(CameraProjection, output.worldPos);

    //output.pos = float4(vertex.position, 1.0f);
	return output;
}