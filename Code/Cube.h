//
// Created by 31934 on 2025/10/31.
//

#ifndef GRAPHIC_CUBE_H
#define GRAPHIC_CUBE_H
#pragma once
#include "Shape.h"
#include "Matrix3.h"
#include <algorithm>

class Cube : public Shape {
public:
    Vector3 center;
    Vector3 size;      // 长方体的尺寸 (width, height, depth)
    Matrix3 rot;       // world rotation (object->world)
    Matrix3 rot_inv;   // transpose

    Cube() : size(1.0, 1.0, 1.0) {
        rot = Matrix3();
        rot_inv = rot.transpose();
    }

    // 构造函数：指定尺寸
    Cube(const Vector3& center_, const Vector3& size_)
        : center(center_), size(size_) {
        rot = Matrix3();
        rot_inv = rot.transpose();
    }

    // 构造函数：指定尺寸（标量，创建立方体）
    Cube(const Vector3& center_, double uniform_size)
        : center(center_), size(uniform_size, uniform_size, uniform_size) {
        rot = Matrix3();
        rot_inv = rot.transpose();
    }

    virtual bool intersect(const Ray &r, Hit &h) const override;
    virtual void bounds(Vector3 &bmin, Vector3 &bmax) const override;

    // 设置旋转（Euler angles，单位：度）
    void set_rotation(double rx_deg, double ry_deg, double rz_deg);

    // 设置统一尺寸（创建立方体）
    void set_uniform_scale(double scale) {
        size = Vector3(scale, scale, scale);
    }

    // 设置长方体尺寸
    void set_size(double width, double height, double depth) {
        size = Vector3(width, height, depth);
    }

    // 获取半尺寸（用于相交检测） —— 仍然返回真实半尺寸
    Vector3 half_size() const {
        return size * 0.5;
    }

    // 获取最小和最大顶点（局部坐标系）
    Vector3 local_min() const {
        return -half_size();
    }

    Vector3 local_max() const {
        return half_size();
    }

    // 将点从世界坐标系转换到局部坐标系
    Vector3 world_to_local(const Vector3& world_point) const {
        Vector3 local = rot_inv.mul(world_point - center);
        // 考虑非均匀缩放：把世界坐标除以 size => 得到局部坐标（范围[-0.5,0.5]）
        local.x /= size.x;
        local.y /= size.y;
        local.z /= size.z;
        return local;
    }

    // 将点从局部坐标系转换到世界坐标系
    Vector3 local_to_world(const Vector3& local_point) const {
        Vector3 scaled_point(
            local_point.x * size.x,
            local_point.y * size.y,
            local_point.z * size.z
        );
        return center + rot.mul(scaled_point);
    }

    // 获取表面法线（在局部坐标系中）
    // 注意：local_point 此处假定是在标准局部坐标 [-0.5,0.5]
    Vector3 compute_local_normal(const Vector3& local_point) const {
        Vector3 half(0.5, 0.5, 0.5); // 应当在局部归一化坐标下使用 0.5
        // 计算到各面的距离
        double dist_to_min_x = std::abs(local_point.x - (-half.x));
        double dist_to_max_x = std::abs(local_point.x - half.x);
        double dist_to_min_y = std::abs(local_point.y - (-half.y));
        double dist_to_max_y = std::abs(local_point.y - half.y);
        double dist_to_min_z = std::abs(local_point.z - (-half.z));
        double dist_to_max_z = std::abs(local_point.z - half.z);

        // 找到最小距离
        double min_dist = dist_to_min_x;
        Vector3 normal(1, 0, 0);

        if (dist_to_max_x < min_dist) {
            min_dist = dist_to_max_x;
            normal = Vector3(-1, 0, 0);
        }
        if (dist_to_min_y < min_dist) {
            min_dist = dist_to_min_y;
            normal = Vector3(0, 1, 0);
        }
        if (dist_to_max_y < min_dist) {
            min_dist = dist_to_max_y;
            normal = Vector3(0, -1, 0);
        }
        if (dist_to_min_z < min_dist) {
            min_dist = dist_to_min_z;
            normal = Vector3(0, 0, 1);
        }
        if (dist_to_max_z < min_dist) {
            normal = Vector3(0, 0, -1);
        }

        return normal;
    }
};

#endif //GRAPHIC_CUBE_H
