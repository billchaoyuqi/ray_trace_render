#include "BVH.h"
#include <algorithm>
#include <stack>
#include <limits>
#include <cmath>

// AABB 方法实现
AABB::AABB() {
    bmin = Vector3(1e30, 1e30, 1e30);
    bmax = Vector3(-1e30, -1e30, -1e30);
}

void AABB::expand(const AABB &o) {
    bmin.x = std::min(bmin.x, o.bmin.x);
    bmin.y = std::min(bmin.y, o.bmin.y);
    bmin.z = std::min(bmin.z, o.bmin.z);
    bmax.x = std::max(bmax.x, o.bmax.x);
    bmax.y = std::max(bmax.y, o.bmax.y);
    bmax.z = std::max(bmax.z, o.bmax.z);
}

void AABB::expand_point(const Vector3 &p) {
    bmin.x = std::min(bmin.x, p.x);
    bmin.y = std::min(bmin.y, p.y);
    bmin.z = std::min(bmin.z, p.z);
    bmax.x = std::max(bmax.x, p.x);
    bmax.y = std::max(bmax.y, p.y);
    bmax.z = std::max(bmax.z, p.z);
}

// static AABB shape_bounds(const Shape &s) {
//     Vector3 bmin, bmax;
//     s.bounds(bmin, bmax);
//     AABB a;
//     a.bmin = bmin;
//     a.bmax = bmax;
//     return a;
// }

bool AABB::intersect(const Ray &ray, double tmin, double tmax) const {
    // 添加对零方向的容错处理
    const double epsilon = 1e-8;

    // X轴
    if (std::abs(ray.dir.x) < epsilon) {
        if (ray.origin.x < bmin.x || ray.origin.x > bmax.x) return false;
    }
    else {
        double invD = 1.0 / ray.dir.x;
        double t0 = (bmin.x - ray.origin.x) * invD;
        double t1 = (bmax.x - ray.origin.x) * invD;
        if (invD < 0) std::swap(t0, t1);
        tmin = std::max(t0, tmin);
        tmax = std::min(t1, tmax);
        if (tmax <= tmin) return false;
    }

    // Y轴
    if (std::abs(ray.dir.y) < epsilon) {
        if (ray.origin.y < bmin.y || ray.origin.y > bmax.y) return false;
    }
    else {
        double invD = 1.0 / ray.dir.y;
        double t0 = (bmin.y - ray.origin.y) * invD;
        double t1 = (bmax.y - ray.origin.y) * invD;
        if (invD < 0) std::swap(t0, t1);
        tmin = std::max(t0, tmin);
        tmax = std::min(t1, tmax);
        if (tmax <= tmin) return false;
    }

    // Z轴
    if (std::abs(ray.dir.z) < epsilon) {
        if (ray.origin.z < bmin.z || ray.origin.z > bmax.z) return false;
    }
    else {
        double invD = 1.0 / ray.dir.z;
        double t0 = (bmin.z - ray.origin.z) * invD;
        double t1 = (bmax.z - ray.origin.z) * invD;
        if (invD < 0) std::swap(t0, t1);
        tmin = std::max(t0, tmin);
        tmax = std::min(t1, tmax);
        if (tmax <= tmin) return false;
    }

    return true;
}

double AABB::surface_area() const {
    Vector3 d = bmax - bmin;
    return 2.0 * (d.x * d.y + d.x * d.z + d.y * d.z);
}

Vector3 AABB::center() const {
    return Vector3(
        (bmin.x + bmax.x) * 0.5,
        (bmin.y + bmax.y) * 0.5,
        (bmin.z + bmax.z) * 0.5
    );
}

// BVH 方法实现
void BVH::build(const Scene &scene) {
    if (scene.objects.empty()) return;

    // 初始化对象索引
    prim_indices.resize(scene.objects.size());
    for (size_t i = 0; i < scene.objects.size(); i++) {
        prim_indices[i] = i;
    }

    // 递归构建BVH
    nodes.reserve(scene.objects.size() * 2);
    nodes.clear();

    int root_idx = build_recursive(scene, 0, prim_indices.size(), 0);
    (void)root_idx; // 根节点已经在nodes[0]
}

int BVH::build_recursive(const Scene &scene, int start, int end, int depth) {
    BVHNode node;

    // 计算当前节点的AABB（包含所有对象的AABB）
    for (int i = start; i < end; i++) {
        int obj_idx = prim_indices[i];
        const auto& obj = scene.objects[obj_idx];

        // 获取对象的AABB
        Vector3 obj_bmin, obj_bmax;
        obj->bounds(obj_bmin, obj_bmax);

        // 更新当前节点的AABB
        node.box.expand_point(obj_bmin);
        node.box.expand_point(obj_bmax);
    }

    int node_idx = nodes.size();
    nodes.push_back(node);

    int prim_count = end - start;

    // 如果对象数量少或深度太大，创建叶子节点
    if (prim_count <= 2 || depth > 40) {
        nodes[node_idx].first_prim = start;
        nodes[node_idx].prim_count = prim_count;
        return node_idx;
    }

    // 选择分割轴和位置
    Vector3 extent = node.box.bmax - node.box.bmin;
    int axis = 0;
    if (extent.y > extent.x) axis = 1;
    if (extent.z > extent.x && extent.z > extent.y) axis = 2;  // 修正轴选择逻辑

    double split_pos;
    if (axis == 0) split_pos = node.box.bmin.x + extent.x * 0.5;
    else if (axis == 1) split_pos = node.box.bmin.y + extent.y * 0.5;
    else split_pos = node.box.bmin.z + extent.z * 0.5;

    // 分割对象
    int mid = start;
    for (int i = start; i < end; i++) {
        int obj_idx = prim_indices[i];
        const auto& obj = scene.objects[obj_idx];

        // 使用对象AABB的中心进行分割
        Vector3 obj_bmin, obj_bmax;
        obj->bounds(obj_bmin, obj_bmax);
        Vector3 center = (obj_bmin + obj_bmax) * 0.5;

        double center_component;
        if (axis == 0) center_component = center.x;
        else if (axis == 1) center_component = center.y;
        else center_component = center.z;

        if (center_component < split_pos) {
            std::swap(prim_indices[i], prim_indices[mid]);
            mid++;
        }
    }

    // 如果分割失败，创建叶子节点
    if (mid == start || mid == end) {
        // 尝试不同的分割策略
        mid = start + (end - start) / 2;

        // 如果还是失败，创建叶子节点
        if (mid == start || mid == end) {
            nodes[node_idx].first_prim = start;
            nodes[node_idx].prim_count = prim_count;
            return node_idx;
        }
    }

    // 递归构建子节点
    nodes[node_idx].left = build_recursive(scene, start, mid, depth + 1);
    nodes[node_idx].right = build_recursive(scene, mid, end, depth + 1);

    return node_idx;
}

bool BVH::intersect(const Ray &ray, Hit &hit, const Scene &scene) const {
    if (nodes.empty()) return false;

    // 保存原始t值用于恢复
    double original_t = hit.t;
    bool found = intersect_recursive(ray, hit, scene, 0);

    // 如果没有找到交点，恢复原始t值
    if (!found) {
        hit.t = original_t;
    }

    return found;
}

bool BVH::intersect_recursive(const Ray &ray, Hit &hit, const Scene &scene, int node_idx) const {
    const BVHNode& node = nodes[node_idx];

    // 检查与AABB的相交
    double t_min = 0.001;  // 避免自相交
    double t_max = hit.t;  // 使用当前最近交点的t值进行优化
    if (!node.box.intersect(ray, t_min, t_max)) {
        return false;
    }

    bool found_hit = false;

    // 如果是叶子节点，检查所有对象
    if (node.is_leaf()) {
        for (int i = 0; i < node.prim_count; i++) {
            int obj_idx = prim_indices[node.first_prim + i];
            if (scene.objects[obj_idx]->intersect(ray, hit)) {
                found_hit = true;
            }
        }
        return found_hit;
    }

    // 内部节点，递归检查子节点
    bool hit_left = intersect_recursive(ray, hit, scene, node.left);
    bool hit_right = intersect_recursive(ray, hit, scene, node.right);

    return hit_left || hit_right;
}
