#include "Cube.h"
#include <algorithm>
#include <limits>
#include <cmath>

bool Cube::intersect(const Ray &ray, Hit &hit) const {

    Vector3 O = ray.origin;
    Vector3 D = ray.dir;   // 不归一化

    // 三条世界坐标轴方向（已旋转）
    Vector3 axisX = rot.mul(Vector3(1,0,0));
    Vector3 axisY = rot.mul(Vector3(0,1,0));
    Vector3 axisZ = rot.mul(Vector3(0,0,1));

    // 半尺寸（使用你的 size）
    Vector3 half = size * 0.5;

    double tMin = -1e18, tMax = 1e18;
    Vector3 bestNormal;

    auto checkAxis = [&](const Vector3& axis, double h, const Vector3& norm) {
        double e = axis.dot(center - O);
        double f = axis.dot(D);

        if (std::abs(f) < 1e-6) {
            // 光线与该方向的面平行
            if (std::abs(e) > h) return false;
            return true;
        }

        double t1 = (e - h) / f;
        double t2 = (e + h) / f;
        Vector3 n1 = -norm;
        Vector3 n2 =  norm;

        if (t1 > t2) { std::swap(t1, t2); std::swap(n1, n2); }

        if (t1 > tMin) {
            tMin = t1;
            bestNormal = n1;
        }
        if (t2 < tMax) tMax = t2;

        if (tMin > tMax) return false;
        if (tMax < 1e-6) return false;

        return true;
    };

    if (!checkAxis(axisX, half.x, axisX.normalized())) return false;
    if (!checkAxis(axisY, half.y, axisY.normalized())) return false;
    if (!checkAxis(axisZ, half.z, axisZ.normalized())) return false;

    double tHit = (tMin > 1e-6) ? tMin : tMax;
    if (tHit < 1e-6 || tHit >= hit.t) return false;

    Vector3 P = O + D * tHit;

    hit.hit = true;
    hit.t = tHit;
    hit.pos = P;
    hit.normal = bestNormal.normalized();
    hit.color = color;
    hit.material = material;
    hit.texture = texture_image;

    // ----------- 计算 UV（只用 local 坐标）-----------
    Vector3 local = rot_inv.mul(P - center);
    local.x /= half.x;
    local.y /= half.y;
    local.z /= half.z;

    double ax = std::abs(hit.normal.dot(axisX));
    double ay = std::abs(hit.normal.dot(axisY));
    double az = std::abs(hit.normal.dot(axisZ));

    double u, v;

    if (ax > ay && ax > az) {
        // ±X 面
        u = 0.5 + 0.5 * local.z;
        v = 0.5 + 0.5 * local.y;
    } else if (ay > ax && ay > az) {
        // ±Y 面
        u = 0.5 + 0.5 * local.x;
        v = 0.5 + 0.5 * local.z;
    } else {
        // ±Z 面
        u = 0.5 + 0.5 * local.x;
        v = 0.5 + 0.5 * local.y;
    }

    hit.u = u;
    hit.v = v;

    return true;
}

void Cube::bounds(Vector3 &bmin, Vector3 &bmax) const {
    Vector3 half = size * 0.5;

    Vector3 localCorners[8] = {
        { -half.x, -half.y, -half.z },
        { -half.x, -half.y,  half.z },
        { -half.x,  half.y, -half.z },
        { -half.x,  half.y,  half.z },
        {  half.x, -half.y, -half.z },
        {  half.x, -half.y,  half.z },
        {  half.x,  half.y, -half.z },
        {  half.x,  half.y,  half.z }
    };

    bmin = Vector3( 1e18, 1e18, 1e18 );
    bmax = Vector3(-1e18,-1e18,-1e18);

    for (int i = 0; i < 8; i++) {
        // 世界空间角点 = center + rot * corner
        Vector3 wc = center + rot.mul(localCorners[i]);
        bmin = min(bmin, wc);
        bmax = max(bmax, wc);
    }
}

void Cube::set_rotation(double rx_deg, double ry_deg, double rz_deg) {
    double rx = rx_deg * M_PI / 180.0;
    double ry = ry_deg * M_PI / 180.0;
    double rz = rz_deg * M_PI / 180.0;

    rot = Matrix3::from_euler(rx, ry, rz);
    rot_inv = rot.transpose();
}
