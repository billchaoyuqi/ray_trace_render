//
// Created by 31934 on 2025/10/31.
//

#ifndef GRAPHIC_SCENE_H
#define GRAPHIC_SCENE_H
#pragma once

#include <vector>
#include <memory>
#include <string>
#include "Shape.h"
#include "camera.h"
#include "Sphere.h"
#include "Plane.h"
#include "Cube.h"
#include "Vector3.h"

// 点光源结构
struct PointLight {
    Vector3 pos;
    double intensity;

    double radius = 0.0;  // 光源半径，0表示点光源
    // Vector3 normal;       // 面光源的法线方向

    PointLight() : pos(0,0,0), intensity(1.0) {}
    PointLight(const Vector3 &p, double i,double r) : pos(p), intensity(i),radius(r) {}
};

// 场景结构
struct Scene {
    std::shared_ptr<Camera> camera;
    std::vector<std::shared_ptr<Shape>> objects;
    std::vector<PointLight> lights;

    Vector3 background_color = {0.8, 0.9, 1.0};  // 默认天空色
    Vector3 ambient_light   = {0.1, 0.1, 0.1};  // 默认环境光
};


// 从 ASCII 文本文件加载场景
Scene load_scene_txt(const std::string &filename);

#endif //GRAPHIC_SCENE_H
