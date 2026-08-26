#!/usr/bin/env python3
"""Generate System Architecture Diagram (ESP32 → Node-RED → InfluxDB → Grafana)."""

from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

OUT_DIR = Path(__file__).resolve().parent
FONT_DIR = OUT_DIR / "fonts"
OUT_PNG = OUT_DIR / "รูปที่-3.1-สถาปัตยกรรมระบบโดยรวม.png"
OUT_ARTIFACT = Path("/opt/cursor/artifacts/system-architecture-diagram.png")

W, H = 2400, 1350
BG = (255, 255, 255)
INK = (30, 41, 59)
MUTED = (71, 85, 105)
LINE = (148, 163, 184)
ARROW = (51, 65, 85)

COMPONENTS = [
    {
        "name": "ESP32",
        "role_th": "ชั้น Edge / เซนเซอร์",
        "role_en": "Edge Sensing",
        "detail": "IR Total / Good / Waste\nนับชิ้นงาน + Publish",
        "fill": (14, 116, 144),
        "light": (236, 254, 255),
    },
    {
        "name": "Node-RED",
        "role_th": "ชั้น Logic / ประมวลผล",
        "role_en": "OEE + Watchdog",
        "detail": "คำนวณ OEE\nตรวจสถานะ RUNNING / IDLE",
        "fill": (180, 83, 9),
        "light": (255, 247, 237),
    },
    {
        "name": "InfluxDB",
        "role_th": "ชั้น Storage / จัดเก็บ",
        "role_en": "Time-Series DB",
        "detail": "bucket: conveyor_oee\nmeasurement: oee_metrics",
        "fill": (22, 101, 52),
        "light": (240, 253, 244),
    },
    {
        "name": "Grafana",
        "role_th": "ชั้น Visualization / แสดงผล",
        "role_en": "Monitor Dashboard",
        "detail": "เกจ OEE / สถานะเครื่อง\nกราฟแนวโน้มการผลิต",
        "fill": (30, 64, 175),
        "light": (239, 246, 255),
    },
]

ARROWS = [
    ("MQTT / TLS\n:8883", "HiveMQ Cloud"),
    ("Write\nทุก 1 วินาที", "oee_metrics"),
    ("Query\nFlux / Live", "Dashboard"),
]


def load_font(path: str | Path, size: int) -> ImageFont.FreeTypeFont:
    return ImageFont.truetype(str(path), size)


def rounded_rect(draw, xy, radius, fill, outline=None, width=2):
    draw.rounded_rectangle(xy, radius=radius, fill=fill, outline=outline, width=width)


def center_text(draw, cx, y, text, font, fill):
    bbox = draw.textbbox((0, 0), text, font=font)
    tw = bbox[2] - bbox[0]
    draw.text((cx - tw / 2, y), text, font=font, fill=fill)
    return bbox[3] - bbox[1]


def multiline_center(draw, cx, y, text, font, fill, line_gap=8):
    lines = text.split("\n")
    cy = y
    for line in lines:
        h = center_text(draw, cx, cy, line, font, fill)
        cy += h + line_gap
    return cy


def draw_arrow(draw, x1, y, x2, label, sublabel, font_label, font_sub):
    draw.line([(x1, y), (x2 - 18, y)], fill=ARROW, width=6)
    draw.polygon(
        [(x2, y), (x2 - 28, y - 14), (x2 - 28, y + 14)],
        fill=ARROW,
    )
    mid = (x1 + x2) / 2
    multiline_center(draw, mid, y - 78, label, font_label, INK, line_gap=4)
    if sublabel:
        bbox = draw.textbbox((0, 0), sublabel, font=font_sub)
        tw = bbox[2] - bbox[0]
        th = bbox[3] - bbox[1]
        pad_x, pad_y = 14, 8
        bx1 = mid - tw / 2 - pad_x
        by1 = y + 22
        bx2 = mid + tw / 2 + pad_x
        by2 = by1 + th + pad_y * 2
        rounded_rect(draw, (bx1, by1, bx2, by2), 12, (248, 250, 252), LINE, 2)
        draw.text((bx1 + pad_x, by1 + pad_y - 2), sublabel, font=font_sub, fill=MUTED)


def main():
    # Sarabun supports Thai + Latin (needed for mixed labels in academic diagrams)
    font_title = load_font(FONT_DIR / "Sarabun-Bold.ttf", 52)
    font_subtitle = load_font(FONT_DIR / "Sarabun-Bold.ttf", 30)
    font_en_title = load_font(FONT_DIR / "Sarabun-Regular.ttf", 26)
    font_box_name = load_font("/usr/share/fonts/truetype/noto/NotoSans-Bold.ttf", 42)
    font_role_th = load_font(FONT_DIR / "Sarabun-Bold.ttf", 26)
    font_role_en = load_font("/usr/share/fonts/truetype/noto/NotoSans-Regular.ttf", 22)
    font_detail = load_font(FONT_DIR / "Sarabun-Regular.ttf", 24)
    font_arrow = load_font(FONT_DIR / "Sarabun-Bold.ttf", 22)
    font_step = load_font("/usr/share/fonts/truetype/noto/NotoSans-Bold.ttf", 22)
    font_footer = load_font(FONT_DIR / "Sarabun-Regular.ttf", 24)
    font_caption = load_font(FONT_DIR / "Sarabun-Bold.ttf", 24)
    font_badge = load_font("/usr/share/fonts/truetype/noto/NotoSans-Regular.ttf", 18)

    img = Image.new("RGB", (W, H), BG)
    draw = ImageDraw.Draw(img)

    draw.rectangle((0, 0, W, 160), fill=(248, 250, 252))
    draw.line([(0, 160), (W, 160)], fill=LINE, width=2)

    center_text(
        draw,
        W / 2,
        34,
        "แผนภาพสถาปัตยกรรมระบบโดยรวม (System Architecture)",
        font_title,
        INK,
    )
    center_text(
        draw,
        W / 2,
        100,
        "ESP32  →  Node-RED  →  InfluxDB  →  Grafana   |   IIoT OEE Smart Conveyor",
        font_en_title,
        MUTED,
    )

    margin_x = 70
    gap = 70
    box_w = (W - 2 * margin_x - 3 * gap) / 4
    box_h = 520
    box_y = 280
    arrow_y = box_y + box_h / 2

    xs = []
    for i, comp in enumerate(COMPONENTS):
        x1 = margin_x + i * (box_w + gap)
        x2 = x1 + box_w
        xs.append((x1, x2))
        cx = (x1 + x2) / 2

        rounded_rect(draw, (x1, box_y, x2, box_y + box_h), 28, comp["light"], comp["fill"], 4)

        header_h = 120
        draw.rounded_rectangle(
            (x1, box_y, x2, box_y + header_h + 24),
            radius=28,
            fill=comp["fill"],
        )
        draw.rectangle((x1, box_y + header_h - 10, x2, box_y + header_h + 24), fill=comp["fill"])

        r = 22
        nx, ny = x1 + 40, box_y + header_h / 2
        draw.ellipse((nx - r, ny - r, nx + r, ny + r), fill=(255, 255, 255))
        center_text(draw, nx, ny - 12, str(i + 1), font_step, comp["fill"])

        center_text(draw, cx + 8, box_y + 28, comp["name"], font_box_name, (255, 255, 255))
        center_text(draw, cx, box_y + 78, comp["role_en"], font_role_en, (255, 255, 255))

        y = box_y + header_h + 48
        center_text(draw, cx, y, comp["role_th"], font_role_th, comp["fill"])
        y += 48
        draw.line([(x1 + 40, y), (x2 - 40, y)], fill=LINE, width=2)
        y += 28
        multiline_center(draw, cx, y, comp["detail"], font_detail, MUTED, line_gap=10)

    for i, (label, sub) in enumerate(ARROWS):
        x1 = xs[i][1] + 8
        x2 = xs[i + 1][0] - 8
        draw_arrow(draw, x1, arrow_y, x2, label, sub, font_arrow, font_badge)

    legend_y = box_y + box_h + 70
    rounded_rect(
        draw,
        (margin_x, legend_y, W - margin_x, legend_y + 160),
        20,
        (248, 250, 252),
        LINE,
        2,
    )
    center_text(
        draw,
        W / 2,
        legend_y + 24,
        "การไหลของข้อมูล (Data Flow)",
        font_subtitle,
        INK,
    )
    flow = (
        "เซนเซอร์ IR → ESP32 ส่ง MQTT → Node-RED คำนวณ OEE → บันทึก InfluxDB → แสดงผลบน Grafana"
    )
    center_text(draw, W / 2, legend_y + 74, flow, font_footer, MUTED)
    center_text(
        draw,
        W / 2,
        legend_y + 114,
        "รูปที่ 3.1  สถาปัตยกรรมระบบตรวจสอบประสิทธิภาพสายพานลำเลียงอัจฉริยะ (IIoT + OEE)",
        font_caption,
        INK,
    )

    img.save(OUT_PNG, "PNG", dpi=(300, 300))
    OUT_ARTIFACT.parent.mkdir(parents=True, exist_ok=True)
    img.save(OUT_ARTIFACT, "PNG", dpi=(300, 300))
    print(f"Wrote {OUT_PNG}")
    print(f"Wrote {OUT_ARTIFACT}")


if __name__ == "__main__":
    main()
