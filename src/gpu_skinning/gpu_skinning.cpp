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
                { "TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(Vertex, tangent), D3D11_INPUT_PER_VERTEX_DATA, 0 },
                { "BLENDWEIGHT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(Vertex, blendWeights), D3D11_INPUT_PER_VERTEX_DATA, 0 },
                { "BLENDINDICES", 0, DXGI_FORMAT_R32G32B32A32_UINT, 0, offsetof(Vertex, blendIndices), D3D11_INPUT_PER_VERTEX_DATA, 0 },
            };
            auto res = device->CreateInputLayout(elementDesc, 6, shaderDesc->vertexShaderCode, shaderDesc->vertexShaderCodeSize, &layout);
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
    float   bindpose[16];   // local space bindpose
    float   invBindpose[16]; 
    float   globalTransform[16];
    float   localTransform[16];
    int     importId;   // @HACK
    int     parent;
};

struct Skeleton
{
    float bindpose[MAX_NUM_BONES][16];      // global space bindposes
    float invBindpose[MAX_NUM_BONES][16];   // global space inverse bindposes
    char* nameTable[MAX_NUM_BONES];   // contains human readable names of joints
    Joint joints[MAX_NUM_BONES];            // actual joints
    uint32_t numJoints;
};

uint32_t TransferNode(Skeleton* source, Skeleton* target, uint32_t& writeOffset, int nodeIdx)
{
    if (source->joints[nodeIdx].importId == -1) {
        for (uint32_t i = 0; i < target->numJoints; ++i) {
            if (target->joints[i].importId == nodeIdx) { return i; }
        }
    }
    if (source->joints[nodeIdx].parent != -1) {
        source->joints[nodeIdx].parent = TransferNode(source, target, writeOffset, source->joints[nodeIdx].parent);
    }
    target->numJoints++;
    target->joints[writeOffset] = source->joints[nodeIdx];
    target->nameTable[writeOffset] = source->nameTable[nodeIdx];
    memcpy(target->bindpose[writeOffset], source->bindpose[nodeIdx], sizeof(float) * 16);
    memcpy(target->invBindpose[writeOffset], source->invBindpose[nodeIdx], sizeof(float) * 16);
    writeOffset++;
    source->joints[nodeIdx].importId = -1;
    return writeOffset - 1;
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

void QuatToMatrix(const math::Vec4& quat, float* outMatrix)
{
    auto qx = quat.x;
    auto qy = quat.y;
    auto qz = quat.z;
    auto qw = quat.w;

    float temp[16];
    temp[0] = 1.0f - 2.0f*qy*qy - 2.0f*qz*qz;
    temp[1] = 2.0f*qx*qy - 2.0f*qz*qw;
    temp[2] = 2.0f*qx*qz + 2.0f*qy*qw;
    temp[3] = 0.0f;
    temp[4] = 2.0f*qx*qy + 2.0f*qz*qw;
    temp[5] = 1.0f - 2.0f*qx*qx - 2.0f*qz*qz;
    temp[6] = 2.0f*qy*qz - 2.0f*qx*qw;
    temp[7] = 0.0f;
    temp[8] = 2.0f*qx*qz - 2.0f*qy*qw;
    temp[9] = 2.0f*qy*qz + 2.0f*qx*qw;
    temp[10] = 1.0f - 2.0f*qx*qx - 2.0f*qy*qy;
    temp[11] = 0.0f;
    temp[12] = 0.0f;
    temp[13] = 0.0f;
    temp[14] = 0.0f;
    temp[15] = 1.0f;

    float rot[16];
    float conv[16];
    math::Make4x4FloatMatrixIdentity(conv);
    //math::Make4x4FloatRotationMatrixCMLH(conv, math::Vec3(1.0f, 0.0f, 0.0f), math::DegreesToRadians(90.0f));

    math::Make4x4FloatMatrixTranspose(temp, rot);
    math::MultiplyMatricesCM(conv, rot, outMatrix);
}


bool ImportSkeletonFromMemory(ByteStream& stream, Skeleton* outSkeleton)
{
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
        joint.importId = i;

        /*
        float spaceConversion[16] = {
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f,
        };

        float temp[16];
        stream.ReadBytes(temp, sizeof(float) * 16);
        math::MultiplyMatricesCM(spaceConversion, temp, tempSkeleton.bindpose[i]);
        */
        //stream.ReadBytes(tempSkeleton.bindpose[i], sizeof(float) * 16);
        auto pos = stream.Read<math::Vec3>();
        auto scale = stream.Read<math::Vec3>();  // ignore scale 
        auto rot = stream.Read<math::Vec4>();

        float rotMat[16];
        float transMat[16];
        float transScaled[16];
        float scaleMat[16];
        QuatToMatrix(rot, rotMat);
        math::Make4x4FloatTranslationMatrixCM(transMat, pos);   // 
        math::Make4x4FloatMatrixIdentity(scaleMat);
        scaleMat[0] = scale.x;
        scaleMat[5] = scale.y;
        scaleMat[10] = scale.z;
        
        math::MultiplyMatricesCM(transMat, scaleMat, transScaled);
        math::MultiplyMatricesCM(transScaled, rotMat, tempSkeleton.joints[i].bindpose);
       

        //math::Make4x4FloatTranslationMatrixCM(tempSkeleton.joints[i].bindpose, pos);
        auto isParent = stream.Read<uint8_t>() == 1;
        if (isParent) {
            joint.parent = -1;
        }
        else {
            joint.parent = 0;
        }
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
    // compute inverse bind transforms as well as global space bind and inverse bind transform for each bone
    for (int i = 0; i < (int)outSkeleton->numJoints; ++i) {
        auto& joint = outSkeleton->joints[i];
        if (joint.parent == -1) {
            math::Copy4x4FloatMatrixCM(joint.bindpose, outSkeleton->bindpose[i]);
        }
        else {
            math::MultiplyMatricesCM(outSkeleton->bindpose[joint.parent], joint.bindpose, outSkeleton->bindpose[i]);
        }
        math::Inverse4x4FloatMatrixCM(outSkeleton->bindpose[i], outSkeleton->invBindpose[i]);
        math::Inverse4x4FloatMatrixCM(joint.bindpose, joint.invBindpose);
    }

    return true;
}

bool ImportSkeletonFromSGA(const char* path, Skeleton* outSkeleton)
{
    uint32_t fileSize;
    ByteStream stream;
    stream.buffer = (char*)Win32LoadFileContents(path, &fileSize);
    stream.bufferSize = (size_t)(fileSize);
    stream.offset = 0;

    return ImportSkeletonFromMemory(stream, outSkeleton);
}


bool ImportGTSkeleton(const char* path, Skeleton* outSkeleton)
{
    uint32_t fileSize;
    ByteStream stream;
    stream.buffer = (char*)Win32LoadFileContents(path, &fileSize);
    stream.bufferSize = (size_t)(fileSize);
    stream.offset = 0;

    auto magicNumber = stream.Read<uint32_t>();
    assert(magicNumber == 0xdeadbeef);

    auto version = stream.Read<uint32_t>();
    assert(version == 1);

    Skeleton tempSkeleton;
    char* buf = new char[MAX_NUM_BONES * 512];
    memset(buf, 0x0, MAX_NUM_BONES * 512);
    uint32_t bufOffset = 0;

    tempSkeleton.numJoints = stream.Read<uint32_t>();
    for (uint32_t i = 0; i < tempSkeleton.numJoints; ++i) {
        uint32_t nameLen = stream.Read<uint32_t>();
        stream.ReadBytes(buf + bufOffset, nameLen);
        tempSkeleton.nameTable[i] = buf + bufOffset;
        bufOffset += nameLen + 1;

        float bindpose[16];
        stream.ReadBytes(bindpose, sizeof(float) * 16);
        math::Copy4x4FloatMatrixCM(bindpose, tempSkeleton.bindpose[i]);
        
        tempSkeleton.joints[i].importId = i;
        tempSkeleton.joints[i].parent = stream.Read<int32_t>();
    }
    
    SortSkeleton(&tempSkeleton, outSkeleton);
    for (int i = 0; i < (int)outSkeleton->numJoints; ++i) {
        assert(outSkeleton->joints[i].parent < i);
    }
    // compute inverse bind transforms as well as global space bind and inverse bind transform for each bone, make joint transforms local to parent
    for (int i = 0; i < (int)outSkeleton->numJoints; ++i) {
        auto& joint = outSkeleton->joints[i];
        if (joint.parent == -1) {
            math::Copy4x4FloatMatrixCM(outSkeleton->bindpose[i], joint.bindpose);
        }
        else {
            math::MultiplyMatricesCM(outSkeleton->invBindpose[joint.parent], outSkeleton->bindpose[i], joint.bindpose);
        }
        math::Inverse4x4FloatMatrixCM(outSkeleton->bindpose[i], outSkeleton->invBindpose[i]);
        math::Inverse4x4FloatMatrixCM(joint.bindpose, joint.invBindpose);
    }

    return true;
}
///

struct Keyframe
{
    float           timeStamp;
    math::Vec3      position;
    math::Vec4      rotation;

};

struct BoneTrack
{
    uint32_t    numKeyframes = 0;
    Keyframe*   keyframes = nullptr;
};

struct Animation
{
    char*           name  = "";
    uint32_t        numTracks = 0;
    BoneTrack       tracks[MAX_NUM_BONES];
    float           duration = 0.0f;
};

struct AnimationState
{
    Animation*  currentAnim;
    float       animationTime = 0.0f;
};

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

int GetBoneWithImportId(Skeleton* skeleton, int importId)
{
    for (uint32_t i = 0; i < skeleton->numJoints && i < MAX_NUM_BONES; ++i) {
        if (skeleton->joints[i].importId == importId) {
            return (int)i;
        }
    }
    return -1;
}
///
bool ImportAnimationFromSGA(const char* path, Skeleton* targetSkeleton, Animation* outAnimations, uint32_t* outNumAnimations)
{
    return false;
    //uint32_t fileSize;
    //ByteStream stream;
    //stream.buffer = (char*)Win32LoadFileContents(path, &fileSize);
    //stream.bufferSize = (size_t)(fileSize);
    //stream.offset = 0;

    //{
    //    Skeleton tempSkeleton;
    //    if (!ImportSkeletonFromMemory(stream, &tempSkeleton)) {
    //        assert(false);
    //        return false;
    //    }
    //}

    //*outNumAnimations = (uint32_t)stream.Read<uint16_t>();
    //if (outAnimations != nullptr) {
    //    for (uint32_t i = 0; i < *outNumAnimations; ++i) {
    //        Animation& anim = outAnimations[i];
    //        auto nameLen = stream.Read<uint16_t>();
    //        anim.name = (char*)malloc(nameLen);
    //        stream.ReadBytes(anim.name, nameLen);
    //        auto numAffectedBones = stream.Read<uint16_t>();
    //       
    //        float biggestTimestamp = 0.0f;
    //        for (uint16_t j = 0; j < numAffectedBones; ++j) {
    //            auto importId = stream.Read<uint16_t>();
    //            auto id = GetBoneWithImportId(targetSkeleton, importId);
    //            auto& track = anim.tracks[id];
    //            track.numKeyframes = stream.Read<uint32_t>();
    //            track.keyframes = new KeyFrame[track.numKeyframes];
    //            for (uint32_t k = 0; k < track.numKeyframes; ++k) {
    //                auto& frame = track.keyframes[k];
    //                frame.timeStamp = stream.Read<float>();
    //                if (frame.timeStamp > biggestTimestamp) { biggestTimestamp = frame.timeStamp; }
    //                frame.jointTransform.position = stream.Read<math::Vec3>();  
    //                frame.jointTransform.scale = stream.Read<math::Vec3>();     // 
    //                frame.jointTransform.rotation = stream.Read<math::Vec4>();  // rotation as a 4-component quaternion
    //            }
    //        }
    //        anim.duration = biggestTimestamp;
    //    }
    //}
    //return true;
}


//
bool ImportGTAnimation(const char* path, Skeleton* targetSkeleton, Animation* outAnimation)
{
    uint32_t fileSize;
    ByteStream stream;
    stream.buffer = (char*)Win32LoadFileContents(path, &fileSize);
    stream.bufferSize = (size_t)(fileSize);
    stream.offset = 0;

    auto magicNumber = stream.Read<uint32_t>();
    assert(magicNumber == 0xdeadbeef);

    auto version = stream.Read<uint32_t>();
    assert(version == 1);


    Animation& anim = *outAnimation;
    auto nameLen = stream.Read<uint32_t>();
    anim.name = (char*)malloc(nameLen + 1);
    memset(anim.name, 0x0, nameLen + 1);
    stream.ReadBytes(anim.name, nameLen);
    float biggestTimestamp = 0.0f;
    
    anim.numTracks = stream.Read<uint32_t>();
    for (uint32_t j = 0; j < anim.numTracks; ++j) {
        auto importId = stream.Read<uint32_t>();
        auto id = GetBoneWithImportId(targetSkeleton, importId);
        {   // translation
            auto& track = anim.tracks[id];
            track.numKeyframes = stream.Read<uint32_t>();
            //assert(track.numKeyframes != 0);
            track.keyframes = new Keyframe[track.numKeyframes];
            //printf("Bone: %s\n", targetSkeleton->nameTable[id]);
            for (uint32_t k = 0; k < track.numKeyframes; ++k) {
                auto& frame = track.keyframes[k];
                frame.timeStamp = stream.Read<float>();
                if (frame.timeStamp > biggestTimestamp) { biggestTimestamp = frame.timeStamp; }
                frame.position = stream.Read<math::Vec3>();
                frame.rotation = stream.Read<math::Vec4>();
            }
        }
    }
    
    anim.duration = biggestTimestamp;
     
    return true;
}

///
/**
    ASSUMPTIONS/RULES:
    -> bindposes describe the resting pose of a bone in model (not bone) space, i.e. they have hierarchy applied
    -> a vertex is transformed as follows:
        skinnedVertex = modelTransform * modelSpaceBoneTransform * invBindPose * vertex
    where
        modelSpaceBoneTransform = parentGlobalTransform * boneLocalTransform
*/

void ResetLocalTransforms(Skeleton* skeleton)
{
    for (uint32_t i = 0; i < skeleton->numJoints; ++i) {
        math::Copy4x4FloatMatrixCM(skeleton->joints[i].bindpose, skeleton->joints[i].localTransform);
    }
}


void ComputeLocalPoses(Skeleton* skeleton, AnimationState* animState, bool didLoop)
{
    auto& anim = *animState->currentAnim;
    auto time = animState->animationTime;
    // @NOTE we assume that the skeleton is sorted so parents are always evaluated before their children
    // and also local transforms have been reset at least once so there's no BS in them and we can do this in a single loop
    for (uint32_t i = 0; i < skeleton->numJoints; ++i) {
        
        math::Vec3 translation;
        math::Vec4 rotation;

        // translation
        if (anim.tracks[i].numKeyframes != 0)
        {
            Keyframe* prevKeyframe = anim.tracks[i].keyframes;
            Keyframe* nextKeyframe = anim.tracks[i].keyframes;
            for (uint32_t k = 0; k < anim.tracks[i].numKeyframes; ++k) {
                nextKeyframe = anim.tracks[i].keyframes + k;
                if (nextKeyframe->timeStamp > time) {
                    break;
                }
                prevKeyframe = anim.tracks[i].keyframes + k;
            }
            if (prevKeyframe != nextKeyframe) {
                float alpha = (time - prevKeyframe->timeStamp) / (nextKeyframe->timeStamp - prevKeyframe->timeStamp);
                translation = math::Lerp(prevKeyframe->position, nextKeyframe->position, alpha);
                rotation = math::Slerp(prevKeyframe->rotation, nextKeyframe->rotation, alpha);
            }
            else {
                translation = prevKeyframe->position;
                rotation = prevKeyframe->rotation;
            }
            //translation = prevKeyframe->value;
            //translation = math::Vec3();
        }

        // @NOTE
        //if (didLoop) {
        //    // the previous keyframe will be the last keyframe in the track
        //    prevKeyframe = &anim.tracks[i].keyframes[anim.tracks[i].numKeyframes - 1]; // @NOTE this is a bit hacky tho
        //    alpha = time;
        //}

        // 
        float rot[16];
        float transl[16];

        QuatToMatrix(rotation, rot);
        math::Make4x4FloatTranslationMatrixCM(transl, translation);
      
        float localPose[16];
        math::MultiplyMatricesCM(transl, rot, localPose);

        if (i != 0) {   // @HACK what the fuck? why is this only necessary for non-root? what is going on
            float transl[16];   // @NOTE THIS IS SUPER WEIRD 
            math::Make4x4FloatTranslationMatrixCM(transl, math::Get4x4FloatMatrixColumnCM(skeleton->joints[i].bindpose, 3).xyz);
            math::MultiplyMatricesCM(transl, localPose, skeleton->joints[i].localTransform);
            //math::Copy4x4FloatMatrixCM(localPose, skeleton->joints[i].localTransform);
        }
        else {
            math::Copy4x4FloatMatrixCM(localPose, skeleton->joints[i].localTransform);
        }

    }
    //
}

void TransformHierarchy(Skeleton* skeleton)
{
    for (auto i = 0u; i < skeleton->numJoints; ++i) {
        if (skeleton->joints[i].parent != -1) {
            auto& parent = skeleton->joints[skeleton->joints[i].parent];
            math::MultiplyMatricesCM(parent.globalTransform, skeleton->joints[i].localTransform, skeleton->joints[i].globalTransform);
        }
        else {
            math::Copy4x4FloatMatrixCM(skeleton->joints[i].localTransform, skeleton->joints[i].globalTransform);
        }
    }
}

void GetSkinningTransforms(Skeleton* skeleton, SkeletonConstantData* outBuffer)
{
    for (auto i = 0u; i < MAX_NUM_BONES && i < skeleton->numJoints; ++i) 
    {
        auto idx = skeleton->joints[i].importId;
        math::MultiplyMatricesCM(skeleton->joints[i].globalTransform, skeleton->invBindpose[i], outBuffer->boneTransform[idx]);

        //math::Copy4x4FloatMatrixCM(skeleton->joints[i].globalTransform, outBuffer->boneTransform[idx]);
       // math::Make4x4FloatMatrixIdentity(outBuffer->boneTransform[idx]);
    }
}

///
struct AppData
{   
    ShaderDesc shaderDesc;

    Skeleton    testSkeleton;
    Mesh        testMesh;
    Shader      shader;

    uint32_t        currentAnim = 0;
    Animation       testAnim[128];
    AnimationState  animState;

    ID3D11Buffer* frameConstantBuffer;
    ID3D11Buffer* objectConstantBuffer;
    ID3D11Buffer* skeletonConstantBuffer;

    FrameConstantData frameData;
    ObjectConstantData objectData;
    SkeletonConstantData skeletonData;

} g_data;


bool ImportGTMesh(const char* path, Mesh* outMesh, ID3D11Device* device)
{
    uint32_t fileSize;
    ByteStream stream;
    stream.buffer = (char*)Win32LoadFileContents(path, &fileSize);
    stream.bufferSize = (size_t)(fileSize);
    stream.offset = 0;

    auto magicNumber = stream.Read<uint32_t>();
    assert(magicNumber == 0xdeadbeef);

    auto version = stream.Read<uint32_t>();
    assert(version == 1);

    MeshDesc meshDesc;
    meshDesc.numVertices = stream.Read<uint32_t>();
    meshDesc.vertices = new Vertex[meshDesc.numVertices];
    for (uint32_t i = 0; i < meshDesc.numVertices; ++i) {
        auto& vert = meshDesc.vertices[i];

        vert.position = stream.Read<math::Vec3>();
        vert.normal = stream.Read<math::Vec3>();
        stream.ReadBytes(vert.uv, sizeof(float) * 2);
        stream.ReadBytes(nullptr, sizeof(float) * 2);   // @NOTE ignore second UV set
        vert.tangent = stream.Read<math::Vec4>();
        stream.ReadBytes(vert.blendWeights, sizeof(float) * 4);
        stream.ReadBytes(vert.blendIndices, sizeof(uint32_t) * 4);
    }

    auto indexSize = stream.Read<uint32_t>();
    auto numSubmeshes = stream.Read<uint32_t>();
    uint32_t totalNumIndices = 0;

    MeshDesc* submeshDesc = new MeshDesc[numSubmeshes];
    for (uint32_t i = 0; i < numSubmeshes; ++i) {
        auto& desc = submeshDesc[i];
        desc.numIndices = stream.Read<uint32_t>();
        desc.indices = new IndexType[desc.numIndices];
        for (uint32_t idx = 0; idx < desc.numIndices; ++idx) {
            if (indexSize == 2) {
                desc.indices[idx] = (IndexType)stream.Read<uint16_t>();
            }
            else {
                desc.indices[idx] = (IndexType)stream.Read<uint32_t>();
            }
        }
        totalNumIndices += desc.numIndices;
    }
    meshDesc.numIndices = totalNumIndices;
    meshDesc.indices = new IndexType[totalNumIndices];
    IndexType* writePtr = meshDesc.indices;
    for (uint32_t i = 0; i < numSubmeshes; ++i) {
        memcpy(writePtr, submeshDesc[i].indices, submeshDesc[i].numIndices * sizeof(IndexType));
        delete[] submeshDesc[i].indices;
        writePtr += submeshDesc[i].numIndices;
    }
    delete[] submeshDesc;

    auto success = CreateMesh(device, &meshDesc, outMesh);
    delete[] meshDesc.vertices;
    delete[] meshDesc.indices;
    return success;
}


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

    //
    for (auto i = 0u; i < meshDesc.numVertices; ++i) {

        auto weightSum = 0.f;
        for (auto j = 0; j < 4; ++j) {
            weightSum += meshDesc.vertices[i].blendWeights[j];
        }
        assert(math::Abs(1.0f - weightSum) <= 0.00001f);
    }

    auto res = CreateMesh(device, &meshDesc, outMesh);
    delete[] submeshDesc;
    return res;

    return true;
}


const char* animFiles[] = { "assets/knight_idle.gtanimclip", "assets/knight_walk.gtanimclip", 
                            "assets/knight_run_default.gtanimclip", "assets/knight_run_fast.gtanimclip", 
                            "assets/knight_run_slide.gtanimclip", "assets/knight_draw.gtanimclip",
                            "assets/knight_onehand_combo.gtanimclip", "assets/knight_sheathe.gtanimclip", 
                            "assets/knight_draw_twohand.gtanimclip",
                            "assets/knight_twohand_combo.gtanimclip", "assets/knight_sheathe_twohanded.gtanimclip" };
const int numAnims = ARRAYSIZE(animFiles);
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
    /*if (!ImportSGM("assets/doug.sgm", &g_data.testMesh, device)) {
       printf("failed to load test mesh from %s\n", "assets/character.gtmesh");
        return;
    }*/
    if(!ImportGTMesh("assets/knight.gtmesh", &g_data.testMesh, device)) {
        printf("failed to load test mesh from %s\n", "assets/character.gtmesh");
        return;
    }
    printf("Created test mesh\n");

    if (!ImportGTSkeleton("assets/knight.gtskel", &g_data.testSkeleton)) {
        printf("failed to load test skeleton from %s\n", "assets/character.sga");
        return;
    }
    ResetLocalTransforms(&g_data.testSkeleton);
    printf("Created test skeleton\n");

    uint32_t numAnimations = 0;
    uint32_t currentImportAnimation = 0;
    
    for (uint32_t i = 0; i < numAnims; ++i) {
        if (!ImportGTAnimation(animFiles[i], &g_data.testSkeleton, &g_data.testAnim[currentImportAnimation])) {
            printf("failed to load animation from %s\n", animFiles[i]);
            return;

        }
        printf("loaded anim: %s\n", g_data.testAnim[currentImportAnimation].name);
        currentImportAnimation++;
    }
    g_data.animState.animationTime = 0.0f;
    g_data.currentAnim = 0;
    g_data.animState.currentAnim = &g_data.testAnim[0];

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
    RECT rect;
    GetClientRect(hWnd, &rect);

    float width = float(rect.right - rect.left);
    float height = float(rect.bottom - rect.top);
    math::Make4x4FloatProjectionMatrixCMLH(g_data.frameData.projection, math::DegreesToRadians(60.0f), width, height, 0.1f, 1000.0f);
    float inverseCamera[16];

    static float zoom = 1.0f;
    static float camHeight = 0.0f;
    static float xRot = 0.0f;
    static float yRot = 0.0f;
    ImGui::DragFloat("Zoom", &zoom, 0.01f);
    ImGui::DragFloat("X Rot", &xRot, 0.1f);
    ImGui::DragFloat("Y Rot", &yRot, 0.1f);
    ImGui::DragFloat("Z Pos", &camHeight, 0.01f);

    float rot[16];
    float xRotMat[16];
    float yRotMat[16];

    math::Make4x4FloatRotationMatrixCMLH(xRotMat, math::Vec3(1.0f, 0.0f, 0.0f), math::DegreesToRadians(xRot));
    math::Make4x4FloatRotationMatrixCMLH(yRotMat, math::Vec3(0.0f, 1.0f, 0.0f), math::DegreesToRadians(yRot));

    math::MultiplyMatricesCM(yRotMat, xRotMat, rot);
    auto camPos = math::TransformPositionCM(math::Vec3(0.0f, camHeight, -zoom), rot);
    
    auto root = 0;

    static math::Vec3 offset(2.0f, 4.0f, -7.5f);

    ImGui::DragFloat("Offset X", &offset.x, 0.01f);
    ImGui::DragFloat("Offset Y", &offset.y, 0.01f);
    ImGui::DragFloat("Offset Z", &offset.z, 0.01f);

    math::Vec3 playerPos = math::Get4x4FloatMatrixColumnCM(g_data.objectData.transform, 3).xyz + math::Vec3(0.0f, 0.8f, 0.0f);
    camPos = playerPos + offset* zoom;
    math::Vec3 focus = camPos + math::Vec3(0.0f, 0.0f, 500.0f);
    math::Make4x4FloatLookAtMatrixCMLH(inverseCamera, camPos, focus, math::Vec3(0.0f, 1.0f, 0.0f));
    math::Inverse4x4FloatMatrixCM(inverseCamera, g_data.frameData.camera);
    math::MultiplyMatricesCM(g_data.frameData.projection, g_data.frameData.camera, g_data.frameData.cameraProjection);

    math::Make4x4FloatMatrixIdentity(g_data.objectData.transform);
    ///
    static float animSpeedMod = 1.0f;
    static bool animate = true;
    static bool rootMotion = false;
    static bool sequence = true;
    static bool loop = true;
    auto speed = (1.0f) * ImGui::GetIO().DeltaTime;
    bool didLoop = false;   // for root motion 
    static bool didSwitchAnimation = false;
    g_data.animState.animationTime += animate ? animSpeedMod * speed : 0.0f;
    if (g_data.animState.animationTime > g_data.animState.currentAnim->duration && loop) {
        g_data.animState.animationTime -= g_data.animState.currentAnim->duration;
        didLoop = true;
    }
    if (g_data.animState.animationTime < 0.0f && loop) {
        g_data.animState.animationTime += g_data.animState.currentAnim->duration;
        didLoop = true;
    }
    if (didLoop && sequence) {
        g_data.currentAnim = (g_data.currentAnim + 1) % numAnims;
        g_data.animState.currentAnim = &g_data.testAnim[g_data.currentAnim];
        didSwitchAnimation = true;
    }

    //ImGui::ShowTestWindow();

    static bool tPose = false;
    static bool showSkeleton = true;
    static bool transformHierarchy = true;
    ResetLocalTransforms(&g_data.testSkeleton);
    animate = animate && !tPose;
    if (!tPose) {
        ComputeLocalPoses(&g_data.testSkeleton, &g_data.animState, didLoop);
    }

    {   // override local poses here
        
        static math::Vec3 objectPosition;
        if(rootMotion)
        {   // root motion
            auto& rootJoint = g_data.testSkeleton.joints[0];
            auto numKeyframes = g_data.animState.currentAnim->tracks[0].numKeyframes;
            auto& firstKeyframe = g_data.animState.currentAnim->tracks[0].keyframes[0];
            static auto initialPosition = firstKeyframe.position;
            static auto sourcePosition = initialPosition;
            if (didSwitchAnimation) {
                initialPosition = firstKeyframe.position;
                didSwitchAnimation = false;
            }
            if (didLoop) {
                sourcePosition = initialPosition;
            }
            auto currentPosition = math::Get4x4FloatMatrixColumnCM(rootJoint.localTransform, 3).xyz;
            auto dist = currentPosition - sourcePosition;
            math::SetTranslation4x4FloatMatrixCM(rootJoint.localTransform, initialPosition);
            sourcePosition = currentPosition;
            //dist.x = 0.0f;
            //dist.z = 0.0f;
            objectPosition += dist;
        } else {
            objectPosition = math::Vec3();
        }
        math::Make4x4FloatTranslationMatrixCM(g_data.objectData.transform, objectPosition);
        ImGui::Text("x = %f", objectPosition.x);
        ImGui::Text("y = %f", objectPosition.y);
        ImGui::Text("z = %f", objectPosition.z);

    }

    //
    if (transformHierarchy) {
        TransformHierarchy(&g_data.testSkeleton);
    }
    else {
        for (auto i = 0u; i < g_data.testSkeleton.numJoints; ++i) {
            math::Copy4x4FloatMatrixCM(g_data.testSkeleton.joints[i].localTransform, g_data.testSkeleton.joints[i].globalTransform);
        }
    }

    GetSkinningTransforms(&g_data.testSkeleton, &g_data.skeletonData);
    ///
    //
    static int selectedJoint = -1;
    if (selectedJoint != -1) {
        ImGui::PlotLines("X Pos Curve", [](void* data, int idx) -> float {
            return static_cast<Animation*>(data)->tracks[0].keyframes[idx].position.x;
        }, g_data.animState.currentAnim, g_data.animState.currentAnim->tracks[selectedJoint].numKeyframes, 0, nullptr, -2.0f, 2.0f);
        ImGui::PlotLines("Y Pos Curve", [](void* data, int idx) -> float {
            return static_cast<Animation*>(data)->tracks[0].keyframes[idx].position.y;
        }, g_data.animState.currentAnim, g_data.animState.currentAnim->tracks[selectedJoint].numKeyframes, 0, nullptr, -2.0f, 2.0f);
        ImGui::PlotLines("Z Pos Curve", [](void* data, int idx) -> float {
            return static_cast<Animation*>(data)->tracks[0].keyframes[idx].position.z;
        }, g_data.animState.currentAnim, g_data.animState.currentAnim->tracks[selectedJoint].numKeyframes, 0, nullptr, -2.0f, 2.0f);
        ImGui::Text("Parent: %i", g_data.testSkeleton.joints[selectedJoint].parent);

        ImGui::PlotLines("X Rot Curve", [](void* data, int idx) -> float {
            return static_cast<Animation*>(data)->tracks[0].keyframes[idx].rotation.x;
        }, g_data.animState.currentAnim, g_data.animState.currentAnim->tracks[selectedJoint].numKeyframes, 0, nullptr, -1.0f, 1.0f);
        ImGui::PlotLines("Y Rot Curve", [](void* data, int idx) -> float {
            return static_cast<Animation*>(data)->tracks[0].keyframes[idx].rotation.y;
        }, g_data.animState.currentAnim, g_data.animState.currentAnim->tracks[selectedJoint].numKeyframes, 0, nullptr, -1.0f, 1.0f);
        ImGui::PlotLines("Z Rot Curve", [](void* data, int idx) -> float {
            return static_cast<Animation*>(data)->tracks[0].keyframes[idx].rotation.z;
        }, g_data.animState.currentAnim, g_data.animState.currentAnim->tracks[selectedJoint].numKeyframes, 0, nullptr, -1.0f, 1.0f);
        ImGui::PlotLines("w Rot Curve", [](void* data, int idx) -> float {
            return static_cast<Animation*>(data)->tracks[0].keyframes[idx].rotation.w;
        }, g_data.animState.currentAnim, g_data.animState.currentAnim->tracks[selectedJoint].numKeyframes, 0, nullptr, -1.0f, 1.0f);
        ImGui::Text("Parent: %i", g_data.testSkeleton.joints[selectedJoint].parent);
    }
    if (ImGui::Begin("Skeleton")) {
        ImGui::Checkbox("T-Pose", &tPose);
        ImGui::Checkbox("Show Skeleton", &showSkeleton);
        ImGui::Checkbox("Transform Hierarchy", &transformHierarchy);
        ImGui::Checkbox("Animate", &animate);
        ImGui::Checkbox("Root Motion", &rootMotion);
        ImGui::Checkbox("Loop", &loop);
        ImGui::Checkbox("Sequence", &sequence);
        ImGui::SliderFloat("Playback Speed Modifier", &animSpeedMod, -1.0f, 1.0f);

        if (ImGui::BeginCombo("Animation Clip", g_data.testAnim[g_data.currentAnim].name)) {
            for (uint32_t i = 0; i < numAnims; ++i) {
                ImGui::PushID(i);
                if (ImGui::Selectable(g_data.testAnim[i].name, i == g_data.currentAnim)) {
                    g_data.currentAnim = i;
                    g_data.animState.currentAnim = &g_data.testAnim[g_data.currentAnim];
                    didSwitchAnimation = true;
                }
                ImGui::PopID();
            }
            ImGui::EndCombo();
        }

        static bool* isDisplayed = nullptr;
        if (isDisplayed == nullptr) {
            isDisplayed = new bool[g_data.testSkeleton.numJoints];
        }
        for (auto i = 0u; i < g_data.testSkeleton.numJoints; ++i) { isDisplayed[i] = false; }
        for (auto i = 0u; i < g_data.testSkeleton.numJoints; ++i) {
            ImGui::PushID(i);
            if (ImGui::Selectable(g_data.testSkeleton.nameTable[i], selectedJoint == i)) {
                selectedJoint = i;
            }
            ImGui::PopID();
        }
    } ImGui::End();
   

    auto canvasFlags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings;
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(width, height));
    if (ImGui::Begin("#canvas", nullptr, ImVec2(), 0.0f, canvasFlags)) {

        if (ImGui::IsWindowHovered() && ImGui::IsMouseDragging(1)) {
            xRot -= ImGui::GetMouseDragDelta(1).y * 0.1f;
            yRot -= ImGui::GetMouseDragDelta(1).x * 0.1f;

            ImGui::ResetMouseDragDelta(1);
        }
        


        auto WorldToScreen = [&](const math::Vec3& pos) -> math::Vec3 {
            auto clipPos = math::TransformPositionCM(math::Vec4(pos, 1.0f), g_data.frameData.cameraProjection);
            clipPos.xyz /= clipPos.w;
            auto uv = (clipPos.xyz * 0.5f + 0.5f);
            uv.y = 1.0f - uv.y;
            return uv * math::Vec3(width, height, 1.0f);
        };
        auto drawList = ImGui::GetWindowDrawList();

        auto u = WorldToScreen(math::Vec3(1.0f, 0.0f, 0.0f));
        auto v = WorldToScreen(math::Vec3(0.0f, 1.0f, 0.0f));
        auto w = WorldToScreen(math::Vec3(0.0f, 0.0f, 1.0f));
        auto o = WorldToScreen(math::Vec3());
        
        drawList->AddLine(ImVec2(o.x, o.y), ImVec2(u.x, u.y), ImColor(1.0f, 0.0f, 0.0f), 4.0f);
        drawList->AddLine(ImVec2(o.x, o.y), ImVec2(v.x, v.y), ImColor(0.0f, 0.0f, 1.0f), 4.0f);
        drawList->AddLine(ImVec2(o.x, o.y), ImVec2(w.x, w.y), ImColor(0.0f, 1.0f, 0.0f), 4.0f);

        for (auto i = 0u; showSkeleton && i < g_data.testSkeleton.numJoints; ++i) {

            auto boneHead = math::Get4x4FloatMatrixColumnCM(g_data.testSkeleton.joints[i].globalTransform, 3).xyz;
            boneHead = math::TransformPositionCM(boneHead, g_data.objectData.transform);
            auto screenPos = WorldToScreen(boneHead);

            auto boneU = math::TransformPositionCM(math::Vec3(1.0f, 0.0f, 0.0f) * 0.1f, g_data.testSkeleton.joints[i].globalTransform);
            auto boneV = math::TransformPositionCM(math::Vec3(0.0f, 1.0f, 0.0f) * 0.1f, g_data.testSkeleton.joints[i].globalTransform);
            auto boneW = math::TransformPositionCM(math::Vec3(0.0f, 0.0f, 1.0f) * 0.1f, g_data.testSkeleton.joints[i].globalTransform);

            boneU = math::TransformPositionCM(boneU, g_data.objectData.transform);
            boneV = math::TransformPositionCM(boneV, g_data.objectData.transform);
            boneW = math::TransformPositionCM(boneW, g_data.objectData.transform);


            auto boneUPos = WorldToScreen(boneU);
            auto boneVPos = WorldToScreen(boneV);
            auto boneWPos = WorldToScreen(boneW);

            ImVec2 labelPos(screenPos.x, screenPos.y);

            drawList->AddCircleFilled(labelPos, 5.0f, selectedJoint == i ? ImColor(0.0f, 1.0f, 0.0f) : ImColor(1.0f, 0.0f, 0.0f), 48);
            drawList->AddLine(labelPos, ImVec2(boneUPos.x, boneUPos.y), ImColor(1.0f, 0.0f, 0.0f));
            drawList->AddLine(labelPos, ImVec2(boneVPos.x, boneVPos.y), ImColor(0.0f, 0.0f, 1.0f));
            drawList->AddLine(labelPos, ImVec2(boneWPos.x, boneWPos.y), ImColor(0.0f, 1.0f, 0.0f));

            
            ImGui::SetCursorScreenPos(ImVec2(labelPos.x - 2.5f, labelPos.y - 2.5f));
            ImGui::InvisibleButton(g_data.testSkeleton.nameTable[i], ImVec2(8.5f, 8.5f));
            if (ImGui::IsItemHoveredRect() || selectedJoint == i) {
                drawList->AddText(labelPos, ImColor(1.0f, 1.0f, 1.0f), g_data.testSkeleton.nameTable[i]);
            }
            
            auto parent = g_data.testSkeleton.joints[i].parent;
            if (parent != -1) {
                auto parentPos = math::Get4x4FloatMatrixColumnCM(g_data.testSkeleton.joints[parent].globalTransform, 3).xyz;
                parentPos = math::TransformPositionCM(parentPos, g_data.objectData.transform);
                auto parentScreenPos = WorldToScreen(parentPos);

                drawList->AddLine(ImVec2(parentScreenPos.x, parentScreenPos.y), ImVec2(screenPos.x, screenPos.y), ImColor(0.0f, 0.0f, 1.0f));
            }
        }
    } ImGui::End();
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
       
        ///
        {   // frame data
            D3D11_MAPPED_SUBRESOURCE resource;
            auto res = deviceContext->Map(g_data.frameConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource);
            if (!SUCCEEDED(res)) { printf("Failed to map frame constant buffer!\n"); }
            memcpy(resource.pData, &g_data.frameData, sizeof(FrameConstantData));
            deviceContext->Unmap(g_data.frameConstantBuffer, 0);
        }
        {   // object data
            D3D11_MAPPED_SUBRESOURCE resource;
            auto res = deviceContext->Map(g_data.objectConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource);
            if (!SUCCEEDED(res)) { printf("Failed to map object constant buffer!\n"); }
            memcpy(resource.pData, &g_data.objectData, sizeof(ObjectConstantData));
            deviceContext->Unmap(g_data.objectConstantBuffer, 0);
        }
        {   // skeleton data
            D3D11_MAPPED_SUBRESOURCE resource;
            auto res = deviceContext->Map(g_data.skeletonConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource);
            if (!SUCCEEDED(res)) { printf("Failed to map skeleton constant buffer!\n"); }
            memcpy(resource.pData, &g_data.skeletonData, sizeof(SkeletonConstantData));
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