//
// Created by 31934 on 2025/11/11.
//

#ifndef GRAPHIC_CW_SCENEUTILS_H
#define GRAPHIC_CW_SCENEUTILS_H
#pragma once
#include "Scene.h"
#include "Ray.h"
#include "Shape.h"

bool intersect_scene(const Ray &ray, const Scene &scene, Hit &hit);

#endif //GRAPHIC_CW_SCENEUTILS_H