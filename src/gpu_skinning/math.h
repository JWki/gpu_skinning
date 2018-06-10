#pragma once

#include <stdint.h>
#include <math.h>
#include <memory.h>

#undef near
#undef far
//
#define PI 3.1415926f
///
#define VEC3_BINARY_COMP_OP(OP) \
static Vec3 operator OP (const Vec3& a, const Vec3& b) \
{   \
    return Vec3(a.x OP b.x, a.y OP b.y, a.z OP b.z); \
}
#define VEC3_BINARY_SCALAR_OP_LHS(OP) \
static Vec3 operator OP (const Vec3& vec, float v) \
{   \
    return Vec3(vec.x OP v, vec.y OP v, vec.z OP v); \
}
#define VEC3_BINARY_SCALAR_OP_RHS(OP) \
static Vec3 operator OP (float v, const Vec3& vec) \
{   \
    return Vec3(vec.x OP v, vec.y OP v, vec.z OP v); \
}
///
#define VEC4_BINARY_COMP_OP(OP) \
static Vec4 operator OP (const Vec4& a, const Vec4& b) \
{   \
    return Vec4(a.x OP b.x, a.y OP b.y, a.z OP b.z, a.w OP b.w); \
}
#define VEC4_BINARY_SCALAR_OP_LHS(OP) \
static Vec4 operator OP (const Vec4& vec, float v) \
{   \
    return Vec4(vec.x OP v, vec.y OP v, vec.z OP v, vec.w OP v); \
}
#define VEC4_BINARY_SCALAR_OP_RHS(OP) \
static Vec4 operator OP (float v, const Vec4& vec) \
{   \
    return Vec4(vec.x OP v, vec.y OP v, vec.z OP v, vec.w OP v); \
}

///
namespace math
{
    static float RadiansToDegrees(float rad)
    {
        return rad * (180.0f / PI);
    }

    static float DegreesToRadians(float deg)
    {
        return deg * (PI / 180.0f);
    }

    static float Sqrt(float n)
    {
        return sqrtf(n);
    }
    static double Sqrt(double n)
    {
        return sqrt(n);
    }

    static float Sin(float n)
    {
        return sinf(n);
    }

    static float Cos(float n)
    {
        return cosf(n);
    }

    static double Sin(double n)
    {
        return sin(n);
    }

    static double Cos(double n)
    {
        return cos(n);
    }

    template <class TValue>
    static TValue Lerp(const TValue& a, const TValue& b, float alpha)
    {
        return a * (1.0f - alpha) + b * alpha;
    }

  

    template <class TValue>
    static TValue Min(TValue a, TValue b)
    {
        return a < b ? a : b;
    }

    template <class TValue>
    static TValue Max(TValue a, TValue b)
    {
        return a > b ? a : b;
    }

    template <class TValue>
    static TValue Clamp(TValue val, TValue min, TValue max)
    {
        return Min(max, Max(val, min));
    }

    template <class TValue>
    static TValue Abs(TValue val)
    {
        return val > 0.0f ? val : -val;
    }

    ///
    struct Vec3
    {
        union {
            struct { float x, y, z; };
            struct { float r, g, b; };
            float elements[3];
        };

        Vec3() : x(0.0f), y(0.0f), z(0.0f) {};
        Vec3(float x_, float y_, float z_)
            : x(x_), y(y_), z(z_) {}

        inline Vec3& operator += (const Vec3& other) { x += other.x; y += other.y; z += other.z; return *this; }
        inline Vec3& operator -= (const Vec3& other) { x -= other.x; y -= other.y; z -= other.z; return *this; }
        inline Vec3& operator *= (const Vec3& other) { x *= other.x; y *= other.y; z *= other.z; return *this; }
        inline Vec3& operator /= (const Vec3& other) { x /= other.x; y /= other.y; z /= other.z; return *this; }

        inline Vec3& operator += (float v) { x += v; y += v; z += v; return *this; }
        inline Vec3& operator -= (float v) { x -= v; y -= v; z -= v; return *this; }
        inline Vec3& operator *= (float v) { x *= v; y *= v; z *= v; return *this; }
        inline Vec3& operator /= (float v) { x /= v; y /= v; z /= v; return *this; }

        inline Vec3 operator - () const { return Vec3(-x, -y, -z); }
        inline float operator [](int i) const { return elements[i]; }
        inline float& operator [](int i) { return elements[i]; }
    };
    //
    struct Vec4
    {
        union {
            struct { float x, y, z, w; };
            struct { float r, g, b, a; };
            float elements[4];
            Vec3 xyz;
        };

        Vec4() : x(0.0f), y(0.0f), z(0.0f), w(0.0f) {};
        Vec4(const Vec3& xyz_, float w_) : xyz(xyz_), w(w_) {}
        Vec4(float x_, float y_, float z_, float w_)
            : x(x_), y(y_), z(z_), w(w_) {}

        inline Vec4& operator += (const Vec4& other) { x += other.x; y += other.y; z += other.z; w += other.w; return *this; }
        inline Vec4& operator -= (const Vec4& other) { x -= other.x; y -= other.y; z -= other.z; w -= other.w; return *this; }
        inline Vec4& operator *= (const Vec4& other) { x *= other.x; y *= other.y; z *= other.z; w *= other.w; return *this; }
        inline Vec4& operator /= (const Vec4& other) { x /= other.x; y /= other.y; z /= other.z; w /= other.w; return *this; }

        inline Vec4& operator += (float v) { x += v; y += v; z += v; w += v; return *this; }
        inline Vec4& operator -= (float v) { x -= v; y -= v; z -= v; w -= v; return *this; }
        inline Vec4& operator *= (float v) { x *= v; y *= v; z *= v; w *= v; return *this; }
        inline Vec4& operator /= (float v) { x /= v; y /= v; z /= v; w /= v; return *this; }

        inline Vec4 operator - () const { return Vec4(-x, -y, -z, -w); }
        inline float operator [](int i) const { return elements[i]; }
        inline float& operator [](int i) { return elements[i]; }
    };
    /// 
    VEC3_BINARY_COMP_OP(*);
    VEC3_BINARY_COMP_OP(/ );
    VEC3_BINARY_COMP_OP(+);
    VEC3_BINARY_COMP_OP(-);
    VEC3_BINARY_SCALAR_OP_LHS(*);
    VEC3_BINARY_SCALAR_OP_LHS(/);
    VEC3_BINARY_SCALAR_OP_LHS(+);
    VEC3_BINARY_SCALAR_OP_LHS(-);

    VEC3_BINARY_SCALAR_OP_RHS(*);
    VEC3_BINARY_SCALAR_OP_RHS(/);
    VEC3_BINARY_SCALAR_OP_RHS(+);
    VEC3_BINARY_SCALAR_OP_RHS(-);
    ///
    VEC4_BINARY_COMP_OP(*);
    VEC4_BINARY_COMP_OP(/ );
    VEC4_BINARY_COMP_OP(+);
    VEC4_BINARY_COMP_OP(-);
    VEC4_BINARY_SCALAR_OP_LHS(*);
    VEC4_BINARY_SCALAR_OP_LHS(/);
    VEC4_BINARY_SCALAR_OP_LHS(+);
    VEC4_BINARY_SCALAR_OP_LHS(-);

    VEC4_BINARY_SCALAR_OP_RHS(*);
    VEC4_BINARY_SCALAR_OP_RHS(/);
    VEC4_BINARY_SCALAR_OP_RHS(+);
    VEC4_BINARY_SCALAR_OP_RHS(-);
    ///
    static float SquaredLength(const Vec3& vec) { return vec.x * vec.x + vec.y * vec.y + vec.z * vec.z; }
    static float Length(const Vec3& vec) { return sqrtf(vec.x * vec.x + vec.y * vec.y + vec.z * vec.z); }
    static float Dot(const Vec3& a, const Vec3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

    static float SquaredLength(const Vec4& vec) { return vec.x * vec.x + vec.y * vec.y + vec.z * vec.z + vec.w * vec.w; }
    static float Length(const Vec4& vec) { return sqrtf(vec.x * vec.x + vec.y * vec.y + vec.z * vec.z + vec.w * vec.w); }
    static float Dot(const Vec4& a, const Vec4& b) { return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w; }

    static Vec3 Normalize(const Vec3& vec) { return vec / Length(vec); }
    static Vec4 Normalize(const Vec4& vec) { return vec / Length(vec); }

    static Vec3 Cross(const Vec3& a, const Vec3& b) { return Vec3(a.y * b.z - a.z * b.y, -(a.x * b.z - a.z * b.x), a.x * b.y - a.y * b.x); }
    static Vec3 Reflect(const Vec3& v, const Vec3& n) { return v - 2.0f * Dot(v, n) * n; }
    static bool Refract(const Vec3& v, const Vec3& n, float niOverNt, Vec3& outRefracted)
    {
        Vec3 uv = Normalize(v);
        float dt = Dot(uv, n);
        float discr = 1.0f - niOverNt * niOverNt * (1.0f - dt * dt);
        if (discr > 0.0f) {
            outRefracted = niOverNt * (uv - n * dt) - n * sqrtf(discr);
            return true;
        }
        return false;
    }


    static Vec4 Slerp(Vec4 a, Vec4 b, float alpha)
    {
        auto dot = Dot(a, b);
        if (dot < 0.0f) {
            b = -b;
            dot = -dot;
        }
        const float threshold = 0.9995f;
        if (dot > threshold) {
            return Normalize(a + alpha * (b - a));
        }
        dot = Clamp(dot, -1.0f, 1.0f);
        float theta0 = acosf(dot);
        float theta = theta0 * alpha;
        float s0 = Cos(theta) - dot * Sin(theta) / Sin(theta0);
        float s1 = Sin(theta) / Sin(theta0);

        return Normalize((s0 * a) + (s1 * b));
    }
    ///
    float Random(uint32_t& randomState);
    Vec3 RandomInUnitDisk(uint32_t& randomState);
    Vec3 RandomInUnitSphere(uint32_t& randomState);
    Vec3 RandomUnitVector(uint32_t& randomState);
}


namespace math
{
    static bool Inverse4x4FloatMatrixCM(float* m, float* invOut)
    {
        int i;
        float det;
        float inv[16];

        inv[0] = m[5] * m[10] * m[15] -
            m[5] * m[11] * m[14] -
            m[9] * m[6] * m[15] +
            m[9] * m[7] * m[14] +
            m[13] * m[6] * m[11] -
            m[13] * m[7] * m[10];

        inv[4] = -m[4] * m[10] * m[15] +
            m[4] * m[11] * m[14] +
            m[8] * m[6] * m[15] -
            m[8] * m[7] * m[14] -
            m[12] * m[6] * m[11] +
            m[12] * m[7] * m[10];

        inv[8] = m[4] * m[9] * m[15] -
            m[4] * m[11] * m[13] -
            m[8] * m[5] * m[15] +
            m[8] * m[7] * m[13] +
            m[12] * m[5] * m[11] -
            m[12] * m[7] * m[9];

        inv[12] = -m[4] * m[9] * m[14] +
            m[4] * m[10] * m[13] +
            m[8] * m[5] * m[14] -
            m[8] * m[6] * m[13] -
            m[12] * m[5] * m[10] +
            m[12] * m[6] * m[9];

        inv[1] = -m[1] * m[10] * m[15] +
            m[1] * m[11] * m[14] +
            m[9] * m[2] * m[15] -
            m[9] * m[3] * m[14] -
            m[13] * m[2] * m[11] +
            m[13] * m[3] * m[10];

        inv[5] = m[0] * m[10] * m[15] -
            m[0] * m[11] * m[14] -
            m[8] * m[2] * m[15] +
            m[8] * m[3] * m[14] +
            m[12] * m[2] * m[11] -
            m[12] * m[3] * m[10];

        inv[9] = -m[0] * m[9] * m[15] +
            m[0] * m[11] * m[13] +
            m[8] * m[1] * m[15] -
            m[8] * m[3] * m[13] -
            m[12] * m[1] * m[11] +
            m[12] * m[3] * m[9];

        inv[13] = m[0] * m[9] * m[14] -
            m[0] * m[10] * m[13] -
            m[8] * m[1] * m[14] +
            m[8] * m[2] * m[13] +
            m[12] * m[1] * m[10] -
            m[12] * m[2] * m[9];

        inv[2] = m[1] * m[6] * m[15] -
            m[1] * m[7] * m[14] -
            m[5] * m[2] * m[15] +
            m[5] * m[3] * m[14] +
            m[13] * m[2] * m[7] -
            m[13] * m[3] * m[6];

        inv[6] = -m[0] * m[6] * m[15] +
            m[0] * m[7] * m[14] +
            m[4] * m[2] * m[15] -
            m[4] * m[3] * m[14] -
            m[12] * m[2] * m[7] +
            m[12] * m[3] * m[6];

        inv[10] = m[0] * m[5] * m[15] -
            m[0] * m[7] * m[13] -
            m[4] * m[1] * m[15] +
            m[4] * m[3] * m[13] +
            m[12] * m[1] * m[7] -
            m[12] * m[3] * m[5];

        inv[14] = -m[0] * m[5] * m[14] +
            m[0] * m[6] * m[13] +
            m[4] * m[1] * m[14] -
            m[4] * m[2] * m[13] -
            m[12] * m[1] * m[6] +
            m[12] * m[2] * m[5];

        inv[3] = -m[1] * m[6] * m[11] +
            m[1] * m[7] * m[10] +
            m[5] * m[2] * m[11] -
            m[5] * m[3] * m[10] -
            m[9] * m[2] * m[7] +
            m[9] * m[3] * m[6];

        inv[7] = m[0] * m[6] * m[11] -
            m[0] * m[7] * m[10] -
            m[4] * m[2] * m[11] +
            m[4] * m[3] * m[10] +
            m[8] * m[2] * m[7] -
            m[8] * m[3] * m[6];

        inv[11] = -m[0] * m[5] * m[11] +
            m[0] * m[7] * m[9] +
            m[4] * m[1] * m[11] -
            m[4] * m[3] * m[9] -
            m[8] * m[1] * m[7] +
            m[8] * m[3] * m[5];

        inv[15] = m[0] * m[5] * m[10] -
            m[0] * m[6] * m[9] -
            m[4] * m[1] * m[10] +
            m[4] * m[2] * m[9] +
            m[8] * m[1] * m[6] -
            m[8] * m[2] * m[5];

        det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];

        if (det == 0)
            return false;

        det = 1.0f / det;

        for (i = 0; i < 16; i++)
            invOut[i] = inv[i] * det;

        return true;
    }



    static void Copy4x4FloatMatrixCM(float* matFrom, float* matTo)
    {
        memcpy(matTo, matFrom, sizeof(float) * 16);
    }

    static float Get4x4FloatMatrixValueCM(float* mat, int column, int row)
    {
        int index = 4 * column + row;
        return mat[index];
    }

    static void Set4x4FloatMatrixValueCM(float* mat, int column, int row, float value)
    {
        int index = 4 * column + row;
        mat[index] = value;
    }

    static Vec3 TransformPositionCM(const Vec3& pos, float* mat)
    {
        Vec4 vec4(pos, 1.0f);
        Vec4 result;
        for (int i = 0; i < 4; ++i) {
            float accum = 0.0f;
            for (int j = 0; j < 4; ++j) {
                float x = Get4x4FloatMatrixValueCM(mat, j, i);
                accum += x * vec4[j];
            }
            result[i] = accum;
        }
        return result.xyz;
    }

    static Vec4 TransformPositionCM(const Vec4& pos, float* mat)
    {
        Vec4 result;
        for (int i = 0; i < 4; ++i) {
            float accum = 0.0f;
            for (int j = 0; j < 4; ++j) {
                float x = Get4x4FloatMatrixValueCM(mat, j, i);
                accum += x * pos[j];
            }
            result[i] = accum;
        }
        return result;
    }

    static Vec3 TransformDirectionCM(const Vec3& dir, float* mat)
    {
        Vec4 vec4(dir, 0.0f);
        Vec4 result;
        for (int i = 0; i < 4; ++i) {
            float accum = 0.0f;
            for (int j = 0; j < 4; ++j) {
                float x = Get4x4FloatMatrixValueCM(mat, j, i);
                accum += x * vec4[j];
            }
            result[i] = accum;
        }
        return result.xyz;
    }

    static void Make4x4FloatMatrixIdentity(float* mat)
    {
        memset(mat, 0x0, sizeof(float) * 16);
        for (int i = 0; i < 4; ++i) {
            Set4x4FloatMatrixValueCM(mat, i, i, 1.0f);
        }
    }

    static Vec4 Get4x4FloatMatrixColumnCM(float* mat, int column)
    {
        return {
            Get4x4FloatMatrixValueCM(mat, column, 0),
            Get4x4FloatMatrixValueCM(mat, column, 1),
            Get4x4FloatMatrixValueCM(mat, column, 2),
            Get4x4FloatMatrixValueCM(mat, column, 3),
        };
    }

    static void Set4x4FloatMatrixColumnCM(float* mat, int column, Vec4 value)
    {
        for (int i = 0; i < 4; ++i) {
            Set4x4FloatMatrixValueCM(mat, column, i, value[i]);
        }
    }


    /*void Make4x4FloatProjectionMatrixCMLH(float* mat, float fovInRadians, float aspect, float near, float far)
    {
    Make4x4FloatMatrixIdentity(mat);
    float tanHalfFovy = tanf(fovInRadians / 2.0f);
    Set4x4FloatMatrixValueCM(mat, 0, 0, 1.0f / (aspect * tanHalfFovy));
    Set4x4FloatMatrixValueCM(mat, 1, 1, 1.0f / tanHalfFovy);
    Set4x4FloatMatrixValueCM(mat, 2, 3, 1.0f);
    Set4x4FloatMatrixValueCM(mat, 2, 2, (far * near) / (far - near));
    Set4x4FloatMatrixValueCM(mat, 3, 2, -(2.0f * far * near) / (far - near));
    }*/
    static void Make4x4FloatProjectionMatrixCMLH(float* mat, float fovInRadians, float width, float height, float near, float far)
    {
        Make4x4FloatMatrixIdentity(mat);

        float yScale = 1 / tanf(fovInRadians / 2.0f);
        float xScale = yScale / (width / height);

        Set4x4FloatMatrixValueCM(mat, 0, 0, xScale);
        Set4x4FloatMatrixValueCM(mat, 1, 1, yScale);
        Set4x4FloatMatrixValueCM(mat, 2, 2, far / (far - near));
        Set4x4FloatMatrixValueCM(mat, 3, 2, -near * far / (far - near));
        Set4x4FloatMatrixValueCM(mat, 2, 3, 1.0f);
        Set4x4FloatMatrixValueCM(mat, 3, 3, 0.0f);
    }

    static void Make4x4FloatOrthographicMatrixCMLH(float* mat, float left, float right, float bottom, float top, float zNear, float zFar)
    {
        Make4x4FloatMatrixIdentity(mat);

        Set4x4FloatMatrixValueCM(mat, 0, 0, 2.0f / (right - left));
        Set4x4FloatMatrixValueCM(mat, 1, 1, 2.0f / (top - bottom));
        Set4x4FloatMatrixValueCM(mat, 2, 2, 1.0f / (zFar - zNear));
        Set4x4FloatMatrixValueCM(mat, 0, 3, (left + right) / (left - right));
        Set4x4FloatMatrixValueCM(mat, 1, 3, (top + bottom) / (bottom - top));
        Set4x4FloatMatrixValueCM(mat, 2, 3, zNear / (zNear - zFar));
    }

    static void Make4x4FloatMatrixTranspose(float* mat, float* result)
    {
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                Set4x4FloatMatrixValueCM(result, j, i, Get4x4FloatMatrixValueCM(mat, i, j));
            }
        }
    }

    static void Make4x4FloatLookAtMatrixCMLH(float* mat, const Vec3& from, const Vec3& to, const Vec3& up)
    {
        Make4x4FloatMatrixIdentity(mat);

        // find frame for desired direction
        auto direction = Normalize(to - from);
        auto x = Normalize(Cross(up, direction));
        auto y = Normalize(Cross(direction, x));

        Set4x4FloatMatrixColumnCM(mat, 0, Vec4(x, 0.0f));
        Set4x4FloatMatrixColumnCM(mat, 1, Vec4(y, 0.0f));
        Set4x4FloatMatrixColumnCM(mat, 2, Vec4(direction, 0.0f));
        Set4x4FloatMatrixColumnCM(mat, 3, Vec4(from, 1.0f));   // translate

    }

    static void Make4x4FloatScaleMatrixCM(float* mat, float scale)
    {
        Make4x4FloatMatrixIdentity(mat);
        Set4x4FloatMatrixValueCM(mat, 0, 0, scale);
        Set4x4FloatMatrixValueCM(mat, 1, 1, scale);
        Set4x4FloatMatrixValueCM(mat, 2, 2, scale);
        Set4x4FloatMatrixValueCM(mat, 3, 3, 1.0f);
    }



    static void Make4x4FloatRotationMatrixCMLH(float* mat, Vec3 axisIn, float rad)
    {
        float rotate[16];
        float base[16];
        Make4x4FloatMatrixIdentity(base);
        Make4x4FloatMatrixIdentity(rotate);
        Make4x4FloatMatrixIdentity(mat);

        float a = rad;
        float c = cosf(a);
        float s = sinf(a);

        auto axis = Normalize(axisIn);
        auto temp = axis * (1.0f - c);

        Set4x4FloatMatrixValueCM(rotate, 0, 0, c + temp[0] * axis[0]);
        Set4x4FloatMatrixValueCM(rotate, 0, 1, temp[0] * axis[1] + s * axis[2]);
        Set4x4FloatMatrixValueCM(rotate, 0, 2, temp[0] * axis[2] - s * axis[1]);

        Set4x4FloatMatrixValueCM(rotate, 1, 0, temp[1] * axis[0] - s * axis[2]);
        Set4x4FloatMatrixValueCM(rotate, 1, 1, c + temp[1] * axis[1]);
        Set4x4FloatMatrixValueCM(rotate, 1, 2, temp[1] * axis[2] + s * axis[0]);

        Set4x4FloatMatrixValueCM(rotate, 2, 0, temp[2] * axis[0] + s * axis[1]);
        Set4x4FloatMatrixValueCM(rotate, 2, 1, temp[2] * axis[1] - s * axis[0]);
        Set4x4FloatMatrixValueCM(rotate, 2, 2, c + temp[2] * axis[2]);

        Vec4 m0 = Get4x4FloatMatrixColumnCM(base, 0);
        Vec4 m1 = Get4x4FloatMatrixColumnCM(base, 1);
        Vec4 m2 = Get4x4FloatMatrixColumnCM(base, 2);
        Vec4 m3 = Get4x4FloatMatrixColumnCM(base, 3);

        float r00 = Get4x4FloatMatrixValueCM(rotate, 0, 0);
        float r11 = Get4x4FloatMatrixValueCM(rotate, 1, 1);
        float r12 = Get4x4FloatMatrixValueCM(rotate, 1, 2);
        float r01 = Get4x4FloatMatrixValueCM(rotate, 0, 1);
        float r02 = Get4x4FloatMatrixValueCM(rotate, 0, 2);

        float r10 = Get4x4FloatMatrixValueCM(rotate, 1, 0);
        float r20 = Get4x4FloatMatrixValueCM(rotate, 2, 0);
        float r21 = Get4x4FloatMatrixValueCM(rotate, 2, 1);
        float r22 = Get4x4FloatMatrixValueCM(rotate, 2, 2);

        for (int i = 0; i < 4; ++i) {
            Set4x4FloatMatrixValueCM(mat, i, 0, m0[i] * r00 + m1[i] * r01 + m2[i] * r02);
            Set4x4FloatMatrixValueCM(mat, i, 1, m0[i] * r10 + m1[i] * r11 + m2[i] * r12);
            Set4x4FloatMatrixValueCM(mat, i, 2, m0[i] * r20 + m1[i] * r21 + m2[i] * r22);
            Set4x4FloatMatrixValueCM(mat, i, 3, m3[i]);
        }
    }

    static void Make4x4FloatTranslationMatrixCM(float* mat, Vec3 t)
    {
        Make4x4FloatMatrixIdentity(mat);
        for (int i = 0; i < 3; ++i) {
            Set4x4FloatMatrixValueCM(mat, 3, i, t[i]);
        }
    }

    static void SetTranslation4x4FloatMatrixCM(float* mat, Vec3 t)
    {
        for (int i = 0; i < 3; ++i) {
            Set4x4FloatMatrixValueCM(mat, 3, i, t[i]);
        }
        mat[15] = 1.0f;
    }

    // result = matA * matB
    static void MultiplyMatricesCM(float* left, float* right, float* result)
    {
        Make4x4FloatMatrixIdentity(result);
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                float acc = 0.0f;
                for (int k = 0; k < 4; ++k) {
                    acc += Get4x4FloatMatrixValueCM(left, k, i) * Get4x4FloatMatrixValueCM(right, j, k);
                }
                Set4x4FloatMatrixValueCM(result, j, i, acc);
            }
        }
    }
}