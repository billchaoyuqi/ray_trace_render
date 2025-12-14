#include "Sphere.h"
#include <cmath>

bool Sphere::intersect(const Ray &ray, Hit &hit) const {
    // ray: o + t d
    Vector3 L = ray.origin - center;
    double a = ray.dir.dot(ray.dir);
    double b = 2.0 * ray.dir.dot(L);
    double c = L.dot(L) - radius*radius;
    double disc = b*b - 4*a*c;
    if (disc < 0) return false;
    double sq = std::sqrt(disc);
    double t0 = (-b - sq) / (2*a);
    double t1 = (-b + sq) / (2*a);
    double t = t0;
    if (t < 1e-6) t = t1;
    if (t < 1e-6) return false;
    if (t >= hit.t) return false;
    hit.hit = true;
    hit.t = t;
    hit.pos = ray.origin + ray.dir * t;
    hit.normal = (hit.pos - center).normalized();
    hit.color = this->color;
    hit.material = this->material;

    // 计算球面 UV（基于归一化法线 n）
    Vector3 n = hit.normal; // 已归一化
    double u = 0.5 + std::atan2(n.z, n.x) / (2.0 * M_PI);
    double v = 0.5 - std::asin(n.y) / M_PI;
    hit.u = u;
    hit.v = v;
    hit.texture = this->texture_image; // 可能为空

    return true;
}

void Sphere::bounds(Vector3 &bmin, Vector3 &bmax) const {
    bmin = { center.x - radius, center.y - radius, center.z - radius };
    bmax = { center.x + radius, center.y + radius, center.z + radius };
}
