#include <cmath>

#define ARRAY_COUNT(array) (sizeof((array))/sizeof((array)[0]))

struct v3
{
    float x, y, z;

    inline float length() const { return std::sqrt(x*x + y*y + z*z); }
    inline float length_sq() const { return x*x + y*y + z*z; }
};

static inline bool operator==(const v3 &a, const v3 &b)
{
    return a.x == b.x && a.y == b.y;
}

static inline v3 operator+(const v3 &a, const v3 &b)
{
    return v3{ a.x + b.x, a.y + b.y, a.z + b.z };
}

static inline v3 operator-(const v3 &a, const v3 &b)
{
    return v3{ a.x - b.x, a.y - b.y, a.z - b.z };
}

static inline v3 operator*(const v3 &a, const v3 &b)
{
    return v3{ a.x * b.x, a.y * b.y, a.z * b.z };
}

static inline v3 operator/(const v3 &a, const v3 &b)
{
    return v3{ a.x / b.x, a.y / b.y, a.z / b.z };
}

static inline v3 operator*(const v3 &vec, const float &scalar)
{
    return v3{ vec.x * scalar, vec.y * scalar, vec.z * scalar };
}

static inline v3 operator*(const float &scalar, const v3 &vec)
{
    return v3{ vec.x * scalar, vec.y * scalar, vec.z * scalar };
}

static inline v3 operator/(const v3 &vec, const float &scalar)
{
    return v3{ vec.x / scalar, vec.y / scalar, vec.z / scalar };
}

static inline v3 &operator+=(v3 &a, const v3 &b)
{
    a = a + b;
    return a;
}

static inline v3 &operator/=(v3 &vec, const float &scalar)
{
    vec = vec / scalar;
    return vec;
}

static inline v3 v3_normalize(const v3 &vec)
{
    v3 result = vec / vec.length();
    return result;
}

static inline float v3_dot(const v3 &a, const v3 &b)
{
    float result = a.x * b.x + a.y * b.y + a.z * b.z;
    return result;
}

static inline v3 v3_cross(const v3 &a, const v3 &b)
{
    v3 result{ a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x };
    return result;
}

static inline v3 v3_lerp(const v3 &a, const v3 &b, float t)
{
    v3 result = (1.0f - t) * a + t * b;
    return result;
}

struct Ray
{
    v3 origin;
    v3 direction;

    inline v3 at_param(float t) const { return origin + t * direction; }
};
