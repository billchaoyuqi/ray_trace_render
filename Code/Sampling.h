//
// Created by 31934 on 2025/12/2.
//
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef GRAPHIC_CW_SAMPLING_H
#define GRAPHIC_CW_SAMPLING_H
#include "Vector3.h"
#include <random>
#include <cmath>

// 线程局部随机数生成器声明
extern thread_local std::mt19937 g_rng;

// 在单位圆盘上均匀采样（用于面光源）
inline Vector3 uniformSampleDisk(double radius = 1.0) {
    thread_local std::uniform_real_distribution<> dis(0.0, 1.0);

    double r = sqrt(dis(g_rng)) * radius;
    double theta = 2.0 * M_PI * dis(g_rng);

    return Vector3(r * cos(theta), r * sin(theta), 0);
}

// 在单位球面上均匀采样
inline Vector3 uniformSampleSphere() {
    thread_local std::uniform_real_distribution<> dis(0.0, 1.0);

    double z = 2.0 * dis(g_rng) - 1.0;
    double phi = 2.0 * M_PI * dis(g_rng);
    double r = sqrt(1.0 - z * z);

    return Vector3(r * cos(phi), r * sin(phi), z);
}

// 在单位半球上余弦重要性采样（用于漫反射）
inline Vector3 cosineSampleHemisphere(const Vector3& normal) {
    thread_local std::uniform_real_distribution<> dis(0.0, 1.0);

    double u1 = dis(g_rng);
    double u2 = dis(g_rng);

    // 采样单位圆盘
    double r = sqrt(u1);
    double phi = 2.0 * M_PI * u2;

    double x = r * cos(phi);
    double y = r * sin(phi);
    double z = sqrt(fmax(0.0, 1.0 - x*x - y*y));

    // 建立局部坐标系
    Vector3 w = normal.normalized();
    Vector3 u;

    // 选择一个与w不平行的向量
    if (fabs(w.x) > 0.9)
        u = Vector3(0, 1, 0);
    else
        u = Vector3(1, 0, 0);

    u = u.cross(w).normalized();
    Vector3 v = w.cross(u);

    // 转换到世界坐标系
    return (u * x + v * y + w * z).normalized();
}

// GGX重要性采样（用于粗糙表面反射）
inline Vector3 ggxSampleHemisphere(const Vector3& normal, const Vector3& viewDir, double roughness) {
    thread_local std::uniform_real_distribution<> dis(0.0, 1.0);

    double a = roughness * roughness;
    double a2 = a * a;

    // 采样GGX分布
    double phi = 2.0 * M_PI * dis(g_rng);
    double cosTheta = sqrt((1.0 - dis(g_rng)) / (dis(g_rng) * (a2 - 1.0) + 1.0));
    double sinTheta = sqrt(fmax(0.0, 1.0 - cosTheta * cosTheta));

    // 微表面法线（切线空间）
    Vector3 h;
    h.x = sinTheta * cos(phi);
    h.y = sinTheta * sin(phi);
    h.z = cosTheta;

    // 从切线空间转换到世界空间
    Vector3 up = fabs(normal.z) < 0.999 ? Vector3(0, 0, 1) : Vector3(1, 0, 0);
    Vector3 tangent = up.cross(normal).normalized();
    Vector3 bitangent = normal.cross(tangent);

    Vector3 hWorld = tangent * h.x + bitangent * h.y + normal * h.z;
    hWorld = hWorld.normalized();

    // 计算反射方向
    Vector3 reflectDir = viewDir - hWorld * 2.0 * viewDir.dot(hWorld);
    return reflectDir.normalized();
}

// 计算光源采样位置（根据光源半径）
inline Vector3 sampleLightPosition(const PointLight& light) {
    if (light.radius <= 0.0) {
        return light.pos;  // 点光源
    }

    // 面光源：在圆盘上采样
    Vector3 offset = uniformSampleDisk(light.radius);

    // 如果有光源法线方向，需要将采样点旋转到正确的方向
    // 这里简化处理：假设光源在XY平面
    return light.pos + offset;
}
#endif //GRAPHIC_CW_SAMPLING_H