#include "Camera.h"
#include "Scene.h"
#include "BVH.h"
#include "Image.h"
#include <iostream>
#include <chrono>
#include <filesystem>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <functional>
#include <random>
#include <omp.h>
#include "SceneUtils.h"
// 在 #include 部分添加
#include <bemapiset.h>

#include "Sampling.h"

// 线程局部随机数生成器定义
thread_local std::mt19937 g_rng(std::random_device{}());

using namespace std;
namespace fs = std::filesystem;

const int MAX_DEPTH = 5;

// ====================== 光照函数 ======================
// 与你原来相同：shade 仅依赖 Hit 与 scene（不负责反射/折射）
// Vector3 shade(const Hit &hit, const Scene &scene) {
//     if (!hit.hit) return {0, 0, 0};
//
//     Vector3 base_color = hit.texture ?
//         hit.texture->sample_uv(hit.u, hit.v) :
//         hit.color;
//
//     Vector3 color = scene.ambient_light * base_color;
//
//     for (const auto &light : scene.lights) {
//
//         Vector3 L = (light.pos - hit.pos).normalized();
//
//         // ★ 正确的视角方向：camera − hit
//         //   （你可以用 scene.camera 的位置）
//         Vector3 V = (scene.camera->position - hit.pos).normalized();
//
//         // 半角向量
//         Vector3 H = (L + V).normalized();
//
//         // 漫反射
//         double diff = max(0.0, hit.normal.dot(L));
//
//         // 高光（Blinn-Phong）
//         double spec = pow(
//             max(0.0, hit.normal.dot(H)),
//             hit.material.shininess
//         );
//
//         // 阴影检测（保持你原来的方式）
//         Ray shadow_ray(hit.pos + hit.normal * 1e-4, L);
//         Hit shadow_hit;
//         if (intersect_scene(shadow_ray, scene, shadow_hit) &&
//             shadow_hit.t < (light.pos - hit.pos).length()) {
//             continue;
//             }
//
//         color += base_color * diff * light.intensity;
//         color += Vector3(1,1,1) * spec * light.intensity;
//     }
//
//     return color;
// }

// 辅助函数：检测阴影
bool is_in_shadow(const Ray& ray, const Scene& scene, double maxDistance) {
    Hit hit;
    // 这里使用你已有的相交函数
    return intersect_scene(ray, scene, hit) && hit.t < maxDistance;
}

// 分布式
Vector3 shade(const Hit &hit, const Scene &scene, std::mt19937& rng, int shadowSamples = 4) {
    if (!hit.hit) return {0, 0, 0};

    // 创建局部随机数分布（使用传入的rng）
    std::uniform_real_distribution<> dis(0.0, 1.0);

    Vector3 base_color = hit.texture ?
        hit.texture->sample_uv(hit.u, hit.v) :
        hit.color;

    // 环境光部分保持不变
    Vector3 color = scene.ambient_light * base_color;

    // 对每个光源进行采样
    for (const auto &light : scene.lights) {
        Vector3 directIllumination = {0, 0, 0};
        int validSamples = 0;  // 统计有效光照样本

        // 柔光阴影：对光源进行多次采样
        for (int i = 0; i < shadowSamples; i++) {
            // 采样光源位置
            Vector3 lightSamplePos;

            if (light.radius > 0.0) {
                // 面光源：在圆盘上采样
                double r = sqrt(dis(rng)) * light.radius;
                double theta = 2.0 * M_PI * dis(rng);

                // 简化：假设光源在XY平面
                Vector3 offset(r * cos(theta), r * sin(theta), 0);
                lightSamplePos = light.pos + offset;
            } else {
                // 点光源：为了模拟柔光阴影，添加一点随机偏移
                Vector3 offset(
                    (dis(rng) - 0.5) * 0.05,  // 小范围抖动
                    (dis(rng) - 0.5) * 0.05,
                    (dis(rng) - 0.5) * 0.05
                );
                lightSamplePos = light.pos + offset;
            }

            // 计算从交点指向光源的向量
            Vector3 L = (lightSamplePos - hit.pos).normalized();
            double distanceToLight = (lightSamplePos - hit.pos).length();

            // 视角方向
            Vector3 V = (scene.camera->position - hit.pos).normalized();

            // 半角向量（用于Blinn-Phong高光）
            Vector3 H = (L + V).normalized();

            // 漫反射系数
            double diff = std::max(0.0, hit.normal.dot(L));

            // 高光系数
            double spec = pow(
                std::max(0.0, hit.normal.dot(H)),
                hit.material.shininess
            );

            // 阴影检测
            // bool inShadow = false;
            Ray shadow_ray(hit.pos + hit.normal * 1e-4, L);
            // Hit shadow_hit;
            //
            // // 检查是否有物体遮挡光源
            // if (intersect_scene(shadow_ray, scene, shadow_hit)) {
            //     // 如果相交点比光源近，说明被遮挡
            //     if (shadow_hit.t < distanceToLight - 1e-4) {
            //         inShadow = true;
            //     }
            // }
            // 阴影检测
            bool inShadow = is_in_shadow(shadow_ray, scene, distanceToLight - 1e-4);

            // 如果不在阴影中，计算光照贡献
            if (!inShadow) {
                // 简单光照衰减（可根据需要调整）
                double attenuation = 1.0 / (1.0 + 0.1 * distanceToLight);

                // 漫反射贡献
                Vector3 sampleColor = base_color * diff * light.intensity * attenuation;

                // 高光贡献
                if (spec > 0.0) {
                    sampleColor += Vector3(1, 1, 1) * spec * light.intensity * attenuation;
                }

                directIllumination += sampleColor;
                validSamples++;
            }
        }

        // 平均所有有效样本
        if (validSamples > 0) {
            color += directIllumination * (1.0 / validSamples);
        }

        // 如果没有有效样本（完全在阴影中），可以考虑添加一点环境光
        // 或者保持为0，这取决于你想要的效果
    }

    return color;
}

// ====================== 通用递归 trace 生成器 ======================
// intersect_fn: 函数签名 bool(const Ray&, const Scene&, Hit&)
// 它负责执行一次场景相交（可以是 BVH 或逐对象）
// 返回一个 std::function<Vector3(const Ray&, int)>，该函数执行完整 shading + reflection + refraction
using IntersectFn = std::function<bool(const Ray&, const Scene&, Hit&)>;

std::function<Vector3(const Ray&, int)> make_tracer(const Scene &scene, IntersectFn intersect_fn) {
    // 由于递归 lambda，我们先声明一个 std::function，然后在 lambda 内部捕获并调用它
    std::function<Vector3(const Ray&, int)> trace_fn;
    trace_fn = [&](const Ray &ray, int depth) -> Vector3 {
        if (depth > MAX_DEPTH) return {0,0,0};

        Hit hit;
        if (!intersect_fn(ray, scene, hit)) {
            return scene.background_color;
        }

        std::random_device rd;
        std::mt19937 local_rng(rd());

        Vector3 color = shade(hit, scene, local_rng, 1);

        // 反射
        if (hit.material.reflectivity > 0.0) {
            Vector3 R = ray.dir - hit.normal * 2.0 * ray.dir.dot(hit.normal);
            Ray refl(hit.pos + hit.normal * 1e-4, R.normalized());
            Vector3 refl_color = trace_fn(refl, depth + 1);
            color = color * (1 - hit.material.reflectivity) + refl_color * hit.material.reflectivity;
        }

        // 折射
        if (hit.material.refractivity > 0.0) {
            double eta = hit.material.ior;
            Vector3 N = hit.normal;
            double cosi = -clamp(ray.dir.dot(N), -1.0, 1.0);
            if (cosi < 0) { cosi = -cosi; N = -N; eta = 1.0 / eta; }
            double k = 1 - eta*eta*(1 - cosi*cosi);
            if (k >= 0) {
                Vector3 T = ray.dir * eta + N * (eta*cosi - sqrt(k));
                Ray refr(hit.pos - hit.normal * 1e-4, T.normalized());
                Vector3 refr_color = trace_fn(refr, depth + 1);
                color = color * (1 - hit.material.refractivity) + refr_color * hit.material.refractivity;
            }
        }

        return color;
    };

    return trace_fn;
}

// ====================== 分布式追踪器生成器 ======================
std::function<Vector3(const Ray&, int, std::mt19937&)>
make_distributed_tracer(const Scene &scene, IntersectFn intersect_fn, int shadowSamples = 4) {

    std::function<Vector3(const Ray&, int, std::mt19937&)> trace_fn;
    trace_fn = [&](const Ray &ray, int depth, std::mt19937& rng) -> Vector3 {
        if (depth > MAX_DEPTH) return {0,0,0};

        Hit hit;
        if (!intersect_fn(ray, scene, hit)) {
            return scene.background_color;
        }

        // 使用带柔光阴影的shade函数
        Vector3 color = shade(hit, scene, rng, shadowSamples);

        // 反射（暂时保持原来的镜面反射）
        if (hit.material.reflectivity > 0.0) {
            Vector3 R = ray.dir - hit.normal * 2.0 * ray.dir.dot(hit.normal);
            Ray refl(hit.pos + hit.normal * 1e-4, R.normalized());
            Vector3 refl_color = trace_fn(refl, depth + 1, rng);
            color = color * (1 - hit.material.reflectivity) + refl_color * hit.material.reflectivity;
        }

        // 折射（保持不变）
        if (hit.material.refractivity > 0.0) {
            double eta = hit.material.ior;
            Vector3 N = hit.normal;
            double cosi = -clamp(ray.dir.dot(N), -1.0, 1.0);
            if (cosi < 0) { cosi = -cosi; N = -N; eta = 1.0 / eta; }
            double k = 1 - eta*eta*(1 - cosi*cosi);
            if (k >= 0) {
                Vector3 T = ray.dir * eta + N * (eta*cosi - sqrt(k));
                Ray refr(hit.pos - hit.normal * 1e-4, T.normalized());
                Vector3 refr_color = trace_fn(refr, depth + 1, rng);
                color = color * (1 - hit.material.refractivity) + refr_color * hit.material.refractivity;
            }
        }

        return color;
    };

    return trace_fn;
}

// ====================== 分布式渲染函数（柔光阴影） ======================
void render_distributed_soft_shadows(const Camera &cam, const Scene &scene, Image &img,
                                     bool use_bvh = true, int pixelSamples = 16, int shadowSamples = 8) {

    // 创建BVH对象（如果需要）
    std::unique_ptr<BVH> bvh_ptr;
    if (use_bvh) {
        bvh_ptr = std::make_unique<BVH>();
        bvh_ptr->build(scene);
    }

    IntersectFn intersect_fn;
    if (use_bvh) {
        BVH* bvh = bvh_ptr.get();
        intersect_fn = [bvh](const Ray &r, const Scene &s, Hit &h) -> bool {
            return bvh->intersect(r, h, s);
        };
    } else {
        intersect_fn = [](const Ray &r, const Scene &s, Hit &h) -> bool {
            return intersect_scene(r, s, h);
        };
    }

    // 创建分布式追踪器
    auto tracer = make_distributed_tracer(scene, intersect_fn, shadowSamples);

#pragma omp parallel for schedule(dynamic, 4)
    for (int y = 0; y < cam.res_y; y++) {
        // 每个线程自己的随机数生成器
        std::random_device rd;
        std::mt19937 rng(rd() + omp_get_thread_num());
        std::uniform_real_distribution<> dis(0.0, 1.0);

        for (int x = 0; x < cam.res_x; x++) {
            Vector3 color_sum{0,0,0};

            for (int s = 0; s < pixelSamples; s++) {
                double dx = dis(rng);
                double dy = dis(rng);

                Ray ray = cam.pixel_to_ray(x + 0.5 + dx, y + 0.5 + dy);
                color_sum += tracer(ray, 0, rng);
            }

            Vector3 color = color_sum * (1.0 / pixelSamples);
            img.set_pixel(x, y, color);
        }

#pragma omp critical
        {
            if (y % 50 == 0)
                cout << "[Distributed Soft Shadows] row " << y << "/" << cam.res_y
                     << " (shadowSamples=" << shadowSamples << ")" << endl;
        }
    }
}

// ====================== 渲染（不使用 BVH） ======================
void render_no_bvh(const Camera &cam, const Scene &scene, Image &img) {
    const int SAMPLES = 16;

    // intersect_fn 使用原有的直接场景相交函数
    IntersectFn intersect_fn = [](const Ray &r, const Scene &s, Hit &h) -> bool {
        return intersect_scene(r, s, h);
    };

    auto tracer = make_tracer(scene, intersect_fn);

    #pragma omp parallel for schedule(dynamic, 4)
    for (int y = 0; y < cam.res_y; y++) {
        // 每个线程有随机数生成器
        std::random_device rd;
        std::mt19937 gen(rd() + omp_get_thread_num());
        std::uniform_real_distribution<> dis(0.0, 1.0);

        for (int x = 0; x < cam.res_x; x++) {
            Vector3 color_sum{0,0,0};
            for (int s = 0; s < SAMPLES; s++) {
                double dx = dis(gen);
                double dy = dis(gen);
                Ray ray = cam.pixel_to_ray(x + 0.5 + dx, y + 0.5 + dy);
                color_sum += tracer(ray, 0);
            }
            Vector3 color = color_sum * (1.0 / SAMPLES);
            img.set_pixel(x, y, color);
        }

        #pragma omp critical
        {
            if (y % 50 == 0)
                cout << "[No BVH Parallel] row " << y << "/" << cam.res_y << endl;
        }
    }
}

// ====================== 渲染（使用 BVH） ======================
void render_bvh(const Camera &cam, const Scene &scene, Image &img) {
    BVH bvh;
    bvh.build(scene);

    const int SAMPLES = 16;

    // intersect_fn 使用 BVH 的相交接口
    IntersectFn intersect_fn = [&bvh](const Ray &r, const Scene &s, Hit &h) -> bool {
        // 假定你的 BVH::intersect 接口是: bool intersect(const Ray&, Hit&, const Scene&)
        // 但你原先在 lambda 中调用的是 bvh.intersect(r, h, scene)
        // 如果签名不同，请根据你的 BVH 接口调整这一行顺序或参数。
        return bvh.intersect(r, h, s);
    };

    auto tracer = make_tracer(scene, intersect_fn);

    #pragma omp parallel for schedule(dynamic, 4)
    for (int y = 0; y < cam.res_y; y++) {
        std::random_device rd;
        std::mt19937 gen(rd() + omp_get_thread_num());
        std::uniform_real_distribution<> dis(0.0, 1.0);

        for (int x = 0; x < cam.res_x; x++) {
            Vector3 color_sum{0,0,0};
            for (int s = 0; s < SAMPLES; s++) {
                double dx = dis(gen);
                double dy = dis(gen);
                Ray ray = cam.pixel_to_ray(x + 0.5 + dx, y + 0.5 + dy);
                color_sum += tracer(ray, 0);
            }
            Vector3 color = color_sum * (1.0 / SAMPLES);
            img.set_pixel(x, y, color);
        }

        #pragma omp critical
        {
            if (y % 50 == 0)
                cout << "[BVH Parallel] row " << y << "/" << cam.res_y << endl;
        }
    }
}

// ====================== 渲染使用特效 ======================
void render_with_effects(const Camera &cam, const Scene &scene, Image &img, bool use_bvh = true) {
    const int SAMPLES = 16;

    // 创建BVH对象（如果需要）
    std::unique_ptr<BVH> bvh_ptr;
    if (use_bvh) {
        bvh_ptr = std::make_unique<BVH>();
        bvh_ptr->build(scene);
    }

    IntersectFn intersect_fn;
    if (use_bvh) {
        BVH* bvh = bvh_ptr.get();
        intersect_fn = [bvh](const Ray &r, const Scene &s, Hit &h) -> bool {
            return bvh->intersect(r, h, s);
        };
    } else {
        intersect_fn = [](const Ray &r, const Scene &s, Hit &h) -> bool {
            return intersect_scene(r, s, h);
        };
    }

    auto tracer = make_tracer(scene, intersect_fn);

    // 计算透镜半径
    Camera& mutable_cam = const_cast<Camera&>(cam);
    mutable_cam.compute_lens_radius();

    #pragma omp parallel for schedule(dynamic, 4)
    for (int y = 0; y < cam.res_y; y++) {
        std::random_device rd;
        std::mt19937 gen(rd() + omp_get_thread_num());
        std::uniform_real_distribution<> dis(0.0, 1.0);

        for (int x = 0; x < cam.res_x; x++) {
            Vector3 color_sum{0,0,0};

            for (int s = 0; s < SAMPLES; s++) {
                double dx = dis(gen);
                double dy = dis(gen);

                // 生成效果参数
                double time_offset = 0.0;
                if (cam.shutter_speed > 0.0) {
                    std::uniform_real_distribution<> time_dis(0.0, cam.shutter_speed);
                    time_offset = time_dis(gen);
                }

                Vector3 lens_pos = cam.position;
                if (cam.lens_radius_m > 0.0) {
                    lens_pos = cam.sample_lens_position(gen);
                }

                Ray ray = cam.pixel_to_ray_with_effects(
                    x + 0.5 + dx, y + 0.5 + dy,
                    time_offset, lens_pos
                );

                color_sum += tracer(ray, 0);
            }

            Vector3 color = color_sum * (1.0 / SAMPLES);
            img.set_pixel(x, y, color);
        }
    }
}

// ====================== 分布式渲染 + 动态模糊函数 ======================
void render_distributed_with_motion_blur(const Camera &cam, const Scene &scene, Image &img,
                                         bool use_bvh = true, int pixelSamples = 16,
                                         int shadowSamples = 8) {

    // 创建BVH对象（如果需要）
    std::unique_ptr<BVH> bvh_ptr;
    if (use_bvh) {
        bvh_ptr = std::make_unique<BVH>();
        bvh_ptr->build(scene);
    }

    IntersectFn intersect_fn;
    if (use_bvh) {
        BVH* bvh = bvh_ptr.get();
        intersect_fn = [bvh](const Ray &r, const Scene &s, Hit &h) -> bool {
            return bvh->intersect(r, h, s);
        };
    } else {
        intersect_fn = [](const Ray &r, const Scene &s, Hit &h) -> bool {
            return intersect_scene(r, s, h);
        };
    }

    // 创建分布式追踪器
    auto tracer = make_distributed_tracer(scene, intersect_fn, shadowSamples);

    // 计算透镜半径（如果需要）
    Camera& mutable_cam = const_cast<Camera&>(cam);
    mutable_cam.compute_lens_radius();

#pragma omp parallel for schedule(dynamic, 4)
    for (int y = 0; y < cam.res_y; y++) {
        // 每个线程自己的随机数生成器
        std::random_device rd;
        std::mt19937 rng(rd() + omp_get_thread_num());
        std::uniform_real_distribution<> dis(0.0, 1.0);

        for (int x = 0; x < cam.res_x; x++) {
            Vector3 color_sum{0,0,0};

            for (int s = 0; s < pixelSamples; s++) {
                double dx = dis(rng);
                double dy = dis(rng);

                // 生成动态模糊的时间偏移
                double time_offset = 0.0;
                if (cam.shutter_speed > 0.0) {
                    std::uniform_real_distribution<> time_dis(0.0, cam.shutter_speed);
                    time_offset = time_dis(rng);
                }

                // 生成景深效果的光线起点
                Vector3 lens_pos = cam.position;
                if (cam.lens_radius_m > 0.0) {
                    lens_pos = cam.sample_lens_position(rng);
                }

                // 生成带有特效的光线
                Ray ray = cam.pixel_to_ray_with_effects(
                    x + 0.5 + dx, y + 0.5 + dy,
                    time_offset, lens_pos
                );

                // 使用分布式追踪器追踪光线
                color_sum += tracer(ray, 0, rng);
            }

            Vector3 color = color_sum * (1.0 / pixelSamples);
            img.set_pixel(x, y, color);
        }

#pragma omp critical
        {
            if (y % 50 == 0)
                cout << "[Distributed + Motion Blur] row " << y << "/" << cam.res_y
                     << " (samples=" << pixelSamples << ")" << endl;
        }
    }
}

// ====================== Main ======================
int main(int argc, char* argv[]) {
    try {
        // 默认参数设置
        bool use_bvh = true;
        bool use_motion_blur = false;
        bool use_distributed = false;
        int shadow_samples = 4;  // 分布式渲染的阴影采样数
        int pixel_samples = 16;  // 每个像素的采样数

        // 解析命令行参数
        for (int i = 1; i < argc; i++) {
            std::string arg = argv[i];

            if (arg == "--no-bvh") {
                use_bvh = false;
                std::cout << "BVH disabled" << std::endl;
            }
            else if (arg == "--bvh") {
                use_bvh = true;
                std::cout << "BVH enabled" << std::endl;
            }
            else if (arg == "--motion-blur" || arg == "--mb") {
                use_motion_blur = true;
                std::cout << "Motion blur enabled" << std::endl;
            }
            else if (arg == "--distributed" || arg == "--dist") {
                use_distributed = true;
                std::cout << "Distributed rendering enabled" << std::endl;
            }
            else if (arg == "--shadow-samples" && i + 1 < argc) {
                shadow_samples = std::stoi(argv[++i]);
                std::cout << "Shadow samples: " << shadow_samples << std::endl;
            }
            else if (arg == "--help" || arg == "-h") {
                std::cout << "Usage: " << argv[0] << " [options]\n"
                          << "Options:\n"
                          << "  --bvh / --no-bvh     Enable/disable BVH acceleration\n"
                          << "  --motion-blur        Enable motion blur effects\n"
                          << "  --distributed        Enable distributed rendering with soft shadows\n"
                          << "  --shadow-samples N   Number of shadow samples (default: 4)\n"
                          << "  --help               Show this help message\n";
                return 0;
            }
            else {
                std::cerr << "Unknown argument: " << arg << std::endl;
                std::cerr << "Use --help for usage information" << std::endl;
                return 1;
            }
        }

        const string input_path  = "../ASCII/scene.txt";
        fs::create_directories("../Output");

        cout << "Loading scene: " << input_path << " ..." << endl;
        Scene scene = load_scene_txt(input_path);

        if (!scene.camera) {
            cerr << "Error: No camera in scene file." << endl;
            return -1;
        }

        Camera cam = *scene.camera;
        cam.compute_basis();
        cout << "Camera loaded: " << cam.name << " (" << cam.res_x << "x" << cam.res_y << ")" << endl;

        // 检查相机是否支持动态模糊
        bool camera_supports_motion_blur = (cam.shutter_speed > 0.0);
        if (!camera_supports_motion_blur && (use_motion_blur)) {
            cout << "Warning: Camera does not support motion blur (shutter_speed = 0)" << endl;
            cout << "Motion blur will have no effect" << endl;
        }

        srand((unsigned int)time(nullptr));

        Image img(cam.res_x, cam.res_y);
        std::string output_filename;
        std::string description;

        auto start_time = chrono::high_resolution_clock::now();

        // 根据命令行参数选择合适的渲染方式
        if (use_motion_blur && use_distributed) {
            description = "Combined Distributed + Motion Blur";
            output_filename = "../Output/output_combined";
            output_filename += (use_bvh ? "_bvh" : "_nobvh");
            output_filename += "_ss" + std::to_string(shadow_samples);
            output_filename += "_ps" + std::to_string(pixel_samples);
            output_filename += ".ppm";

            cout << "\n=== " << description << " ===" << endl;
            cout << "BVH: " << (use_bvh ? "enabled" : "disabled") << endl;
            cout << "Pixel samples: " << pixel_samples << endl;
            cout << "Shadow samples: " << shadow_samples << endl;
            cout << "Motion blur: " << (camera_supports_motion_blur ? "supported" : "not supported") << endl;

            render_distributed_with_motion_blur(cam, scene, img, use_bvh, pixel_samples, shadow_samples);
        }
        else if (use_distributed) {
            description = "Distributed Soft Shadows";
            output_filename = "../Output/output_distributed";
            output_filename += (use_bvh ? "_bvh" : "_nobvh");
            output_filename += "_ss" + std::to_string(shadow_samples);
            output_filename += "_ps" + std::to_string(pixel_samples);
            output_filename += ".ppm";

            cout << "\n=== " << description << " ===" << endl;
            cout << "BVH: " << (use_bvh ? "enabled" : "disabled") << endl;
            cout << "Pixel samples: " << pixel_samples << endl;
            cout << "Shadow samples: " << shadow_samples << endl;

            render_distributed_soft_shadows(cam, scene, img, use_bvh, pixel_samples, shadow_samples);
        }
        else if (use_motion_blur) {
            description = "Motion Blur Effects";
            output_filename = "../Output/output_effects";
            output_filename += (use_bvh ? "_bvh" : "_nobvh");
            output_filename += ".ppm";

            cout << "\n=== " << description << " ===" << endl;
            cout << "BVH: " << (use_bvh ? "enabled" : "disabled") << endl;
            cout << "Motion blur: " << (camera_supports_motion_blur ? "supported" : "not supported") << endl;

            render_with_effects(cam, scene, img, use_bvh);
        }
        else if (use_bvh) {
            description = "Standard with BVH";
            output_filename = "../Output/output_bvh.ppm";

            cout << "\n=== " << description << " ===" << endl;
            render_bvh(cam, scene, img);
        }
        else {
            description = "Standard without BVH";
            output_filename = "../Output/output_no_bvh.ppm";

            cout << "\n=== " << description << " ===" << endl;
            render_no_bvh(cam, scene, img);
        }

        auto end_time = chrono::high_resolution_clock::now();

        // 保存图像
        img.write_ppm(output_filename);
        cout << "\n=== Render Complete ===" << endl;
        cout << "Mode: " << description << endl;
        cout << "Output: " << output_filename << endl;
        cout << "Time: " << chrono::duration<double>(end_time - start_time).count() << " seconds" << endl;

    } catch (const exception &e) {
        cerr << "Error: " << e.what() << endl;
        return -1;
    }

    return 0;
}
