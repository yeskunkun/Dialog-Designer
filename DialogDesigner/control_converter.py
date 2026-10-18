import sys

# ---------- 字号/字重映射 ----------
FW_MAP = {
    100: "FW_THIN", 200: "FW_EXTRALIGHT", 300: "FW_LIGHT",
    400: "FW_NORMAL", 500: "FW_MEDIUM", 600: "FW_SEMIBOLD",
    700: "FW_BOLD", 800: "FW_EXTRABOLD", 900: "FW_HEAVY",
}

DLG_STYLE = "WS_OVERLAPPEDWINDOW|WS_VISIBLE"
DLG_STYLE_C = "WS_OVERLAPPEDWINDOW"  # 不含 WS_VISIBLE 参与边框计算


def fw_to_cpp(weight: int) -> str:
    return FW_MAP.get(weight, str(weight))


def _int_or_def(tok: str) -> str:
    return "CW_USEDEFAULT" if tok.upper() == "DEF" else str(int(tok))


def gen_dialog(line: str, hwnd: str = "hwnd") -> str:
    parts = [p.strip() for p in line.strip().split(",")]
    if len(parts) < 6:
        raise ValueError(f"dialog 字段不足: {line}")
    _, title, x, y, cw, ch = parts[:6]

    x_expr = _int_or_def(x)
    y_expr = _int_or_def(y)

    lines = []
    lines.append("{")
    lines.append(f"    RECT rc = {{ 0, 0, {int(cw)}, {int(ch)} }};")
    lines.append(f"    AdjustWindowRectEx(&rc, {DLG_STYLE_C}, FALSE, 0);")
    lines.append(
        f"    {hwnd} = CreateWindowExW(0, L\"#32770\", L\"{title}\", {DLG_STYLE}, "
        f"{x_expr}, {y_expr}, rc.right - rc.left, rc.bottom - rc.top, 0, 0, 0, 0);"
    )
    lines.append("}")
    return "\n".join(lines)


def gen_control(line: str, ctrl_id: int, hwnd: str = "hwnd", tmph: str = "tmph") -> str:
    parts = [p.strip() for p in line.strip().split(",")]
    if len(parts) < 12:
        raise ValueError(f"控件字段不足，期望 12 个，实际 {len(parts)}: {line}")
    cls, text = parts[0], parts[1]
    x, y, w, h = map(int, parts[2:6])
    font_name = parts[6]
    font_size = int(parts[7])
    weight = int(parts[8])
    underline = int(parts[9])
    italic = int(parts[10])
    strikeout = int(parts[11])

    if font_size < 0:
        height_expr = str(font_size)
    else:
        height_expr = f"-MulDiv({font_size}, GetDeviceCaps(hdc, LOGPIXELSY), 72)"

    fw = fw_to_cpp(weight)

    create = (
        f'{tmph} = CreateWindowExW(0, L"{cls}", L"{text}", '
        f'WS_CHILD|WS_VISIBLE|BS_CENTER, '
        f'{x}, {y}, {w}, {h}, {hwnd}, HMENU({ctrl_id}), 0, 0);'
    )
    font = (
        f'SendMessageW({tmph}, WM_SETFONT, WPARAM(CreateFontW('
        f'{height_expr}, 0, 0, 0, {fw}, '
        f'{italic}, {underline}, {strikeout}, '
        f'DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, '
        f'DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, '
        f'L"{font_name}")), 1);'
    )
    return create + "\n" + font


def convert(text: str) -> str:
    out = []
    ctrl_id = 1
    for raw in text.splitlines():
        line = raw.strip()
        if not line:
            continue
        if "/**Dialog Designer**/" in line:
            continue

        head = line.split(",", 1)[0].strip().lower()
        try:
            if head == "dialog":
                out.append(gen_dialog(line))
            else:
                out.append(gen_control(line, ctrl_id))
                ctrl_id += 1
        except ValueError as e:
            out.append(f"// 错误: {e}")
    return "\n".join(out)


def main():
    while True:
        print("请输入文本：", file=sys.stderr, end='')
        buffer = []
        for line in sys.stdin:
            line = line.rstrip("\n")
            if line.strip() == "":
                print("==输出============================")
                if buffer:
                    print(convert("\n".join(buffer)), flush=True)
                    buffer = []
                print("==================================")
            else:
                buffer.append(line)
        if buffer:
            print(convert("\n".join(buffer)), flush=True)


if __name__ == "__main__":
    main()
