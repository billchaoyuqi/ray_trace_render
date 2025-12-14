#include "Camera.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <cmath>

Camera Camera::from_txt(const std::string &filename, int index) {
    std::ifstream in(filename);
    if (!in) throw std::runtime_error("Cannot open " + filename);

    std::string line;
    int cam_count = -1;
    Camera cam;

    while (std::getline(in, line)) {
        if (line.rfind("Camera ", 0) == 0) {
            cam_count++;
            cam.name = line.substr(7);
        } else if (cam_count == index) {
            std::istringstream iss(line);
            std::string tag;
            iss >> tag;
            if (tag == "location")
                iss >> cam.position.x >> cam.position.y >> cam.position.z;
            else if (tag == "gaze")
                iss >> cam.gaze.x >> cam.gaze.y >> cam.gaze.z;
            else if (tag == "focal_length") {
                double fmm; iss >> fmm; cam.focal_length_m = fmm / 1000.0;
            }
            else if (tag == "sensor_width") {
                double mm; iss >> mm; cam.sensor_w_m = mm / 1000.0;
            }
            else if (tag == "sensor_height") {
                double mm; iss >> mm; cam.sensor_h_m = mm / 1000.0;
            }
            else if (tag == "resolution") {
                iss >> cam.res_x >> cam.res_y;
            }
            // ✅ 新增：运动模糊参数
            else if (tag == "shutter_speed") {
                iss >> cam.shutter_speed;
            }
            else if (tag == "camera_velocity") {
                iss >> cam.velocity.x >> cam.velocity.y >> cam.velocity.z;
            }
            // ✅ 新增：景深参数
            else if (tag == "aperture") {
                iss >> cam.aperture_fstop;
            }
            else if (tag == "focus_distance") {
                double dist_mm; iss >> dist_mm; cam.focus_distance_m = dist_mm / 1000.0;
            }
            else if (tag == "end") break;
        }
    }
    cam.compute_basis();
    return cam;
}

void Camera::compute_basis() {
    forward = gaze.normalized();
    Vector3 world_up(0, 0, 1);
    if (std::abs(forward.dot(world_up)) > 0.999)
        world_up = Vector3(0, 1, 0);
    right = forward.cross(world_up).normalized();
    up = right.cross(forward).normalized();
}

Ray Camera::pixel_to_ray(double px, double py) const {
    double ndc_x = (px + 0.5) / res_x - 0.5;
    double ndc_y = 0.5 - (py + 0.5) / res_y;
    double sx = ndc_x * sensor_w_m;
    double sy = ndc_y * sensor_h_m;
    Vector3 dir = (forward * focal_length_m + right * sx + up * sy).normalized();
    return Ray(position, dir);
}

Ray Camera::pixel_to_ray_with_effects(double px, double py, double time_offset, const Vector3& lens_pos) const {
    // 计算像素在传感器上的位置（使用你原有的NDC计算方法）
    double ndc_x = (px + 0.5) / res_x - 0.5;
    double ndc_y = 0.5 - (py + 0.5) / res_y;
    double sensor_x = ndc_x * sensor_w_m;
    double sensor_y = ndc_y * sensor_h_m;

    // 传感器平面位于相机前方 focal_length_m 处
    Vector3 sensor_point = forward * focal_length_m + right * sensor_x + up * sensor_y;

    // 应用运动模糊：移动相机位置
    Vector3 cam_pos = position;
    if (time_offset != 0.0 && velocity.length() > 0.0) {
        cam_pos = cam_pos + velocity * time_offset;
    }

    Vector3 ray_direction;
    Vector3 ray_origin;

    // 应用景深效果 - 修复 operator== 问题
    bool using_dof = (lens_radius_m > 0.0);
    if (using_dof) {
        // 检查透镜位置是否与相机位置相同（不使用景深）
        bool lens_at_center = (lens_pos.x == position.x &&
                              lens_pos.y == position.y &&
                              lens_pos.z == position.z);
        if (lens_at_center) {
            using_dof = false;
        }
    }

    if (using_dof) {
        // 计算焦点平面上的点
        // 从原始相机位置出发，计算与焦点平面的交点
        Vector3 original_dir = sensor_point.normalized();

        // 焦点平面是垂直于视线方向，距离为 focus_distance_m 的平面
        // 计算射线与焦点平面的交点
        double t = focus_distance_m / original_dir.dot(forward);
        Vector3 focus_point = cam_pos + original_dir * t;  // 使用运动模糊后的位置

        // 从透镜位置指向焦点
        ray_direction = (focus_point - lens_pos).normalized();
        ray_origin = lens_pos;
    } else {
        // 无景深：直接使用传感器方向
        ray_direction = sensor_point.normalized();
        ray_origin = cam_pos;  // 使用运动模糊后的位置
    }

    return Ray(ray_origin, ray_direction);
}

void Camera::compute_lens_radius() {
    if (aperture_fstop <= 0.0) {
        lens_radius_m = 0.0;
    } else {
        // 透镜半径 = 焦距 / (2 * F值)
        lens_radius_m = focal_length_m / (2.0 * aperture_fstop);
    }
}

double Camera::get_time_offset(std::mt19937& gen) const {
    if (shutter_speed <= 0.0) return 0.0;
    std::uniform_real_distribution<> time_dis(0.0, shutter_speed);
    return time_dis(gen);
}

Vector3 Camera::sample_lens_position(std::mt19937& gen) const {
    if (lens_radius_m <= 0.0) return position;

    // 在圆盘内均匀采样
    std::uniform_real_distribution<> radius_dis(0.0, lens_radius_m);
    std::uniform_real_distribution<> angle_dis(0.0, 2.0 * M_PI);

    double r = radius_dis(gen);
    double theta = angle_dis(gen);

    // 在透镜平面内随机采样（使用相机坐标系）
    Vector3 lens_offset = right * (r * cos(theta)) + up * (r * sin(theta));
    return position + lens_offset;
}