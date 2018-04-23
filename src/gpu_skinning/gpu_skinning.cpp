#include "imgui/imgui.h"


#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <d3d11.h>
#include <d3dcompiler.h>
#include <Windows.h>
///
#include <stdint.h>
#include <stdio.h>
#include "math.h"

//
void* operator new[](size_t size, const char* pName, int flags, unsigned debugFlags, const char* file, int line)
{
    return malloc(size);
}
void* operator new[](size_t size, size_t alignment, size_t alignmentOffset, const char* pName, int flags, unsigned debugFlags, const char* file, int line)
{
    return malloc(size);
}
///

struct ShaderDesc
{
    char*       vertexShaderCode        = nullptr;
    uint32_t    vertexShaderCodeSize    = 0;

    char*       pixelShaderCode         = nullptr;
    uint32_t    pixelShaderCodeSize     = 0;
};

struct Shader
{
    ID3D11VertexShader* vertexShader    = nullptr;
    ID3D11PixelShader*  pixelShader     = nullptr;
};

bool CreateShader(ID3D11Device* device, ShaderDesc* desc, Shader* shader)
{
    {   ///
        auto res = device->CreateVertexShader(desc->vertexShaderCode, desc->vertexShaderCodeSize, nullptr, &shader->vertexShader);
        if (!SUCCEEDED(res)) {
            printf("Failed to create vertex shader\n");
            return false;
        }
    }
    {   ///
        auto res = device->CreatePixelShader(desc->pixelShaderCode, desc->pixelShaderCodeSize, nullptr, &shader->pixelShader);
        if (!SUCCEEDED(res)) {
            printf("Failed to create pixel shader\n");
            return false;
        }
    }
    return true;
}


struct Vertex
{
    math::Vec3  position;
    math::Vec3  normal;
    float       uv[2];
    float       blendWeights[4];
    uint32_t    blendIndices[4];

    static ID3D11InputLayout* GetInputLayout(ID3D11Device* device, ShaderDesc* shaderDesc)
    {
        static ID3D11InputLayout* layout = nullptr;
        if (!layout) {
            D3D11_INPUT_ELEMENT_DESC elementDesc[] = {
                { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, position), D3D11_INPUT_PER_VERTEX_DATA, 0 },
                { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, normal), D3D11_INPUT_PER_VERTEX_DATA, 0 },
                { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(Vertex, uv), D3D11_INPUT_PER_VERTEX_DATA, 0 },
                { "BLENDWEIGHT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(Vertex, blendWeights), D3D11_INPUT_PER_VERTEX_DATA, 0 },
                { "BLENDINDICES", 0, DXGI_FORMAT_R32G32B32A32_UINT, 0, offsetof(Vertex, blendIndices), D3D11_INPUT_PER_VERTEX_DATA, 0 },
            };
            auto res = device->CreateInputLayout(elementDesc, 5, shaderDesc->vertexShaderCode, shaderDesc->vertexShaderCodeSize, &layout);
            if (!SUCCEEDED(res)) {
                printf("Failed to create input layout\n");
                layout = nullptr;
            }
        }
        return layout;
    }
};

using IndexType = uint32_t;
struct MeshDesc
{
    Vertex*     vertices = nullptr;
    IndexType*  indices = nullptr;

    uint32_t    numVertices = 0;
    uint32_t    numIndices = 0;
};

struct Mesh 
{
    ID3D11Buffer* vertexBuffer = nullptr;
    ID3D11Buffer* indexBuffer = nullptr;
    
    uint32_t numElements = 0;
};

bool CreateMesh(ID3D11Device* device, MeshDesc* desc, Mesh* mesh)
{
    {   //
        D3D11_BUFFER_DESC bufferDesc;
        ZeroMemory(&bufferDesc, sizeof(D3D11_BUFFER_DESC));

        bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        bufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
        bufferDesc.CPUAccessFlags = 0;
        bufferDesc.MiscFlags = 0;
        bufferDesc.ByteWidth = sizeof(Vertex) * desc->numVertices;
        bufferDesc.StructureByteStride = sizeof(Vertex);

        D3D11_SUBRESOURCE_DATA data;
        ZeroMemory(&data, sizeof(D3D11_SUBRESOURCE_DATA));
        data.pSysMem = desc->vertices;
        data.SysMemPitch = bufferDesc.ByteWidth;

        auto res = device->CreateBuffer(&bufferDesc, &data, &mesh->vertexBuffer);
        if (!SUCCEEDED(res)) {
            printf("Failed to create vertex buffer\n");
            return false;
        }
    }
    {   //
        D3D11_BUFFER_DESC bufferDesc;
        ZeroMemory(&bufferDesc, sizeof(D3D11_BUFFER_DESC));

        bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        bufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
        bufferDesc.CPUAccessFlags = 0;
        bufferDesc.MiscFlags = 0;
        bufferDesc.ByteWidth = sizeof(IndexType) * desc->numIndices;
        bufferDesc.StructureByteStride = sizeof(IndexType);

        D3D11_SUBRESOURCE_DATA data;
        ZeroMemory(&data, sizeof(D3D11_SUBRESOURCE_DATA));
        data.pSysMem = desc->indices;
        data.SysMemPitch = bufferDesc.ByteWidth;

        auto res = device->CreateBuffer(&bufferDesc, &data, &mesh->indexBuffer);
        if (!SUCCEEDED(res)) {
            printf("Failed to create index buffer\n");
            return false;
        }
    }

    mesh->numElements = desc->numIndices;

    return true;
}
///

struct ObjectConstantData
{
    float transform[16];
};

struct FrameConstantData
{
    float camera[16];
    float projection[16];
    float cameraProjection[16];
};

struct SkeletonConstantData
{
    float boneTransform[64][16];
};
///


///
static void* Win32LoadFileContents(const char* path, uint32_t* outFileSize)
{
    HANDLE handle = CreateFileA(path, GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (handle == INVALID_HANDLE_VALUE) {
        printf("Failed to load %s\n", path);
        return nullptr;
    }
    DWORD size = GetFileSize(handle, NULL);
    void* buffer = malloc(size);
    memset(buffer, 0x0, size);  // for text files
    DWORD bytesRead = 0;
    auto res = ReadFile(handle, buffer, size, &bytesRead, NULL);
    if (res == FALSE || bytesRead != size) {
        printf("Failed to read %s - bytes read: %lu / %lu\n", path, bytesRead, size);
        free(buffer);
        CloseHandle(handle);
        return nullptr;
    }
    if (outFileSize) {
        *outFileSize = (uint32_t)size;
    }
    CloseHandle(handle);
    return buffer;
}

///
struct AppData
{   
    ShaderDesc shaderDesc;

    Mesh    testMesh;
    Shader  shader;

    ID3D11Buffer* frameConstantBuffer;
    ID3D11Buffer* objectConstantBuffer;
    ID3D11Buffer* skeletonConstantBuffer;

    
} g_data;



///
struct ByteStream
{
    char*   buffer = nullptr;
    size_t  bufferSize = 0;
    size_t  offset = 0;

    template <class T>
    T Read()
    {
        T res;
        auto readSize = ReadBytes(&res, sizeof(res));
        //GT_ASSERT(readSize == sizeof(res));
        return res;
    }

    size_t ReadBytes(void* dest, size_t numBytes)
    {
        numBytes = (offset + numBytes) <= bufferSize ? numBytes : bufferSize - offset;
        memcpy(dest, buffer + offset, numBytes);
        offset += numBytes;
        return numBytes;
    }
};


bool LoadMeshFromFile(const char* path, Mesh* outMesh, ID3D11Device* device)
{
    uint32_t fileSize;
    ByteStream stream;
    stream.buffer = (char*)Win32LoadFileContents(path, &fileSize);
    stream.bufferSize = (size_t)(fileSize);
    stream.offset = 0;

    uint32_t numVerticesTotal = 0;
    uint32_t numIndicesTotal = 0;

    uint32_t numSubmeshes = stream.Read<uint32_t>();
    MeshDesc* submeshDesc = new MeshDesc[numSubmeshes];
    for (auto i = 0u; i < numSubmeshes; ++i) {
        auto& meshResource = submeshDesc[i];
        stream.Read<uint32_t>();

        meshResource.numVertices = stream.Read<uint32_t>();
        numVerticesTotal += meshResource.numVertices;
        meshResource.vertices = new Vertex[meshResource.numVertices];
        for (auto j = 0u; j < meshResource.numVertices; ++j) {
            auto& vertex = meshResource.vertices[j];
            vertex.position.x = stream.Read<float>();
            vertex.position.y = stream.Read<float>();
            vertex.position.z = stream.Read<float>();

            vertex.normal.x = stream.Read<float>();
            vertex.normal.y = stream.Read<float>();
            vertex.normal.z = stream.Read<float>();

            vertex.uv[0] = stream.Read<float>();
            vertex.uv[1] = 1.0f - stream.Read<float>();

            vertex.blendWeights[0] = stream.Read<float>();
            vertex.blendWeights[1] = stream.Read<float>();
            vertex.blendWeights[2] = stream.Read<float>();
            vertex.blendWeights[3] = stream.Read<float>();

            vertex.blendIndices[0] = stream.Read<uint32_t>();
            vertex.blendIndices[1] = stream.Read<uint32_t>();
            vertex.blendIndices[2] = stream.Read<uint32_t>();
            vertex.blendIndices[3] = stream.Read<uint32_t>();

        }
        meshResource.numIndices = stream.Read<uint32_t>();
        numIndicesTotal += meshResource.numIndices;
        meshResource.indices = new IndexType[meshResource.numIndices];
        for (auto j = 0u; j < meshResource.numIndices; ++j) {
            meshResource.indices[j] = stream.Read<IndexType>();
        }
        /*IndexType* it = meshResource.indices;
        for (auto i = 0u; i < (meshResource.numIndices / 3); ++i) {
            auto swap = it[0];
            it[0] = it[2];
            it[2] = swap;
            it += 3;
        }*/
    }

    MeshDesc meshDesc;
    meshDesc.numVertices = numVerticesTotal;
    meshDesc.numIndices = numIndicesTotal;

    meshDesc.vertices = new Vertex[numVerticesTotal];
    meshDesc.indices = new IndexType[numIndicesTotal];

    Vertex* writeVertexPtr = meshDesc.vertices;
    IndexType* writeIndexPtr = meshDesc.indices;
    IndexType indexOffset = 0;

    for (auto i = 0u; i < numSubmeshes; ++i) {
        auto& submesh = submeshDesc[i];
        memcpy(writeVertexPtr, submesh.vertices, sizeof(Vertex) * submesh.numVertices);
        writeVertexPtr += submesh.numVertices;
        memcpy(writeIndexPtr, submesh.indices, sizeof(IndexType) * submesh.numIndices);
        for (auto j = 0u; j < submesh.numIndices; ++j) {
            writeIndexPtr[j] += indexOffset;
        }
        writeIndexPtr += submesh.numIndices;
        indexOffset += submesh.numVertices;
    }

    auto res = CreateMesh(device, &meshDesc, outMesh);
    delete[] submeshDesc;
    return res;
}

///
void AppInit(HWND hWnd, ID3D11Device* device, ID3D11DeviceContext* deviceContext)
{
#ifdef GT_DEBUG
    static const char* vShaderPath = "bin/Debug/SkinnedGeometry.cso";
    static const char* pShaderPath = "bin/Debug/DefaultShading.cso";
#else
    static const char* vShaderPath = "bin/Release/SkinnedGeometry.cso";
    static const char* pShaderPath = "bin/Release/DefaultShading.cso";
#endif

    g_data.shaderDesc.vertexShaderCode = (char*)Win32LoadFileContents(vShaderPath, &g_data.shaderDesc.vertexShaderCodeSize);
    g_data.shaderDesc.pixelShaderCode = (char*)Win32LoadFileContents(pShaderPath, &g_data.shaderDesc.pixelShaderCodeSize);
    if (!CreateShader(device, &g_data.shaderDesc, &g_data.shader)) {
        printf("Failed to create shader\n");
        return;
    }
    printf("Created shader\n");


    /*{   ///
        static Vertex vertices[] = {
            { {-0.5f, -0.5f, 0.0f}, { 0.0f, 0.0f, -1.0f }, { 0.0f, 0.0f }, { 0.25f, 0.25f, 0.25f, 0.0f }, { 0, 1, 2, 3 } },
            { { 0.f, 0.5f, 0.0f },{ 0.0f, 0.0f, -1.0f },{ 0.0f, 0.0f },{ 0.0f, 1.0f, 0.0f, 0.0f },{ 0, 1, 2, 3 } },
            { { 0.5f, -0.5f, 0.0f },{ 0.0f, 0.0f, -1.0f },{ 0.0f, 0.0f },{ 0.0f, 0.0f, 1.0f, 0.0f },{ 0, 1, 2, 3 } },
        };

        static IndexType indices[] = {
            0, 1, 2
        };

        MeshDesc desc;
        desc.vertices = vertices;
        desc.indices = indices;
        desc.numVertices = 3;
        desc.numIndices = 3;

        if (!CreateMesh(device, &desc, &g_data.testMesh)) {
            printf("Failed to create test mesh\n");
            return;
        }
        printf("Created test mesh\n");
    }
    */
    if (!LoadMeshFromFile("assets/character.gtmesh", &g_data.testMesh, device)) {
        printf("failed to load test mesh from %s\n", "assets/character.gtmesh");
        return;
    }
    printf("Created test mesh\n");

    {   ///
        {   // frame constant data
            D3D11_BUFFER_DESC desc;
            ZeroMemory(&desc, sizeof(D3D11_BUFFER_DESC));

            desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
            desc.Usage = D3D11_USAGE_DYNAMIC;
            desc.ByteWidth = sizeof(FrameConstantData);
            desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            auto res = device->CreateBuffer(&desc, nullptr, &g_data.frameConstantBuffer);
            if (!SUCCEEDED(res)) {
                printf("Failed to create frame constant buffer\n");
                return;
            }
        }
        {   // object constant data
            D3D11_BUFFER_DESC desc;
            ZeroMemory(&desc, sizeof(D3D11_BUFFER_DESC));

            desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
            desc.Usage = D3D11_USAGE_DYNAMIC;
            desc.ByteWidth = sizeof(ObjectConstantData);
            desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            auto res = device->CreateBuffer(&desc, nullptr, &g_data.objectConstantBuffer);
            if (!SUCCEEDED(res)) {
                printf("Failed to create object constant buffer\n");
                return;
            }
        }
        {   // frame constant data
            D3D11_BUFFER_DESC desc;
            ZeroMemory(&desc, sizeof(D3D11_BUFFER_DESC));

            desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
            desc.Usage = D3D11_USAGE_DYNAMIC;
            desc.ByteWidth = sizeof(SkeletonConstantData);
            desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            auto res = device->CreateBuffer(&desc, nullptr, &g_data.skeletonConstantBuffer);
            if (!SUCCEEDED(res)) {
                printf("Failed to create skeleton constant buffer\n");
                return;
            }
        }
    }

    
}


///
void AppUpdate(HWND hWnd, ID3D11Device* device, ID3D11DeviceContext* deviceContext)
{
    
}
///
void AppRender(HWND hWnd, ID3D11Device* device, ID3D11DeviceContext* deviceContext)
{
    D3D11_VIEWPORT vp;
    {   // viewport, etc
        RECT rect;
        GetClientRect(hWnd, &rect);
        
        ZeroMemory(&vp, sizeof(D3D11_VIEWPORT));
        vp.MinDepth = 0.0f;
        vp.MaxDepth = 1;
        vp.Width = float(rect.right - rect.left);
        vp.Height = float(rect.bottom - rect.top);
        vp.TopLeftX = 0;
        vp.TopLeftY = 0;

        deviceContext->RSSetViewports(1, &vp);
    }

    {   // update constant buffers
        static FrameConstantData frameData;    
        math::Make4x4FloatProjectionMatrixCMLH(frameData.projection, math::DegreesToRadians(60.0f), vp.Width, vp.Height, 0.1f, 100.0f);
        float inverseCamera[16];
        math::Make4x4FloatLookAtMatrixCMLH(inverseCamera, math::Vec3(0.0f, 2.5f, -2.5f), math::Vec3(0.0f, 1.0f, 0.0f), math::Vec3(0.0f, 1.0f, 0.0f));
        math::Inverse4x4FloatMatrixCM(inverseCamera, frameData.camera);
        math::MultiplyMatricesCM(frameData.projection, frameData.camera, frameData.cameraProjection);

        static ObjectConstantData objectData;
        math::Make4x4FloatMatrixIdentity(objectData.transform);

        static SkeletonConstantData skeletonData;
        for (auto i = 0; i < 64; ++i) {
            math::Make4x4FloatMatrixIdentity(skeletonData.boneTransform[i]);
        }
        static float anim = 0.0f;
       // anim += 0.001f;   // animate root 
        math::Make4x4FloatTranslationMatrixCM(skeletonData.boneTransform[0], math::Vec3(0.0f, math::Sin(anim), 0.0f));


        {   // frame data
            D3D11_MAPPED_SUBRESOURCE resource;
            auto res = deviceContext->Map(g_data.frameConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource);
            if (!SUCCEEDED(res)) { printf("Failed to map frame constant buffer!\n"); }
            memcpy(resource.pData, &frameData, sizeof(FrameConstantData));
            deviceContext->Unmap(g_data.frameConstantBuffer, 0);
        }
        {   // object data
            D3D11_MAPPED_SUBRESOURCE resource;
            auto res = deviceContext->Map(g_data.objectConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource);
            if (!SUCCEEDED(res)) { printf("Failed to map object constant buffer!\n"); }
            memcpy(resource.pData, &objectData, sizeof(ObjectConstantData));
            deviceContext->Unmap(g_data.objectConstantBuffer, 0);
        }
        {   // skeleton data
            D3D11_MAPPED_SUBRESOURCE resource;
            auto res = deviceContext->Map(g_data.skeletonConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource);
            if (!SUCCEEDED(res)) { printf("Failed to map skeleton constant buffer!\n"); }
            memcpy(resource.pData, &skeletonData, sizeof(SkeletonConstantData));
            deviceContext->Unmap(g_data.skeletonConstantBuffer, 0);
        }
    }

    {   // bind vertex/index buffer, input layout, shaders, constant buffers...
        uint32_t stride = sizeof(Vertex);
        uint32_t offset = 0;
        deviceContext->IASetVertexBuffers(0, 1, &g_data.testMesh.vertexBuffer, &stride, &offset);
        deviceContext->IASetIndexBuffer(g_data.testMesh.indexBuffer, DXGI_FORMAT_R32_UINT, 0);
        deviceContext->IASetInputLayout(Vertex::GetInputLayout(device, &g_data.shaderDesc));
        deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        deviceContext->VSSetShader(g_data.shader.vertexShader, nullptr, 0);
        deviceContext->PSSetShader(g_data.shader.pixelShader, nullptr, 0);

        ID3D11Buffer* cbuffers[] = {
            g_data.skeletonConstantBuffer, 
            g_data.objectConstantBuffer,
            g_data.frameConstantBuffer
        };
        deviceContext->VSSetConstantBuffers(0, 3, cbuffers);

        deviceContext->DrawIndexed(g_data.testMesh.numElements, 0, 0);
    }
}

///
void AppShutdown()
{
    
}