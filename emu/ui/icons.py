"""
Monochrome Vector Icons for Apex-Dash Emulator
1-bit high-contrast graphical glyphs matching U8g2 Open Iconic font.
Drawn dynamically using QPainter vector paths honoring foreground/background colors.
"""

from PySide6.QtCore import Qt, QPointF, QRectF
from PySide6.QtGui import QPainter, QPainterPath, QPen, QColor


def draw_icon_water(p: QPainter, x: float, y: float, s: float = 12, fill: bool = False):
    """Water droplet / coolant temp icon"""
    old_pen = p.pen()
    color = old_pen.color()
    cx = x + s / 2.0
    path = QPainterPath()
    path.moveTo(cx, y + 1)
    path.quadTo(x + s - 1, y + s * 0.6, cx, y + s - 1)
    path.quadTo(x + 1, y + s * 0.6, cx, y + 1)
    if fill:
        p.fillPath(path, color)
    else:
        p.drawPath(path)


def draw_icon_flame(p: QPainter, x: float, y: float, s: float = 12, fill: bool = False):
    """Exhaust / EGT flame icon"""
    old_pen = p.pen()
    color = old_pen.color()
    cx = x + s / 2.0
    path = QPainterPath()
    path.moveTo(cx, y + 1)
    path.quadTo(x + s - 1, y + s * 0.45, x + s * 0.75, y + s - 1)
    path.quadTo(cx, y + s * 0.7, x + s * 0.25, y + s - 1)
    path.quadTo(x + 1, y + s * 0.45, cx, y + 1)
    if fill:
        p.fillPath(path, color)
    else:
        p.drawPath(path)


def draw_icon_gauge(p: QPainter, x: float, y: float, s: float = 12, fill: bool = False):
    """Tachometer / Rev gauge icon"""
    color = p.pen().color()
    p.drawArc(QRectF(x + 1, y + 1, s - 2, s - 2), 0, 180 * 16)
    p.drawLine(x + 1, y + s / 2, x + s - 1, y + s / 2)
    cx = x + s / 2
    cy = y + s / 2
    # Needle
    p.drawLine(cx, cy, cx + s * 0.3, cy - s * 0.3)


def draw_icon_battery(p: QPainter, x: float, y: float, s: float = 12, fill: bool = False):
    """Battery level icon"""
    color = p.pen().color()
    bx = x + 1
    by = y + (s - 7) / 2.0
    bw = s - 4
    bh = 7.0
    p.drawRect(QRectF(bx, by, bw, bh))
    p.fillRect(QRectF(bx + bw, by + 2, 2, 3), color)
    p.fillRect(QRectF(bx + 2, by + 2, (bw - 4) * 0.65, bh - 4), color)


def draw_icon_wireless(p: QPainter, x: float, y: float, s: float = 12, fill: bool = False):
    """Wireless radio link icon"""
    cx = x + s / 2.0
    cy = y + s - 2.0
    p.drawEllipse(QPointF(cx, cy), 1.0, 1.0)
    p.drawArc(QRectF(cx - 3.5, cy - 4.5, 7, 7), 40 * 16, 100 * 16)
    p.drawArc(QRectF(cx - 5.5, cy - 7.5, 11, 11), 40 * 16, 100 * 16)


def draw_icon_gear(p: QPainter, x: float, y: float, s: float = 12):
    """Setup / Gear icon"""
    cx = x + s / 2.0
    cy = y + s / 2.0
    r = s / 2.0 - 2.0
    p.drawEllipse(QPointF(cx, cy), r, r)
    p.drawEllipse(QPointF(cx, cy), r * 0.45, r * 0.45)
    # 4 gear teeth
    p.drawLine(cx, cy - r - 2, cx, cy - r)
    p.drawLine(cx, cy + r, cx, cy + r + 2)
    p.drawLine(cx - r - 2, cy, cx - r, cy)
    p.drawLine(cx + r, cy, cx + r + 2, cy)


def draw_icon_lightbulb(p: QPainter, x: float, y: float, s: float = 12):
    """Shift lightbulb / LED icon"""
    cx = x + s / 2.0
    p.drawEllipse(QPointF(cx, y + 4.5), 3.5, 3.5)
    p.drawLine(cx - 2, y + 8, cx + 2, y + 8)
    p.drawLine(cx - 1, y + 10, cx + 1, y + 10)
    # Side rays
    p.drawLine(cx, y, cx, y + 1.5)
    p.drawLine(x + 1, y + 4.5, x + 2.5, y + 4.5)
    p.drawLine(x + s - 1, y + 4.5, x + s - 2.5, y + 4.5)


def draw_icon_flag(p: QPainter, x: float, y: float, s: float = 12):
    """Checkered / Race flag icon"""
    p.drawLine(x + 2, y + 1, x + 2, y + s)
    path = QPainterPath()
    path.moveTo(x + 2, y + 1)
    path.lineTo(x + s - 1, y + 4)
    path.lineTo(x + 2, y + 7)
    path.closeSubpath()
    p.fillPath(path, p.pen().color())


def draw_icon_disk(p: QPainter, x: float, y: float, s: float = 12):
    """Storage disk icon"""
    p.drawRect(QRectF(x + 1, y + 1, s - 2, s - 2))
    p.fillRect(QRectF(x + 3, y + 2, s - 6, 3), p.pen().color())
    p.drawRect(QRectF(x + 3, y + 6, s - 6, s - 8))


def draw_icon_sun(p: QPainter, x: float, y: float, s: float = 12):
    """Display polarity / Backlight sun icon"""
    cx = x + s / 2.0
    cy = y + s / 2.0
    r = 2.5
    p.drawEllipse(QPointF(cx, cy), r, r)
    p.drawLine(cx, y + 1, cx, cy - r - 1)
    p.drawLine(cx, cy + r + 1, cx, y + s - 1)
    p.drawLine(x + 1, cy, cx - r - 1, cy)
    p.drawLine(cx + r + 1, cy, x + s - 1, cy)


def draw_icon_globe(p: QPainter, x: float, y: float, s: float = 12):
    """Globe / Language icon"""
    cx = x + s / 2.0
    cy = y + s / 2.0
    r = s / 2.0 - 1.5
    p.drawEllipse(QPointF(cx, cy), r, r)
    p.drawLine(x + 1.5, cy, x + s - 1.5, cy)
    p.drawEllipse(QPointF(cx, cy), r * 0.45, r)


def draw_icon_wrench(p: QPainter, x: float, y: float, s: float = 12):
    """Wrench / Diagnostics icon"""
    p.drawLine(x + 3, y + s - 3, x + s - 4, y + 4)
    p.drawLine(x + 4, y + s - 3, x + s - 3, y + 4)
    p.drawArc(QRectF(x + s - 7, y + 1, 6, 6), 30 * 16, 260 * 16)


def draw_icon_back(p: QPainter, x: float, y: float, s: float = 12):
    """Return / Exit arrow icon"""
    cy = y + s / 2.0
    p.drawLine(x + 2, cy, x + 5, y + 2)
    p.drawLine(x + 2, cy, x + 5, y + s - 2)
    p.drawLine(x + 2, cy, x + s - 2, cy)
    p.drawLine(x + s - 2, cy, x + s - 2, y + 3)
