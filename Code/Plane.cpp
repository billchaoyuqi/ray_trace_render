#include "Plane.h"
#include <cmath>

// Helper: point-in-triangle using barycentric (works in 3D on same plane)
static bool point_in_triangle(const Vector3 &p, const Vector3 &a, const Vector3 &b, const Vector3 &c) {
    Vector3 v0 = c - a;
    Vector3 v1 = b - a;
    Vector3 v2 = p - a;
    double dot00 = v0.dot(v0);
    double dot01 = v0.dot(v1);
    double dot02 = v0.dot(v2);
    double dot11 = v1.dot(v1);
    double dot12 = v1.dot(v2);
    double denom = dot00 * dot11 - dot01 * dot01;
    if (std::abs(denom) < 1e-12) return false;
    double u = (dot11 * dot02 - dot01 * dot12) / denom;
    double v = (dot00 * dot12 - dot01 * dot02) / denom;
    return (u >= -1e-8) && (v >= -1e-8) && (u + v <= 1.0 + 1e-8);
}

bool Plane::intersect(const Ray &ray, Hit &hit) const {
    // plane from corners[0..3], treat as convex quad split into two triangles (0,1,2) and (0,2,3)
    Vector3 a = corners[0], b = corners[1], c = corners[2], d = corners[3];

    Vector3 uvec = b - a;
    Vector3 vvec = d - a;
    double ulen2 = uvec.dot(uvec);
    double vlen2 = vvec.dot(vvec);

    Vector3 normal = (b - a).cross(c - a).normalized();
    double denom = normal.dot(ray.dir);
    if (std::abs(denom) < 1e-12) return false; // parallel
    double t = normal.dot(a - ray.origin) / denom;
    if (t < 1e-6 || t >= hit.t) return false;
    Vector3 p = ray.origin + ray.dir * t;
    // check inside quad by triangles
    if (point_in_triangle(p, corners[0], corners[1], corners[2]) ||
        point_in_triangle(p, corners[0], corners[2], corners[3])) {
        hit.hit = true;
        hit.t = t;
        hit.pos = p;
        hit.normal = (denom < 0) ? normal : normal * -1.0; // adjust normal to face opposite ray if needed
        hit.color = this->color;
        hit.material = this->material;

        // 纹理
        Vector3 local = p - a;
        double u = local.dot(uvec) / ulen2; // 0..1 over edge
        double v = local.dot(vvec) / vlen2;
        hit.u = u;
        hit.v = v;
        hit.texture = this->texture_image;
        return true;
    }
    return false;
}

void Plane::bounds(Vector3 &bmin, Vector3 &bmax) const {
    bmin = { 1e30, 1e30, 1e30 }; bmax = { -1e30, -1e30, -1e30 };
    for (int i=0;i<4;i++){
        if (corners[i].x < bmin.x) bmin.x = corners[i].x;
        if (corners[i].y < bmin.y) bmin.y = corners[i].y;
        if (corners[i].z < bmin.z) bmin.z = corners[i].z;
        if (corners[i].x > bmax.x) bmax.x = corners[i].x;
        if (corners[i].y > bmax.y) bmax.y = corners[i].y;
        if (corners[i].z > bmax.z) bmax.z = corners[i].z;
    }
}
