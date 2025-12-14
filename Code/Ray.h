//
// Created by 31934 on 2025/10/15.
//

#ifndef CWPROJECT_RAY_H
#define CWPROJECT_RAY_H

#pragma once
#include "Vector3.h"
struct Ray
{
    Vector3 origin;
    Vector3 dir;
    Ray(){}
    Ray(const Vector3 &o,const Vector3 &d):origin(o),dir(d){}
};

#endif //CWPROJECT_RAY_H