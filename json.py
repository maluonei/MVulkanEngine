import os
import json
import math
import numpy as np

colmap_path = R"D:\Documents\research\3DGaussianSplatting\gaussian-splatting-raw\output\glassball\cameras.json"
blender_path = ""

def fov2focal(fov, pixels):
    return pixels / (2 * math.tan(fov / 2))

def focal2fov(focal, pixels):
    return 2*math.atan(pixels/(2*focal))

def colmap2blender(colmap_data):
    pos = colmap_data["position"]
    rot = colmap_data["rotation"]
    W2C = np.zeros((4, 4))
    W2C[3, 3] = 1.0
    W2C[:3, 3] = pos
    W2C[:3, :3] = rot
    Rt = np.linalg.inv(W2C)
    R = Rt[:3, :3].transpose()
    T = Rt[:3, 3]
    w2c = np.zeros((4, 4))
    w2c[3, 3] = 1.0
    w2c[:3, :3] = np.transpose(R)
    w2c[:3, 3] = T
    c2w = np.linalg.inv(w2c)
    c2w[:3, 1:3] *= -1
    img_name = colmap_data["img_name"]
    file_path = f"./images/{img_name}"
    return {
        "file_path": file_path,
        "transform_matrix": c2w.tolist()
    }


with open(colmap_path, "r") as fin:
    colmap_datas = json.load(fin)

fx = colmap_datas[0]["fx"]
fy = colmap_datas[0]["fy"]
W = colmap_datas[0]["width"]
H = colmap_datas[0]["height"]
fovx = focal2fov(fx, W)
fovy = focal2fov(fy, H)

train_blender_datas = {
    "camera_angle_x": fovx,
    "camera_angle_y": fovy,
    "w": W,
    "h": H,
    "frames": []
}

test_blender_datas = {
    "camera_angle_x": fovx,
    "camera_angle_y": fovy,
    "w": W,
    "h": H,
    "frames": []
}

num_datas = len(colmap_datas)
num_test = (num_datas + 7) // 8
print(f"num_test: {num_test}")

for i, colmap_data in enumerate(colmap_datas):
    blender_data = colmap2blender(colmap_data)
    if i < num_test:
        test_blender_datas["frames"] += [blender_data]
    else:
        train_blender_datas["frames"] += [blender_data]

with open(os.path.join(blender_path, f"transforms_train.json"), "w") as fout:
    json.dump(train_blender_datas, fout)

with open(os.path.join(blender_path, f"transforms_test.json"), "w") as fout:
    json.dump(test_blender_datas, fout)