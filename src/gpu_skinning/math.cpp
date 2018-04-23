#include "math.h"

static uint32_t g_randomState = 0xdeadbeef;

static uint32_t XorShift32(uint32_t& state)
{
    uint32_t x = state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 15;
    state = x;
    return x;
}

float math::Random(uint32_t& state)
{
    return (XorShift32(state) & 0xFFFFFF) / 16777216.0f;
}

math::Vec3 math::RandomInUnitDisk(uint32_t& state)
{
    Vec3 p;
    do {
        p = 2.0 * Vec3(Random(state), Random(state), 0) - Vec3(1, 1, 0);
    } while (Dot(p, p) >= 1.0);
    return p;
}

math::Vec3 math::RandomInUnitSphere(uint32_t& state)
{
    Vec3 p;
    do {
        p = 2.0*Vec3(Random(state), Random(state), Random(state)) - Vec3(1, 1, 1);
    } while (SquaredLength(p) >= 1.0);
    return p;
}

math::Vec3 math::RandomUnitVector(uint32_t& state)
{
    float z = Random(state) * 2.0f - 1.0f;
    float a = Random(state) * 2.0f * PI;
    float r = sqrtf(1.0f - z * z);
    float x = r * cosf(a);
    float y = r * sinf(a);
    return Vec3(x, y, z);
}