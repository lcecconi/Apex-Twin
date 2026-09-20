"""
WS2812 RGB LED Strip Widget for Apex-Dash Emulator
Renders 16 Progressive RPM Shift LEDs with CNC milled housing and realistic glow.
"""

import time
from PySide6.QtCore import Qt, QSize, QRectF
from PySide6.QtGui import QColor, QPainter, QRadialGradient, QBrush, QPen
from PySide6.QtWidgets import QWidget
from emu.core.telemetry_model import SystemSettings, TelemetrySnapshot, RpmDisplayMode


class LedBarWidget(QWidget):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.num_leds = 16
        self.setMinimumSize(400, 42)
        self.setFixedHeight(42)

        self.led_colors = [QColor(20, 25, 30)] * self.num_leds
        self.led_active = [False] * self.num_leds
        self.last_strobe_toggle = time.time()
        self.strobe_state = False
        self.in_test_mode = False
        self.test_start_time = 0.0

    def sizeHint(self) -> QSize:
        return QSize(440, 42)

    def minimumSizeHint(self) -> QSize:
        return QSize(400, 42)

    def trigger_test_pattern(self):
        self.in_test_mode = True
        self.test_start_time = time.time()

    def update_leds(self, telemetry: TelemetrySnapshot, settings: SystemSettings):
        now = time.time()

        # Toggle strobe state at ~16 Hz (60ms)
        if now - self.last_strobe_toggle >= 0.06:
            self.strobe_state = not self.strobe_state
            self.last_strobe_toggle = now

        # Test pattern mode
        if self.in_test_mode:
            if now - self.test_start_time > 2.0:
                self.in_test_mode = False
            else:
                elapsed = now - self.test_start_time
                for i in range(self.num_leds):
                    hue = int(((elapsed * 360.0) + (i * 360.0 / self.num_leds))) % 360
                    self.led_colors[i] = QColor.fromHsv(hue, 255, 255)
                    self.led_active[i] = True
                self.update()
                return

        brightness_scale = max(0.1, settings.led_brightness / 100.0)

        # 1. Shift Lights (16 LEDs: 0..15 mapped to progressive RPM bar)
        if settings.led_shift_enable and settings.rpm_display_mode != RpmDisplayMode.DISPLAY_ONLY:
            shift_rpm = settings.shift_rpm
            rpm = telemetry.rpm

            if rpm >= shift_rpm and shift_rpm > 0:
                # Shift point reached -> Flash all 16 in Bright Cyan / White Strobe
                strobe_col = QColor(0, 220, 255) if self.strobe_state else QColor(20, 25, 35)
                for i in range(self.num_leds):
                    self.led_colors[i] = strobe_col
                    self.led_active[i] = self.strobe_state
            elif rpm > 100:
                # Linear mapping: First LED lights up at > 100 RPM, then linearly up to max_rpm
                max_rpm = settings.max_rpm if settings.max_rpm > 100 else 16000
                span_rpm = max_rpm - 100
                curr_rpm = min(span_rpm, rpm - 100)
                active_leds = 1 + int((curr_rpm * (self.num_leds - 1)) / span_rpm)
                active_leds = min(self.num_leds, active_leds)

                for i in range(self.num_leds):
                    if i < active_leds:
                        self.led_active[i] = True
                        # 6 Green, 5 Yellow/Amber, 5 Red
                        if i < 6:
                            self.led_colors[i] = QColor(0, 255, 50)       # Vivid Green
                        elif i < 11:
                            self.led_colors[i] = QColor(255, 200, 0)     # Amber / Yellow
                        else:
                            self.led_colors[i] = QColor(255, 20, 20)     # Vivid Red
                    else:
                        self.led_active[i] = False
                        self.led_colors[i] = QColor(25, 30, 38)
            else:
                for i in range(self.num_leds):
                    self.led_active[i] = False
                    self.led_colors[i] = QColor(25, 30, 38)
        else:
            for i in range(self.num_leds):
                self.led_active[i] = False
                self.led_colors[i] = QColor(25, 30, 38)

        # Apply global brightness to active colors
        for i in range(self.num_leds):
            if self.led_active[i]:
                c = self.led_colors[i]
                self.led_colors[i] = QColor(
                    min(255, int(c.red() * brightness_scale)),
                    min(255, int(c.green() * brightness_scale)),
                    min(255, int(c.blue() * brightness_scale))
                )

        self.update()

    def paintEvent(self, event):
        painter = QPainter(self)
        painter.setRenderHint(QPainter.Antialiasing)

        width = self.width()
        height = self.height()

        led_spacing = 22.0
        total_span = (self.num_leds - 1) * led_spacing
        start_x = (width - total_span) / 2.0
        cy = height / 2.0
        radius = 7.5

        # Draw CNC milled housing tray / bay
        tray_rect = QRectF(start_x - 16, cy - 14, total_span + 32, 28)
        painter.setPen(QPen(QColor(50, 56, 68), 1.5))
        painter.setBrush(QBrush(QColor(12, 14, 18)))
        painter.drawRoundedRect(tray_rect, 14, 14)

        for i in range(self.num_leds):
            cx = start_x + (i * led_spacing)
            color = self.led_colors[i]
            is_active = self.led_active[i]

            # Outer chrome / bezel ring
            painter.setPen(QPen(QColor(70, 78, 92) if is_active else QColor(40, 45, 55), 1.5))
            painter.setBrush(QBrush(QColor(18, 20, 26)))
            painter.drawEllipse(QRectF(cx - radius - 2, cy - radius - 2, (radius + 2) * 2, (radius + 2) * 2))

            # Radial glow halo if lit
            if is_active:
                glow = QRadialGradient(cx, cy, radius * 2.4)
                glow_color = QColor(color.red(), color.green(), color.blue(), 160)
                glow.setColorAt(0.0, glow_color)
                glow.setColorAt(1.0, QColor(0, 0, 0, 0))
                painter.setPen(Qt.NoPen)
                painter.setBrush(QBrush(glow))
                painter.drawEllipse(QRectF(cx - radius * 2.4, cy - radius * 2.4, radius * 4.8, radius * 4.8))

            # Inner LED lens with reflection gradient
            lens_gradient = QRadialGradient(cx - radius * 0.3, cy - radius * 0.3, radius)
            if is_active:
                lens_gradient.setColorAt(0.0, color.lighter(140))
                lens_gradient.setColorAt(1.0, color)
            else:
                lens_gradient.setColorAt(0.0, QColor(45, 50, 62))
                lens_gradient.setColorAt(1.0, QColor(20, 23, 30))

            painter.setPen(QPen(QColor(0, 0, 0, 200), 1.0))
            painter.setBrush(QBrush(lens_gradient))
            painter.drawEllipse(QRectF(cx - radius, cy - radius, radius * 2, radius * 2))

            # Specular lens highlight
            painter.setPen(Qt.NoPen)
            painter.setBrush(QBrush(QColor(255, 255, 255, 180 if is_active else 50)))
            painter.drawEllipse(QRectF(cx - radius * 0.5, cy - radius * 0.5, radius * 0.45, radius * 0.35))
