//
// Created by 31934 on 2025/10/15.
//

#ifndef CWPROJECT_CAMERA_H
#define CWPROJECT_CAMERA_H
#pragma once
#include <string>
#include "Vector3.h"
#include "Ray.h"
#include <random>

class Camera {
public:
    std::string name;
    Vector3 position;
    Vector3 gaze;
    double focal_length_m = 0.05;  // meters
    double sensor_w_m = 0.036;
    double sensor_h_m = 0.024;
    int res_x = 800;
    int res_y = 600;

    // ✅ 运动模糊参数
    double shutter_speed = 0.0;        // 快门开启时间（秒），0表示无运动模糊
    Vector3 velocity = {0, 0, 0};      // 相机运动速度（米/秒）

    // ✅ 景深参数
    double aperture_fstop = 0.0;       // 光圈F值（如f/2.8），0表示无景深
    double focus_distance_m = 5.0;     // 对焦距离（米）
    double lens_radius_m = 0.0;        // 透镜半径（米），根据光圈计算

    // ✅ 新增带参数构造函数
    Camera() = default; // 默认构造函数
    Camera(const Vector3 &pos, const Vector3 &gaze_,
           double f, double sw, double sh, int rx, int ry)
        : position(pos), gaze(gaze_),
          focal_length_m(f), sensor_w_m(sw), sensor_h_m(sh),
          res_x(rx), res_y(ry)
    {
        compute_basis();
    }

    static Camera from_txt(const std::string &filename, int index = 0);
    // ✅ 基础射线生成（无特效）
    Ray pixel_to_ray(double px, double py) const;

    // ✅ 带特效的射线生成
    Ray pixel_to_ray_with_effects(double px, double py, double time_offset, const Vector3& lens_pos) const;

    void compute_basis();

    // ✅ 计算透镜半径（基于光圈F值）
    void compute_lens_radius();

    // ✅ 生成时间偏移（用于运动模糊）
    double get_time_offset(std::mt19937& gen) const;

    // ✅ 生成透镜位置采样（用于景深）
    Vector3 sample_lens_position(std::mt19937& gen) const;


private:
    Vector3 right, up, forward;

};

#endif //CWPROJECT_CAMERA_H
