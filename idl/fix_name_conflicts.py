
import os
import re

# 遍历所有idl文件
for filename in os.listdir('.'):
    if not filename.endswith('.idl'):
        continue

    with open(filename, 'r') as f:
        lines = f.readlines()

    modified = False

    # 检查每一行
    for i, line in enumerate(lines):
        # 查找类似 "geometry_msgs::msg::Accel accel;" 的模式
        match = re.search(r'(\w+::\w+::(\w+))\s+(\w+)\s*;', line)
        if match:
            full_type = match.group(1)  # 例如: "geometry_msgs::msg::Accel"
            type_name = match.group(2)  # 例如: "Accel"
            var_name = match.group(3)   # 例如: "accel"

            # 检查类型名和变量名是否相同（忽略大小写）
            if type_name.lower() == var_name.lower():
                # 修改变量名，添加下划线
                new_var_name = var_name + '_'
                new_line = line.replace(var_name + ';', new_var_name + ';')
                lines[i] = new_line
                modified = True
                print(f"In {filename}: Changed {var_name} to {new_var_name}")

    # 如果有修改，写回文件
    if modified:
        with open(filename, 'w') as f:
            f.writelines(lines)
