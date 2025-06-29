// alglin.hpp
#pragma once
#include <cmath>

struct Vec3 {
    float x, y, z;

    Vec3() : x(0), y(0), z(0) {}
    Vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}

    Vec3 operator+(const Vec3& v) const { return {x + v.x, y + v.y, z + v.z}; }
    Vec3 operator-(const Vec3& v) const { return {x - v.x, y - v.y, z - v.z}; }
    Vec3 operator*(float s) const { return {x * s, y * s, z * s}; }
};

struct Mat3 {
    float m[9];

    Vec3 operator*(const Vec3& v) const {
        return Vec3(
            m[0]*v.x + m[1]*v.y + m[2]*v.z,
            m[3]*v.x + m[4]*v.y + m[5]*v.z,
            m[6]*v.x + m[7]*v.y + m[8]*v.z
        );
    }

    Mat3 operator*(const Mat3& b) const {
        Mat3 r;
        for (int row = 0; row < 3; ++row)
            for (int col = 0; col < 3; ++col) {
                r.m[row*3+col] = 0;
                for (int k = 0; k < 3; ++k)
                    r.m[row*3+col] += m[row*3+k] * b.m[k*3+col];
            }
        return r;
    }

    const float* data() const { return m; }

    static Mat3 rotation(int axis, float angleRadians) {
        float c = std::cos(angleRadians);
        float s = std::sin(angleRadians);
        Mat3 r = identity();
        switch (axis) {
            case 0:
                r.m[4] = c; r.m[5] = -s;
                r.m[7] = s; r.m[8] = c; break;
            case 1:
                r.m[0] = c; r.m[2] = s;
                r.m[6] = -s; r.m[8] = c; break;
            case 2:
                r.m[0] = c; r.m[1] = -s;
                r.m[3] = s; r.m[4] = c; break;
        }
        return r;
    }

	static Mat3 rotXY(float angleX, float angleY) {
        float cx = std::cos(angleX), sx = std::sin(angleX);
        float cy = std::cos(angleY), sy = std::sin(angleY);
        Mat3 m;
        m.m[0] = cy;        m.m[1] = 0;      m.m[2] = sy;
        m.m[3] = sx*sy;     m.m[4] = cx;     m.m[5] = -sx*cy;
        m.m[6] = -cx*sy;    m.m[7] = sx;     m.m[8] = cx*cy;
        return m;
    }
    static Mat3 identity() {
        Mat3 m = {};
        m.m[0] = m.m[4] = m.m[8] = 1.0f;
        return m;
    }
};

inline float dot(const Vec3& a, const Vec3& b) {
    return a.x*b.x + a.y*b.y + a.z*b.z;
}

inline Vec3 cross(const Vec3& a, const Vec3& b) {
    return Vec3(
        a.y*b.z - a.z*b.y,
        a.z*b.x - a.x*b.z,
        a.x*b.y - a.y*b.x
    );
}

inline Vec3 normalize(const Vec3& v) {
    float len = std::sqrt(dot(v, v));
    if (len == 0.0f) return v;
    return v * (1.0f / len);
}

// Ray/triangle intersection using Möller–Trumbore algorithm
inline bool intersectRayTriangle(const Vec3& orig, const Vec3& dir,
                                 const Vec3& v0, const Vec3& v1, const Vec3& v2,
                                 float& t, Vec3& hitPoint) {
    const float EPS = 1e-6f;
    Vec3 edge1 = v1 - v0;
    Vec3 edge2 = v2 - v0;
    Vec3 h = cross(dir, edge2);
    float a = dot(edge1, h);
    if (std::fabs(a) < EPS) return false;
    float f = 1.0f / a;
    Vec3 s = orig - v0;
    float u = f * dot(s, h);
    if (u < 0.0f || u > 1.0f) return false;
    Vec3 q = cross(s, edge1);
    float v = f * dot(dir, q);
    if (v < 0.0f || u + v > 1.0f) return false;
    float tTmp = f * dot(edge2, q);
    if (tTmp < 0.0f) return false;
    t = tTmp;
    hitPoint = orig + dir * t;
    return true;
}

