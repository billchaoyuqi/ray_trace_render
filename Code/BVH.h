//
// Created by 31934 on 2025/10/31.
//

#ifndef GRAPHIC_BVH_H
#define GRAPHIC_BVH_H
#pragma once
#include "Scene.h"
#include <vector>

struct AABB {
    Vector3 bmin, bmax;
    AABB();
    void expand(const AABB &o);
    void expand_point(const Vector3 &p);
    bool intersect(const Ray &ray, double tmin, double tmax) const;
    // 计算AABB的表面积（用于SAH）
    double surface_area() const;
    // 计算AABB的中心点
    Vector3 center() const;
};

struct BVHNode {
    AABB box;
    int left=-1, right=-1; // 子节点索引
    int first_prim = -1;
    int prim_count = 0;

    bool is_leaf() const { return prim_count > 0; }
};

struct BVH {
    std::vector<BVHNode> nodes;
    std::vector<int> prim_indices; // 对象索引

    void build(const Scene &scene);
    bool intersect(const Ray &ray, Hit &hit, const Scene &scene) const;

private:
    int build_recursive(const Scene &scene, int start, int end, int depth = 0);
    bool intersect_recursive(const Ray &ray, Hit &hit, const Scene &scene, int node_idx) const;
};

#endif //GRAPHIC_BVH_H