//
// Created by 31934 on 2025/11/11.
//
#include "SceneUtils.h"
#include "BVH.h"

// 普通遍历版本
bool intersect_scene(const Ray &ray, const Scene &scene, Hit &hit) {
    bool any_hit = false;
    for (const auto &obj : scene.objects) {
        Hit temp_hit;
        if (obj->intersect(ray, temp_hit)) {
            if (temp_hit.t < hit.t) {
                hit = temp_hit;
                any_hit = true;
            }
        }
    }
    return any_hit;
}

// BVH 加速版本
bool intersect_scene(const Ray &ray, const BVH &bvh, const Scene &scene, Hit &hit) {
    return bvh.intersect(ray, hit, scene);
}
