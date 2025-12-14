//
// Created by 31934 on 2025/10/15.
//

#ifndef CWPROJECT_VEC3_H
#define CWPROJECT_VEC3_H
#pragma once
#include <cmath>
#include <iostream>

struct Vector3 {
    double x, y, z;

    Vector3(double x_=0, double y_=0, double z_=0)
        : x(x_), y(y_), z(z_) {}

    Vector3 operator+(const Vector3& b) const
    {
        return {x + b.x, y + b.y, z + b.z};
    }
    Vector3 operator-(const Vector3& b) const
    {
        return {x - b.x, y - b.y, z - b.z};
    }
    Vector3 operator*(double s) const
    {
        return {x * s, y * s, z * s};
    }
    Vector3 operator/(double s) const
    {
        return {x / s, y / s, z / s};
    }
    // 标量与向量除法（友元函数）
    friend Vector3 operator/(double s, const Vector3& v)
    {
        return {s / v.x, s / v.y, s / v.z};
    }
    Vector3& operator+=(const Vector3& b)
    {
        x += b.x;
        y += b.y;
        z += b.z;
        return *this;
    }
    Vector3 operator*(const Vector3 &b) const
    {
        return {x * b.x, y * b.y, z * b.z};
    }
    // 标量与向量乘法（友元函数）
    friend Vector3 operator*(double s, const Vector3& v) {
        return {s * v.x, s * v.y, s * v.z};
    }

    // ✅ 一元负号运算符（允许 -light_dir）
    Vector3 operator-() const
    {
        return {-x, -y, -z};
    }

    // 向量与向量逐元素除法
    Vector3 operator/(const Vector3& b) const {
        return {x / b.x, y / b.y, z / b.z};
    }

    double dot(const Vector3& b) const
    {
        return x * b.x + y * b.y + z * b.z;
    }
    Vector3 cross(const Vector3& b) const
    {
        return {y * b.z - z * b.y, z * b.x - x * b.z, x * b.y - y * b.x};
    }

    double length() const
    {
        return std::sqrt(x * x + y * y + z * z);
    }

    Vector3 normalized() const {
        double l = length();
        if (l < 1e-12) return {0, 0, 0};
        return {x / l, y / l, z / l};
    }
};

inline std::ostream& operator<<(std::ostream &os, const Vector3 &v) {
    os << "(" << v.x << ", " << v.y << ", " << v.z << ")";
    return os;
}

// 向量逐元素最小值
inline Vector3 min(const Vector3& a, const Vector3& b) {
    return Vector3(
        std::min(a.x, b.x),
        std::min(a.y, b.y),
        std::min(a.z, b.z)
    );
}

// 向量逐元素最大值
inline Vector3 max(const Vector3& a, const Vector3& b) {
    return Vector3(
        std::max(a.x, b.x),
        std::max(a.y, b.y),
        std::max(a.z, b.z)
    );
}

#endif //CWPROJECT_VEC3_H
