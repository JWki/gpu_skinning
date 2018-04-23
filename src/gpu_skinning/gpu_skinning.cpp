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
    math::Vec4  tangent;
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
        if (dest == nullptr) { offset += numBytes;  return numBytes; };
        memcpy(dest, buffer + offset, numBytes);
        offset += numBytes;
        return numBytes;
    }
};

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

#define MAX_NUM_BONES 128

struct SkeletonConstantData
{
    float boneTransform[MAX_NUM_BONES][16];
};
///
struct Joint
{
    float   globalTransform[16];
    float   localTransform[16];
    int     importId;   // @HACK
    int     parent;
};

struct Skeleton
{
    float invBindpose[MAX_NUM_BONES][16];
    const char* nameTable[MAX_NUM_BONES];      // contains human readable names of joints
    Joint joints[MAX_NUM_BONES];               // actual joints
    uint32_t numJoints;
};

void TransferNode(Skeleton* source, Skeleton* target, uint32_t& writeOffset, int nodeIdx)
{
    if (source->joints[nodeIdx].importId == -1) {
        return;
    }
    target->joints[writeOffset] = source->joints[nodeIdx];
    target->nameTable[writeOffset] = source->nameTable[nodeIdx];
    memcpy(target->invBindpose[writeOffset], source->invBindpose[nodeIdx], sizeof(float) * 16);
    writeOffset++;
    if (source->joints[nodeIdx].parent != -1) {
        TransferNode(source, target, writeOffset, source->joints[nodeIdx].parent);
    }
    source->joints[nodeIdx].importId = -1;
}

void SortSkeleton(Skeleton* source, Skeleton* target)
{
    uint32_t writeOffset = 0;
    uint32_t readOffset = 0;
    while (readOffset < MAX_NUM_BONES && readOffset < source->numJoints) {
        TransferNode(source, target, writeOffset, readOffset);
        readOffset++;
    }
}

bool ImportSGA(const char* path, Skeleton* outSkeleton)
{
    uint32_t fileSize;
    ByteStream stream;
    stream.buffer = (char*)Win32LoadFileContents(path, &fileSize);
    stream.bufferSize = (size_t)(fileSize);
    stream.offset = 0;

    auto magicNumber = stream.Read<uint32_t>();
    assert(magicNumber == 383405658);

    auto version = stream.Read<uint8_t>();
    assert(version == 1);

    auto nameLen = stream.Read<uint16_t>();
    stream.ReadBytes(nullptr, nameLen);   // skip the name

    outSkeleton->numJoints = (uint32_t)stream.Read<uint16_t>(); 
    
    Skeleton tempSkeleton;
    tempSkeleton.numJoints = outSkeleton->numJoints;
    assert(outSkeleton->numJoints <= MAX_NUM_BONES);
    char* buf = new char[MAX_NUM_BONES * 512];
    memset(buf, 0x0, MAX_NUM_BONES * 512);
    uint32_t bufOffset = 0;
    int tempParentIndexTable[MAX_NUM_BONES];
    for (auto i = 0; i < MAX_NUM_BONES; ++i) { tempParentIndexTable[i] = -1; }

    for (uint32_t i = 0; i < outSkeleton->numJoints; ++i) {
        // read the bone name and store it
        auto nameLen = stream.Read<uint16_t>();
        assert(nameLen < 512);
        stream.ReadBytes(buf + bufOffset, nameLen);
        tempSkeleton.nameTable[i] = buf + bufOffset;
        bufOffset += nameLen;
        // read the bind pose
        auto& joint = tempSkeleton.joints[i];   
        auto pos = stream.Read<math::Vec3>();
        math::Make4x4FloatTranslationMatrixCM(tempSkeleton.invBindpose[i], pos);
        auto isParent = stream.Read<uint8_t>() == 1;
        if (isParent) {
            joint.parent = -1;
        }
        else {
            joint.parent = 0;
        }
        joint.importId = i;
        auto numChildren = stream.Read<uint16_t>();
        uint16_t children[MAX_NUM_BONES];
        assert(numChildren <= MAX_NUM_BONES);
        stream.ReadBytes(children, sizeof(uint16_t) * numChildren);
        for(uint16_t c = 0; c < numChildren; ++c) {
            tempParentIndexTable[children[c]] = i;
        }
    }
    for (uint16_t i = 0; i < tempSkeleton.numJoints; ++i) {
        tempSkeleton.joints[i].parent = tempParentIndexTable[i];
    }
    SortSkeleton(&tempSkeleton, outSkeleton);
    outSkeleton->numJoints = tempSkeleton.numJoints;
    for (int i = 0; i < (int)outSkeleton->numJoints; ++i) {
        assert(outSkeleton->joints[i].parent < i);
    }

    return true;
}
///
int GetBoneWithName(Skeleton* skeleton, const char* name)
{
    for (uint32_t i = 0; i < skeleton->numJoints && i < MAX_NUM_BONES; ++i) {
        if (strcmp(skeleton->nameTable[i], name) == 0) {
            return (int)i;
        }
    }
    return -1;
}

void ResetLocalTransforms(Skeleton* skeleton)
{
    for (uint32_t i = 0; i < skeleton->numJoints; ++i) {
        math::Make4x4FloatMatrixIdentity(skeleton->joints[i].localTransform);
    }
}

void ComputePose(Skeleton* skeleton, SkeletonConstantData* outBuffer)
{
    // @NOTE we assume that the skeleton is sorted so parents are always evaluated before their children
    // and also local transforms have been reset at least once so there's no BS in them
    for (uint32_t i = 0; i < skeleton->numJoints; ++i) {
        if (skeleton->joints[i].parent != -1) {
            auto& parent = skeleton->joints[skeleton->joints[i].parent];
            float temp[16];
            math::Copy4x4FloatMatrixCM(skeleton->joints[i].localTransform, temp);
            math::MultiplyMatricesCM(temp, parent.globalTransform, skeleton->joints[i].globalTransform);
        }
        else {
            math::Copy4x4FloatMatrixCM(skeleton->joints[i].localTransform, skeleton->joints[i].globalTransform);
        }
        math::Copy4x4FloatMatrixCM(skeleton->joints[i].globalTransform, outBuffer->boneTransform[i]);
    }
    //
}

///
struct AppData
{   
    ShaderDesc shaderDesc;

    Skeleton    testSkeleton;
    Mesh        testMesh;
    Shader      shader;

    ID3D11Buffer* frameConstantBuffer;
    ID3D11Buffer* objectConstantBuffer;
    ID3D11Buffer* skeletonConstantBuffer;

} g_data;



bool ImportSGM(const char* path, Mesh* outMesh, ID3D11Device* device)
{
    uint32_t fileSize;
    ByteStream stream;
    stream.buffer = (char*)Win32LoadFileContents(path, &fileSize);
    stream.bufferSize = (size_t)(fileSize);
    stream.offset = 0;

    uint32_t numVerticesTotal = 0;
    uint32_t numIndicesTotal = 0;

    // skip magic number, version number and material information
    auto magicNumber = stream.Read<uint32_t>();    // magic 
    assert(magicNumber == 352658064);
   
    auto version = stream.Read<uint8_t>();     // version
    assert(version == 3);

    auto numMaterials = stream.Read<uint8_t>();     // num materials
    for (uint8_t i = 0; i < numMaterials; ++i) {
        stream.Read<uint8_t>();     // material ID
        auto numUVSets = stream.Read<uint8_t>();   
        for (uint8_t k = 0; k < numUVSets; ++k) {
            auto numTextures = stream.Read<uint8_t>();
            for (uint8_t j = 0; j < numTextures; ++j) {
                stream.Read<uint8_t>();     // texture type hint
                auto l = stream.Read<uint16_t>();    // filename length
                char buf[512] = "";
                stream.ReadBytes(buf, l);    // filename
                assert(l < 512);
            }
        }
        auto numColors = stream.Read<uint8_t>();
        for (uint8_t j = 0; j < numColors; ++j) {
            stream.Read<uint8_t>();     // color type hint
            math::Vec4 col;
            stream.ReadBytes(&col.elements[0], sizeof(float) * 4);   // color
        }
    }
    // start reading mesh data
    auto numSubmeshes = stream.Read<uint8_t>();
    MeshDesc* submeshDesc = new MeshDesc[numSubmeshes];
    for (uint8_t i = 0; i < numSubmeshes; ++i) {
        auto& meshResource = submeshDesc[i];
        stream.Read<uint8_t>();     // submesh ID
        stream.Read<uint8_t>();     // material ID
        meshResource.numVertices = stream.Read<uint32_t>();
        numVerticesTotal += meshResource.numVertices;
        
        size_t totalVertexSize = sizeof(math::Vec3) * 2;

        auto numUVSets = stream.Read<uint8_t>();
        totalVertexSize += sizeof(float) * 2 * numUVSets;
        assert(numUVSets == 1);
        auto numColorChannels = stream.Read<uint8_t>();
        totalVertexSize += sizeof(math::Vec4) * numColorChannels;
        assert(numColorChannels == 0);

        auto hasTangents = stream.Read<uint8_t>();
        totalVertexSize += hasTangents ? sizeof(math::Vec4) : 0;
        assert(hasTangents == 1);

        auto hasBones = stream.Read<uint8_t>();
        totalVertexSize += hasBones ? sizeof(float) * 4 * 2 : 0;
        assert(hasBones == 1);

        assert(sizeof(Vertex) == totalVertexSize);

        meshResource.vertices = new Vertex[meshResource.numVertices];
        for (uint32_t vtx = 0; vtx < meshResource.numVertices; ++vtx) {     // explicit loop because we convert bone weights
            auto& vert = meshResource.vertices[vtx];

            vert.position = stream.Read<math::Vec3>();
            vert.normal = stream.Read<math::Vec3>();
            stream.ReadBytes(vert.uv, sizeof(float) * 2);
            vert.tangent = stream.Read<math::Vec4>();   
            stream.ReadBytes(vert.blendWeights, sizeof(float) * 4);
            for (auto w = 0; w < 4; ++w) {
                float indexAsFloat = stream.Read<float>();
                vert.blendIndices[w] = (uint32_t)(indexAsFloat);
            }
        }
        
        meshResource.numIndices = stream.Read<uint32_t>();
        numIndicesTotal += meshResource.numIndices;
        meshResource.indices = new IndexType[meshResource.numIndices];
        auto indexSize = stream.Read<uint8_t>();
        if (indexSize == 4) {
            stream.ReadBytes(meshResource.indices, sizeof(IndexType) * meshResource.numIndices);
        }
        else {
            for (uint32_t idx = 0; idx < meshResource.numIndices; ++idx) {
                auto smallIndex = stream.Read<uint16_t>();
                meshResource.indices[idx] = (uint32_t)(smallIndex);
            }
        }
    }
    // Merge submeshes
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

    return true;
}

bool ImportGTMesh(const char* path, Mesh* outMesh, ID3D11Device* device)
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
    if (!ImportSGM("assets/character.sgm", &g_data.testMesh, device)) {
        printf("failed to load test mesh from %s\n", "assets/character.gtmesh");
        return;
    }
    printf("Created test mesh\n");

    if (!ImportSGA("assets/character.sga", &g_data.testSkeleton)) {
        printf("failed to load test skeleton from %s\n", "assets/character.sga");
        return;
    }
    ResetLocalTransforms(&g_data.testSkeleton);
    printf("Created test skeleton\n");

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
        math::Make4x4FloatProjectionMatrixCMLH(frameData.projection, math::DegreesToRadians(60.0f), vp.Width, vp.Height, 0.1f, 1000.0f);
        float inverseCamera[16];
        math::Make4x4FloatLookAtMatrixCMLH(inverseCamera, math::Vec3(0.0f, 1.5f, -3.5f), math::Vec3(0.0f, 1.0f, 0.0f), math::Vec3(0.0f, 1.0f, 0.0f));
        math::Inverse4x4FloatMatrixCM(inverseCamera, frameData.camera);
        math::MultiplyMatricesCM(frameData.projection, frameData.camera, frameData.cameraProjection);

        static ObjectConstantData objectData;
        math::Make4x4FloatMatrixIdentity(objectData.transform);

        static float anim = 0.0f;
        anim += 0.1f;   // animate root

        static auto neck = GetBoneWithName(&g_data.testSkeleton, "Spine1");
        auto rot = math::Sin(anim) * 65.0f;
        math::Make4x4FloatRotationMatrixCMLH(g_data.testSkeleton.joints[neck].localTransform, math::Vec3(0.0f, 1.0f, 0.0f), math::DegreesToRadians(rot));

        static SkeletonConstantData skeletonData;
        for (auto i = 0; i < MAX_NUM_BONES; ++i) {
            ComputePose(&g_data.testSkeleton, &skeletonData);
            //math::Make4x4FloatMatrixIdentity(skeletonData.boneTransform[i]);
        }
        

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