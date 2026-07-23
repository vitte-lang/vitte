#!/usr/bin/env python3
import csv
from pathlib import Path

CSV = Path('pkgout/diagnostics_for_translation.csv')
OUT_DIR = Path('locales/zh-CN')
OUT_DIR.mkdir(parents=True, exist_ok=True)

def translate(s: str) -> str:
    t = s
    repl = [
        ('division by zero', '除以零'),
        ('division by zero in constant evaluation', '常量求值时除以零'),
        ('const division by zero', '常量除以零'),
        ('duplicate', '重复'),
        ('missing', '缺失'),
        ('not found', '未找到'),
        ('not implemented', '未实现'),
        ('invalid', '无效'),
        ('expected', '需要'),
        ('unknown', '未知'),
        ('required', '需要'),
        ('ambiguous', '歧义'),
        ('conflict', '冲突'),
        ('unsupported', '不支持'),
        ('failed', '失败'),
        ('overflow', '溢出'),
        ('runtime panic', '运行时恐慌'),
        ('link failed', '链接失败'),
        ('assembler failed', '汇编器失败'),
        ('abi mismatch', 'ABI 不匹配'),
        ('argument mismatch', '参数不匹配'),
        ('arity mismatch', '参数个数不匹配'),
        ('assignment mismatch', '赋值类型不匹配'),
        ('borrow conflict', '借用冲突'),
        ('branch mismatch', '分支不匹配'),
        ('object write failed', '写入对象失败'),
        ('expected block', '需要块'),
        ('expected delimiter', '需要分隔符'),
        ('expected expression', '需要表达式'),
        ('expected identifier', '需要标识符'),
        ('expected pattern', '需要模式'),
        ('expected type', '需要类型'),
        ('const overflow', '常量溢出'),
        ('const cycle', '常量循环'),
        ('cycle', '循环'),
        ('dangling reference', '悬空引用'),
        ('double drop', '重复释放'),
        ('use after drop', '释放后使用'),
        ('use after move', '移动后使用'),
        ('use before init', '初始化前使用'),
        ('non exhaustive', '非穷尽'),
        ('not found', '未找到'),
        ('missing body', '缺少主体'),
        ('missing return', '缺少返回'),
        ('private symbol', '私有符号'),
    ]
    for a,b in repl:
        t = t.replace(a, b)
        t = t.replace(a.capitalize(), b)
    # general small fixes
    t = t.replace('->', '→')
    if all(ord(c) < 128 for c in t):
        # if still English, try some simple token mappings
        t = t.replace(' ', ' ')  # no-op
    return t

def main():
    msgs = []
    hints = []
    with CSV.open(newline='') as f:
        r = csv.reader(f)
        next(r)
        for key, eng in r:
            tr = translate(eng)
            if key.endswith('.hint') or key.endswith('.explain') or '.hint' in key:
                hints.append((key, tr))
            else:
                msgs.append((key, tr))

    # write diagnostics.ftl
    out_msgs = OUT_DIR / 'diagnostics.ftl'
    with out_msgs.open('w', encoding='utf-8') as f:
        f.write('# Generated zh-CN diagnostics\n')
        for k,v in msgs:
            f.write(f"{k} = {v}\n")

    out_hints = OUT_DIR / 'diagnostics_explain.ftl'
    with out_hints.open('w', encoding='utf-8') as f:
        f.write('# Generated zh-CN diagnostic explanations\n')
        # first, write explicit hints from CSV
        for k,v in hints:
            f.write(f"{k} = {v}\n")
        # then, generate summary/cause/step1/fix/example for each message key
        for k,v in msgs:
            # skip keys that contain dots (they are specialized)
            if '.' in k:
                continue
            base = k
            f.write(f"{base}.summary = 简要：{v}\n")
            f.write(f"{base}.cause = 可能的原因：{v} 未提供或语法不正确。\n")
            f.write(f"{base}.step1 = 步骤：检查相关代码，确保提供所需的元素或修正语法。\n")
            f.write(f"{base}.fix = 修复：在对应位置添加或修正为合法值或结构。\n")
            f.write(f"{base}.example = 示例：请参照文档或示例代码进行修正。\n")

if __name__ == '__main__':
    main()
