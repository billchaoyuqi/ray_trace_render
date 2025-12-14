#include "Scene.h"
#include "Image.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <cmath>
#include <algorithm>
#include <unordered_map>
#include <filesystem>

static std::string trim(const std::string &s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    size_t end = s.find_last_not_of(" \t\r\n");
    return (start == std::string::npos) ? "" : s.substr(start, end - start + 1);
}

Scene load_scene_txt(const std::string &filename) {
    Scene scene;
    scene.ambient_light = {0.2, 0.2, 0.2};
    scene.background_color = {0.8, 0.9, 1.0};

    std::ifstream in(filename);
    if (!in.is_open()) throw std::runtime_error("Cannot open scene file: " + filename);

    // 获取场景文件所在目录
    std::string textures_dir;
    std::string scene_dir = filename.substr(0, filename.find_last_of("/\\"));
    // 使用上级目录
    std::filesystem::path scene_path(scene_dir);
    std::filesystem::path parent_path = scene_path.parent_path();
    std::filesystem::path texture_path = parent_path / "Textures";

    try {
        if (std::filesystem::exists(texture_path) && std::filesystem::is_directory(texture_path)) {
            textures_dir = std::filesystem::canonical(texture_path).string();
            //std::cout << "Found textures directory: " << textures_dir << std::endl;
        } else {
            std::cout << "Textures directory not found at: " << texture_path << std::endl;
            textures_dir = scene_dir; // 回退到场景文件目录
        }
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Filesystem error: " << e.what() << std::endl;
        textures_dir = scene_dir;
    }

    std::string line;
    while (std::getline(in, line)) {
        line = trim(line);
        if (line.empty()) continue;
        std::istringstream iss(line);
        std::string token, name;
        iss >> token;

        if (token == "Background") {
            double r, g, b;
            // 因为已经读出了"Background"，现在直接读取后面的数值
            if (iss >> r >> g >> b) {
                scene.background_color = Vector3(r, g, b);
                std::cout << "[DEBUG] Background: " << scene.background_color << std::endl;
            }
            continue; // 跳过name读取
        } else if (token == "AmbientLight") {
            double r, g, b;
            if (iss >> r >> g >> b) {
                scene.ambient_light = Vector3(r, g, b);
                std::cout << "[DEBUG] AmbientLight: " << scene.ambient_light << std::endl;
            }
            continue; // 跳过name读取
        }

        if (!(iss >> name)) {
            std::cerr << "Warning: Missing name for token " << token << "\n";
            while (std::getline(in, line) && line != "end");
            continue;
        }

        if (token == "Camera") {
            Vector3 loc{0,0,0}, gaze{0,0,-1};
            double focal_length=50, sensor_w=36, sensor_h=24;
            int res_x=800, res_y=600;

            // ✅ 新增：运动模糊参数（默认值）
            double shutter_speed = 0.0;
            Vector3 camera_velocity{0, 0, 0};

            // ✅ 新增：景深参数（默认值）
            double aperture_fstop = 0.0;
            double focus_distance = 5000.0; // 默认5米

            while (std::getline(in,line)) {
                line = trim(line);
                if (line == "end") break;
                std::istringstream l(line);
                std::string key; l >> key;
                if (key == "location") l >> loc.x >> loc.y >> loc.z;
                else if (key == "gaze") l >> gaze.x >> gaze.y >> gaze.z;
                else if (key == "focal_length") l >> focal_length;
                else if (key == "sensor_width") l >> sensor_w;
                else if (key == "sensor_height") l >> sensor_h;
                else if (key == "resolution") l >> res_x >> res_y;
                // ✅ 新增：运动模糊参数解析
                else if (key == "shutter_speed") l >> shutter_speed;
                else if (key == "camera_velocity") l >> camera_velocity.x >> camera_velocity.y >> camera_velocity.z;

                // ✅ 新增：景深参数解析
                else if (key == "aperture") l >> aperture_fstop;
                else if (key == "focus_distance") l >> focus_distance;
            }

            auto cam = std::make_shared<Camera>();
            cam->name = name;
            cam->position = loc;
            cam->gaze = gaze;
            cam->focal_length_m = focal_length / 1000.0;
            cam->sensor_w_m = sensor_w / 1000.0;
            cam->sensor_h_m = sensor_h / 1000.0;
            cam->res_x = res_x;
            cam->res_y = res_y;

            // ✅ 设置运动模糊参数
            cam->shutter_speed = shutter_speed;
            cam->velocity = camera_velocity;

            // ✅ 设置景深参数
            cam->aperture_fstop = aperture_fstop;
            cam->focus_distance_m = focus_distance / 1000.0; // 转换为米

            cam->compute_basis();
            cam->compute_lens_radius(); // 计算透镜半径

            scene.camera = cam;
        }
        else if (token == "PointLight") {
            Vector3 loc{0,0,0};
            double intensity=1.0;
            double radius = 0.0;  // 光源半径
            // Vector3 normal{0,0,0}; // 面光源的法线方向
            while (std::getline(in, line)) {
                line = trim(line);
                if (line == "end") break;
                std::istringstream l(line);
                std::string key; l >> key;
                if (key == "location") l >> loc.x >> loc.y >> loc.z;
                else if (key == "intensity") l >> intensity;
                else if (key == "radius") l >> radius;
            }
            scene.lights.push_back({loc, intensity / 1000.0, radius});
        }
        else if (token == "Sphere") {
            Vector3 loc{0,0,0}, color{0.8,0.8,0.8};
            double radius = 1.0;
            std::string color_filename;
            Material material; // 添加材质属性

            while (std::getline(in, line)) {
                line = trim(line);
                if (line == "end") break;
                std::istringstream l(line);
                std::string key; l >> key;
                if (key == "location") l >> loc.x >> loc.y >> loc.z;
                else if (key == "radius") l >> radius;
                else if (key == "color") l >> color.x >> color.y >> color.z;
                else if (key == "texture") l >> color_filename;
                else if (key == "reflectivity") l >> material.reflectivity;
                else if (key == "refractivity") l >> material.refractivity;
                else if (key == "ior") l >> material.ior;
                else if (key == "shininess") l >> material.shininess;
                else if (key == "roughness") l >> material.roughness;
            }
            auto s = std::make_shared<Sphere>(loc, radius);
            s->name = name;
            s->color = color;
            s->texture_file = color_filename;
            s->material = material; // 设置材质
            scene.objects.push_back(s);
        }
        else if (token == "Plane") {
            auto p = std::make_shared<Plane>();
            Vector3 color{0.8,0.8,0.8};
            std::string color_filename;
            Material material; // 添加材质属性

            while (std::getline(in, line)) {
                line = trim(line);
                if (line == "end") break;
                std::istringstream l(line);
                std::string key; l >> key;
                if (key.rfind("corner",0) == 0) {
                    int idx = std::stoi(key.substr(6)) - 1;
                    l >> p->corners[idx].x >> p->corners[idx].y >> p->corners[idx].z;
                }
                else if (key == "color") l >> color.x >> color.y >> color.z;
                else if (key == "texture") l >> color_filename;
                else if (key == "reflectivity") l >> material.reflectivity;
                else if (key == "refractivity") l >> material.refractivity;
                else if (key == "ior") l >> material.ior;
                else if (key == "shininess") l >> material.shininess;
                else if (key == "roughness") l >> material.roughness;
            }
            p->name = name;
            p->color = color;
            p->texture_file = color_filename;
            p->material = material; // 设置材质
            scene.objects.push_back(p);
        }
        else if (token == "Cube") {
            Vector3 trans{0,0,0}, color{0.7,0.7,0.9}, size{1.0,1.0,1.0};
            double rx=0, ry=0, rz=0, scale=1.0;
            std::string color_filename;
            Material material; // 添加材质属性

            while (std::getline(in, line)) {
                line = trim(line);
                if (line == "end") break;
                std::istringstream l(line);
                std::string key; l >> key;
                if (key == "translation") l >> trans.x >> trans.y >> trans.z;
                else if (key == "rotation") l >> rx >> ry >> rz;
                else if (key == "scale")
                {
                    // 保留规模向下兼容
                    double uniform_scale;
                    l >> uniform_scale;
                    size = Vector3(uniform_scale, uniform_scale, uniform_scale);
                }
                else if (key == "size")
                {
                    l >> size.x >> size.y >> size.z;
                }
                else if (key == "color") l >> color.x >> color.y >> color.z;
                else if (key == "texture") l >> color_filename;
                else if (key == "reflectivity") l >> material.reflectivity;
                else if (key == "refractivity") l >> material.refractivity;
                else if (key == "ior") l >> material.ior;
                else if (key == "shininess") l >> material.shininess;
                else if (key == "roughness") l >> material.roughness;
            }
            auto c = std::make_shared<Cube>();
            c->name = name;
            c->center = trans;
            c->set_size(size.x, size.y, size.z);
            c->set_rotation(rx, ry, rz);
            c->color = color;
            c->texture_file = color_filename;
            c->material = material; // 设置材质
            scene.objects.push_back(c);
        }
        else if (token == "Scene") {
            while (std::getline(in, line) && line != "end") {
                line = trim(line);
                std::istringstream l(line);
                std::string key; l >> key;
                if (key == "ambient") l >> scene.ambient_light.x >> scene.ambient_light.y >> scene.ambient_light.z;
                else if (key == "background") l >> scene.background_color.x >> scene.background_color.y >> scene.background_color.z;
            }
        }
        else {
            std::cerr << "Warning: Unknown token " << token << "\n";
            while (std::getline(in, line) && line != "end");
        }
    }

    std::cout << "Scene loaded: " << scene.objects.size() << " objects, "
              << scene.lights.size() << " lights.\n";
    if (!scene.camera) std::cerr << "Warning: No camera found in scene file!\n";

    // 加载纹理文件
    std::unordered_map<std::string, std::shared_ptr<Image>> tex_cache;
    int loaded_textures = 0;

    for (auto &obj : scene.objects) {
        if (obj->texture_file.empty()) continue;

        // 构建纹理文件的完整路径
        std::string texture_path = textures_dir + "\\" + obj->texture_file + ".ppm";

        //std::cout << "Searching for texture: " << obj->texture_file << " -> " << texture_path << std::endl;

        if (!std::filesystem::exists(texture_path)) {
            std::cerr << "Warning: Texture file not found: " << texture_path << std::endl;
            continue;
        }

        texture_path = std::filesystem::canonical(texture_path).string();
        //std::cout << "  Found at: " << texture_path << std::endl;

        // 加载纹理
        auto it = tex_cache.find(texture_path);
        if (it == tex_cache.end()) {
            auto img = std::make_shared<Image>();
            std::cout << "Loading texture: " << texture_path << std::endl;

            // 检查文件内容
            std::ifstream test_file(texture_path);
            if (test_file) {
                std::string first_line;
                std::getline(test_file, first_line);
                std::cout << "First line: " << first_line << std::endl;

                std::string second_line;
                std::getline(test_file, second_line);
                std::cout << "Second line: " << second_line << std::endl;

                test_file.close();
            }

            if (img->load_ppm(texture_path)) {
                tex_cache[texture_path] = img;
                obj->texture_image = img;
                loaded_textures++;
                std::cout << "Texture loaded successfully!" << std::endl;
            } else {
                std::cerr << "Warning: Failed to load texture " << texture_path << "\n";
            }
        }
        else {
            obj->texture_image = it->second;
            //std::cout << "Texture reused from cache: " << texture_path << std::endl;
        }
    }

    std::cout << "Loaded " << loaded_textures << " textures." << std::endl;

    return scene;
}