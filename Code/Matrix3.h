//
// Created by 31934 on 2025/10/31.
//

#ifndef GRAPHIC_MATRIX3_H
#define GRAPHIC_MATRIX3_H
#pragma once
#include "Vector3.h"
#include <cmath>

struct Matrix3 {
    double m[3][3];
    Matrix3(){ for(int i=0;i<3;i++) for(int j=0;j<3;j++) m[i][j]= (i==j?1.0:0.0); }
    Vector3 mul(const Vector3 &v) const {
        return {
            m[0][0]*v.x + m[0][1]*v.y + m[0][2]*v.z,
            m[1][0]*v.x + m[1][1]*v.y + m[1][2]*v.z,
            m[2][0]*v.x + m[2][1]*v.y + m[2][2]*v.z
        };
    }
    Matrix3 transpose() const {
        Matrix3 r;
        for(int i=0;i<3;i++) for(int j=0;j<3;j++) r.m[i][j]=m[j][i];
        return r;
    }
    static Matrix3 from_euler(double rx, double ry, double rz) {
        double cx = cos(rx), sx = sin(rx);
        double cy = cos(ry), sy = sin(ry);
        double cz = cos(rz), sz = sin(rz);
        // R = Rz * Ry * Rx
        Matrix3 Rx, Ry, Rz;
        Rx.m[0][0]=1; Rx.m[0][1]=0;  Rx.m[0][2]=0;
        Rx.m[1][0]=0; Rx.m[1][1]=cx; Rx.m[1][2]=-sx;
        Rx.m[2][0]=0; Rx.m[2][1]=sx; Rx.m[2][2]=cx;
        Ry.m[0][0]=cy; Ry.m[0][1]=0; Ry.m[0][2]=sy;
        Ry.m[1][0]=0;  Ry.m[1][1]=1; Ry.m[1][2]=0;
        Ry.m[2][0]=-sy;Ry.m[2][1]=0; Ry.m[2][2]=cy;
        Rz.m[0][0]=cz; Rz.m[0][1]=-sz; Rz.m[0][2]=0;
        Rz.m[1][0]=sz; Rz.m[1][1]=cz;  Rz.m[1][2]=0;
        Rz.m[2][0]=0;  Rz.m[2][1]=0;   Rz.m[2][2]=1;
        Matrix3 tmp;
        // tmp = Rz * Ry
        for(int i=0;i<3;i++) for(int j=0;j<3;j++){
            tmp.m[i][j]=0;
            for(int k=0;k<3;k++) tmp.m[i][j]+= Rz.m[i][k]*Ry.m[k][j];
        }
        Matrix3 R;
        // R = tmp * Rx
        for(int i=0;i<3;i++) for(int j=0;j<3;j++){
            R.m[i][j]=0;
            for(int k=0;k<3;k++) R.m[i][j]+= tmp.m[i][k]*Rx.m[k][j];
        }
        return R;
    }
};

#endif //GRAPHIC_MATRIX3_H