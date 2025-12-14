//
// Created by 31934 on 2025/10/15.
//

#ifndef CWPROJECT_IMAGE_H
#define CWPROJECT_IMAGE_H
#pragma once
#include <string>
#include <vector>
#include "Vector3.h"

struct Color { double r,g,b; Color(double r_=0,double g_=0,double b_=0):r(r_),g(g_),b(b_){} };

class Image {
public:
    int width=0, height=0;
    std::vector<Color> pixels;
    Image() {}
    Image(int w,int h):width(w),height(h),pixels(w*h, Color(0,0,0)){}
    void set(int x,int y,const Color &c){ if(x<0||x>=width||y<0||y>=height) return; pixels[y*width+x]=c; }
    Color get(int x,int y) const { if(x<0||x>=width||y<0||y>=height) return Color(); return pixels[y*width+x]; }
    void write_ppm(const std::string &filename) const;
    void set_pixel(int x, int y, const Vector3 &color);
    Vector3 get_pixel(int x, int y) const;
    // 纹理处理
    bool load_ppm(const std::string &filename);      // 读取 P3 ASCII PPM
    Vector3 sample_uv(double u, double v) const;     // 以 u,v (0..1) 取得颜色（最近邻）
};


#endif //CWPROJECT_IMAGE_H