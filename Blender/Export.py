import bpy
from mathutils import Vector
import os


def camera_gaze(cam_obj):
    mat = cam_obj.matrix_world.to_3x3()
    local_forward = Vector((0.0, 0.0, -1.0))
    world_forward = mat @ local_forward
    return world_forward.normalized()


def plane_corners(obj):
    bbox_world = [obj.matrix_world @ Vector(corner) for corner in obj.bound_box]
    xs = [v.x for v in bbox_world]
    ys = [v.y for v in bbox_world]
    minx, maxx = min(xs), max(xs)
    miny, maxy = min(ys), max(ys)
    result = []
    for px, py in [(minx, miny), (minx, maxy), (maxx, maxy), (maxx, miny)]:
        best = min(bbox_world, key=lambda v: (v.x - px) ** 2 + (v.y - py) ** 2)
        result.append(best)
    return result


def get_material_color(obj):
    """获取对象的材质颜色"""
    if obj.active_material:
        # 尝试从材质表面颜色获取
        if hasattr(obj.active_material, 'diffuse_color'):
            color = obj.active_material.diffuse_color
            return (color[0], color[1], color[2])
    # 默认颜色
    return (0.8, 0.8, 0.8)


def get_material_properties(obj):
    """获取材质属性（反射率、折射率等）"""
    reflectivity = 0.0
    refractivity = 0.0
    ior = 1.0
    shininess = 32.0

    if obj.active_material and obj.active_material.use_nodes:
        for node in obj.active_material.node_tree.nodes:
            if node.type == 'BSDF_PRINCIPLED':
                # 从原理化BSDF估算材质属性
                metallic = node.inputs['Metallic'].default_value
                roughness = node.inputs['Roughness'].default_value

                # 兼容性处理：不同Blender版本的输入名称
                try:
                    # 新版本Blender
                    transmission = node.inputs['Transmission'].default_value
                except KeyError:
                    try:
                        # 旧版本Blender
                        transmission = node.inputs['Transmission Weight'].default_value
                    except KeyError:
                        # 如果都没有，使用默认值
                        transmission = 0.0

                try:
                    ior = node.inputs['IOR'].default_value
                except KeyError:
                    ior = 1.0

                # 简单估算
                reflectivity = metallic * (1.0 - roughness)
                refractivity = transmission
                shininess = 100.0 * (1.0 - roughness)
                break

    return reflectivity, refractivity, ior, shininess


def get_texture_filename(obj):
    """获取对象的纹理文件名（只返回文件名，不包含路径）"""
    if obj.active_material and obj.active_material.use_nodes:
        for node in obj.active_material.node_tree.nodes:
            if node.type == 'TEX_IMAGE' and node.image:
                # 获取图像文件名
                if node.image.filepath:
                    # 从文件路径获取
                    filename = os.path.basename(node.image.filepath)
                else:
                    # 从图像名称获取
                    filename = node.image.name

                # 移除扩展名，在加载时会自动添加.ppm
                if filename.lower().endswith('.ppm'):
                    filename = filename[:-4]
                elif filename.lower().endswith(('.png', '.jpg', '.jpeg', '.tga', '.bmp')):
                    filename = filename.rsplit('.', 1)[0]

                return filename
    return ""


def export_scene():
    """导出场景到ASCII文件"""
    scene = bpy.context.scene

    # 确定输出路径 - 在ASCII目录中
    blend_filepath = bpy.data.filepath
    if blend_filepath:
        # 如果Blend文件已保存，在相同目录的ASCII文件夹中输出
        blend_dir = os.path.dirname(blend_filepath)
        output_dir = os.path.join(blend_dir, "ASCII")
    else:
        # 如果Blend文件未保存，使用默认路径
        output_dir = "//ASCII"

    # 创建输出目录
    os.makedirs(output_dir, exist_ok=True)

    # 输出文件名
    output_filename = "scene.txt"
    output_path = os.path.join(output_dir, output_filename)
    output_path = bpy.path.abspath(output_path)

    print(f"Exporting scene to: {output_path}")

    with open(output_path, "w") as f:
        # 背景色和环境光
        if scene.world and hasattr(scene.world, 'color'):
            f.write(f"Background {scene.world.color.r:.6f} {scene.world.color.g:.6f} {scene.world.color.b:.6f}\n")
        else:
            f.write(f"Background 0.0 0.0 0.0\n")
        f.write(f"AmbientLight 0.1 0.1 0.1\n")

        # Cameras
        for obj in scene.objects:
            if obj.type == "CAMERA":
                cam = obj.data
                gaze = camera_gaze(obj)
                f.write(f"Camera {obj.name}\n")
                f.write(f"location {obj.location.x:.6f} {obj.location.y:.6f} {obj.location.z:.6f}\n")
                f.write(f"gaze {gaze.x:.6f} {gaze.y:.6f} {gaze.z:.6f}\n")
                f.write(f"focal_length {cam.lens:.6f}\n")
                f.write(f"sensor_width {cam.sensor_width:.6f}\n")
                f.write(f"sensor_height {cam.sensor_height:.6f}\n")
                f.write(f"resolution {scene.render.resolution_x} {scene.render.resolution_y}\n")
                f.write("end\n\n")

        # Point Lights
        for obj in scene.objects:
            if obj.type == "LIGHT" and obj.data.type == "POINT":
                light = obj.data
                f.write(f"PointLight {obj.name}\n")
                f.write(f"location {obj.location.x:.6f} {obj.location.y:.6f} {obj.location.z:.6f}\n")
                f.write(f"intensity {light.energy:.6f}\n")
                f.write("end\n\n")

        # Spheres
        for obj in scene.objects:
            if obj.type == "MESH" and "sphere" in obj.name.lower():
                color = get_material_color(obj)
                reflectivity, refractivity, ior, shininess = get_material_properties(obj)
                texture_file = get_texture_filename(obj)

                f.write(f"Sphere {obj.name}\n")
                f.write(f"location {obj.location.x:.6f} {obj.location.y:.6f} {obj.location.z:.6f}\n")

                # 计算球体半径 - 使用平均缩放值
                avg_scale = (obj.scale.x + obj.scale.y + obj.scale.z) / 3.0
                f.write(f"radius {avg_scale:.6f}\n")

                f.write(f"color {color[0]:.6f} {color[1]:.6f} {color[2]:.6f}\n")
                if texture_file:
                    f.write(f"texture {texture_file}\n")
                f.write(f"reflectivity {reflectivity:.6f}\n")
                f.write(f"refractivity {refractivity:.6f}\n")
                f.write(f"ior {ior:.6f}\n")
                f.write(f"shininess {shininess:.6f}\n")
                f.write("end\n\n")

        # Cubes
        for obj in scene.objects:
            if obj.type == "MESH" and "cube" in obj.name.lower():
                color = get_material_color(obj)
                reflectivity, refractivity, ior, shininess = get_material_properties(obj)
                texture_file = get_texture_filename(obj)

                f.write(f"Cube {obj.name}\n")
                f.write(f"translation {obj.location.x:.6f} {obj.location.y:.6f} {obj.location.z:.6f}\n")
                f.write(f"rotation {obj.rotation_euler.x:.6f} {obj.rotation_euler.y:.6f} {obj.rotation_euler.z:.6f}\n")

                # 导出尺寸 - 考虑Blender默认立方体尺寸为2x2x2
                actual_size_x = obj.scale.x * 2.0
                actual_size_y = obj.scale.y * 2.0
                actual_size_z = obj.scale.z * 2.0
                f.write(f"size {actual_size_x:.6f} {actual_size_y:.6f} {actual_size_z:.6f}\n")

                f.write(f"color {color[0]:.6f} {color[1]:.6f} {color[2]:.6f}\n")
                if texture_file:
                    f.write(f"texture {texture_file}\n")
                f.write(f"reflectivity {reflectivity:.6f}\n")
                f.write(f"refractivity {refractivity:.6f}\n")
                f.write(f"ior {ior:.6f}\n")
                f.write(f"shininess {shininess:.6f}\n")
                f.write("end\n\n")

        # Planes
        for obj in scene.objects:
            if obj.type == "MESH" and "plane" in obj.name.lower():
                color = get_material_color(obj)
                reflectivity, refractivity, ior, shininess = get_material_properties(obj)
                texture_file = get_texture_filename(obj)
                corners = plane_corners(obj)

                f.write(f"Plane {obj.name}\n")
                for i, c in enumerate(corners):
                    f.write(f"corner{i + 1} {c.x:.6f} {c.y:.6f} {c.z:.6f}\n")
                f.write(f"color {color[0]:.6f} {color[1]:.6f} {color[2]:.6f}\n")
                if texture_file:
                    f.write(f"texture {texture_file}\n")
                f.write(f"reflectivity {reflectivity:.6f}\n")
                f.write(f"refractivity {refractivity:.6f}\n")
                f.write(f"ior {ior:.6f}\n")
                f.write(f"shininess {shininess:.6f}\n")
                f.write("end\n\n")

    print(f"Scene exported successfully to: {output_path}")

    # 统计信息
    cameras = len([obj for obj in scene.objects if obj.type == "CAMERA"])
    lights = len([obj for obj in scene.objects if obj.type == "LIGHT" and obj.data.type == "POINT"])
    spheres = len([obj for obj in scene.objects if obj.type == "MESH" and "sphere" in obj.name.lower()])
    cubes = len([obj for obj in scene.objects if obj.type == "MESH" and "cube" in obj.name.lower()])
    planes = len([obj for obj in scene.objects if obj.type == "MESH" and "plane" in obj.name.lower()])

    print(f"\nExport Summary:")
    print(f"Cameras: {cameras}")
    print(f"Point Lights: {lights}")
    print(f"Spheres: {spheres}")
    print(f"Cubes: {cubes}")
    print(f"Planes: {planes}")
    print(f"Output directory: {output_dir}")


# 运行导出
if __name__ == "__main__":
    export_scene()

    # 可选：显示完成消息
    print("\n=== Export Complete ===")
    print("Please check the ASCII directory for scene.txt")
    print("Make sure your texture files are in the Textures directory")