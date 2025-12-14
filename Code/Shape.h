//
// Created by 31934 on 2025/10/31.
//

#ifndef GRAPHIC_SHAPE_H
#define GRAPHIC_SHAPE_H
#pragma once
#include "Ray.h"
#include "Vector3.h"
#include <limits>
#include <memory>

struct Material {
    double reflectivity = 0.0;
    double refractivity = 0.0;
    double ior = 1.0;
    double shininess = 32.0;
    double roughness = 0.0; // 新增：0为镜面，1 为粗糙
};

struct Hit {
    bool hit = false;
    double t = std::numeric_limits<double>::infinity();
    Vector3 pos;
    Vector3 normal;
    Vector3 color; // simple diffuse albedo
    Material material;
    //Vector2 uv;
    // 纹理坐标（0..1）
    double u = 0.0;
    double v = 0.0;

    // 指向纹理图像（可为空）
    std::shared_ptr<class Image> texture;
};

class Shape {
public:
    std::string name;
    Vector3 color = {0.8, 0.8, 0.8};
    // 材质
    Material material;
    // 纹理文件名
    std::string texture_file;
    // 纹理图像
    std::shared_ptr<Image> texture_image; // in Shape

    virtual ~Shape() {}
    // returns true if hit and fills hit data (with distance measured along ray)
    virtual bool intersect(const Ray &r, Hit &h) const = 0;
    // bounding box for BVH:
    virtual void bounds(Vector3 &bmin, Vector3 &bmax) const = 0;
};

#endif //GRAPHIC_SHAPE_H