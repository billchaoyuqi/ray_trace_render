//
// Created by 31934 on 2025/10/31.
//

#ifndef GRAPHIC_PLANE_H
#define GRAPHIC_PLANE_H
#pragma once
#include "Shape.h"
#include <array>

class Plane : public Shape {
public:
    std::array<Vector3,4> corners;
    Plane() {}
    virtual bool intersect(const Ray &r, Hit &h) const override;
    virtual void bounds(Vector3 &bmin, Vector3 &bmax) const override;
};

#endif //GRAPHIC_PLANE_H