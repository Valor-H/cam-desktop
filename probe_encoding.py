#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""探测 NMainWindow.cpp 中乱码的实际编码"""

FILE_PATH = r"D:\Codes\CamDemo\NMainWindow.cpp"

with open(FILE_PATH, 'rb') as f:
    raw = f.read()

# 尝试用不同编码解码
for enc in ['utf-8', 'gbk', 'gb2312', 'gb18030', 'latin-1']:
    print(f"\n=== Trying {enc} ===")
    try:
        text = raw.decode(enc)
        # 找包含 QStringLiteral 的行中的非ASCII内容
        for i, line in enumerate(text.split('\n'), 1):
            if 'QStringLiteral' in line:
                # 检测是否有乱码（连续的非正常中文）
                has_garble = any(ord(c) > 0x4e00 and ord(c) < 0x9fff for c in line)
                if has_garble:
                    # 提取引号内的内容
                    import re
                    matches = re.findall(r'QStringLiteral\("([^"]*)"\)', line)
                    for m in matches:
                        if any(ord(c) > 0x200 for c in m):
                            bytes_repr = m.encode(enc).hex(' ')
                            print(f"  Line {i}: {bytes_repr}")
                            print(f"         -> {m!r}")
        break
    except Exception as e:
        print(f"  Error: {e}")

# 同时输出原始字节范围
print("\n=== Raw bytes around line 179 ===")
lines = raw.split(b'\n')
if len(lines) >= 179:
    for i in range(176, min(190, len(lines))):
        line_bytes = lines[i]
        print(f"  Line {i+1}: {line_bytes.hex(' ')}")
