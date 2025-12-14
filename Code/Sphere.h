//
// Created by 31934 on 2025/10/31.
//

#ifndef GRAPHIC_SPHERE_H
#define GRAPHIC_SPHERE_H
#pragma once
#include "Shape.h"

class Sphere : public Shape {
public:
    Vector3 center;
    double radius;
    Sphere(const Vector3 &c={0,0,0}, double r=1.0):center(c),radius(r){}
    virtual bool intersect(const Ray &r, Hit &h) const override;
    virtual void bounds(Vector3 &bmin, Vector3 &bmax) const override;
};

#endif //GRAPHIC_SPHERE_H