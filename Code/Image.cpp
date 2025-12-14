#include "Image.h"
#include <fstream>
#include <sstream>
#include <algorithm>  // for std::clamp
#include <windows.h>

void Image::write_ppm(const std::string &filename) const {
    std::ofstream ofs(filename);
    if (!ofs.is_open()) {
        std::cerr << "Error: Cannot open " << filename << " for writing.\n";
        return;
    }

    ofs << "P3\n" << width << " " << height << "\n255\n";
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            Color c = pixels[y * width + x];
            int r = std::clamp(int(c.r * 255.0), 0, 255);
            int g = std::clamp(int(c.g * 255.0), 0, 255);
            int b = std::clamp(int(c.b * 255.0), 0, 255);
            ofs << r << " " << g << " " << b << " ";
        }
        ofs << "\n";
    }
}

// ✅ 接受 Vector3 的颜色输入
void Image::set_pixel(int x, int y, const Vector3 &color) {
    if (x < 0 || x >= width || y < 0 || y >= height) return;
    pixels[y * width + x] = Color(color.x, color.y, color.z);
}

// ✅ 返回 Vector3，方便渲染端使用
Vector3 Image::get_pixel(int x, int y) const {
    if (x < 0 || x >= width || y < 0 || y >= height) return {0, 0, 0};
    Color c = pixels[y * width + x];
    return Vector3(c.r, c.g, c.b);
}

// load_ppm: 仅支持 P3 ASCII，忽略注释
bool Image::load_ppm(const std::string &filename) {
    // 直接尝试打开，让filesystem处理路径
    std::ifstream ifs(filename);

    if (!ifs.is_open()) {
        std::cerr << "Cannot open file: " << filename << std::endl;
        return false;
    }

    std::string magic;
    ifs >> magic;
    std::cout << "PPM magic: " << magic << std::endl;

    if (magic != "P3") {
        std::cerr << "Not a P3 file: " << magic << std::endl;
        return false;
    }

    int w, h, maxv;

    // 跳过注释
    char c;
    while (ifs.get(c)) {
        if (c == '#') {
            std::string comment;
            std::getline(ifs, comment);
        } else if (!std::isspace(c)) {
            ifs.putback(c);
            break;
        }
    }

    ifs >> w >> h >> maxv;
    std::cout << "Image dimensions: " << w << "x" << h << ", maxval: " << maxv << std::endl;

    if (w <= 0 || h <= 0 || maxv <= 0) return false;

    width = w;
    height = h;
    pixels.assign(width * height, Color(0, 0, 0));

    for (int i = 0; i < width * height; ++i) {
        int r, g, b;
        if (!(ifs >> r >> g >> b)) return false;
        pixels[i] = Color(r / 255.0, g / 255.0, b / 255.0);
    }

    std::cout << "Successfully loaded " << width << "x" << height << " image" << std::endl;
    return true;
}

// sample_uv: 最近邻（wrap）
Vector3 Image::sample_uv(double u, double v) const {
    if (width <= 0 || height <= 0) return {1,1,1};
    // wrap repeat
    u = u - floor(u);
    v = v - floor(v);
    int px = std::min(width-1, std::max(0, int(u * width)));
    int py = std::min(height-1, std::max(0, int((1.0 - v) * height))); // v=0 bottom -> top
    Color c = pixels[py * width + px];
    return Vector3(c.r, c.g, c.b);
}
