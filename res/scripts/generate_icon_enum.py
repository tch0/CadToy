#!/usr/bin/env python3
"""
扫描图标目录，生成 C++ 头文件，包含：
  - enum class IconID (默认 int, 值: kGeneralRefresh, kLayerCurrent, ...)
  - inline const std::map<IconID, std::string> iconPaths
    (路径使用相对于程序运行目录的相对路径，使用时需要自行拼接)

用法：
  python generate_icon_enum.py --icon-dir res/icons --output src/generated/IconDefines.h
"""

import os
import argparse
import sys
import datetime

def to_enum_name(filename: str) -> str:
    """将文件名转换为 k 前缀的 PascalCase 枚举名，例如 LayerLocked.png -> kLayerLocked"""
    name = os.path.splitext(filename)[0]
    safe_name = ''.join(ch if ch.isalnum() else '_' for ch in name)
    if safe_name[0].isdigit():
        safe_name = '_' + safe_name
    return 'k' + safe_name

def generate_header(icon_dir: str, output_file: str) -> None:
    if not os.path.isdir(icon_dir):
        print(f"错误: 图标目录不存在: {icon_dir}")
        sys.exit(1)

    png_files = [f for f in os.listdir(icon_dir) if f.lower().endswith('.png')]
    png_files.sort(key=str.lower)

    if not png_files:
        print("警告: 目录中没有 PNG 文件，生成空头文件。")

    os.makedirs(os.path.dirname(output_file), exist_ok=True)

    lines = []
    lines.append('#pragma once')
    lines.append('')
    lines.append('// 由 generate_icon_enum.py 脚本自动生成，请勿手动修改')
    lines.append('// 在 build/ 目录执行 cmake --build . --target update_icon_defines 以重新生成')
    lines.append(f'// 生成时间: {datetime.datetime.now().isoformat()}')
    lines.append('')
    lines.append('#include <string>')
    lines.append('#include <map>')
    lines.append('')

    # 枚举定义 (默认底层类型 int)
    lines.append('enum class IconID {')
    for i, f in enumerate(png_files):
        enum_val = to_enum_name(f)
        if i == 0:
            lines.append(f'    {enum_val} = 0,')
        else:
            lines.append(f'    {enum_val},')
    lines.append('    kCOUNT')
    lines.append('};')
    lines.append('')

    # 图标路径映射表，得到的是相对于可执行文件所在目录的相对路径
    lines.append('// 图标路径映射表，得到的是相对于可执行文件所在目录的相对路径')
    lines.append('inline const std::map<IconID, std::string> g_iconPaths = {')
    for f in png_files:
        enum_val = to_enum_name(f)
        icon_path = f'"res/icons/{f}"'
        lines.append(f'    {{ IconID::{enum_val}, {icon_path} }},')
    lines.append('};')
    lines.append('')

    with open(output_file, 'w', encoding='utf-8') as fout:
        fout.write('\n'.join(lines))

    print(f'生成成功: {output_file}')
    print(f'包含 {len(png_files)} 个图标。')

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='生成图标枚举头文件 (k枚举 + map<IconID, path>)')
    parser.add_argument('--icon-dir', required=True, help='包含 PNG 图标的目录')
    parser.add_argument('--output', required=True, help='输出头文件路径')
    args = parser.parse_args()
    generate_header(args.icon_dir, args.output)