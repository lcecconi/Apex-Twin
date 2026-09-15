"""
WS2812 RGB LED Strip Widget for Apex-Dash Emulator
Renders 5 Progressive RPM Shift LEDs + 2 Multi-Color Alarm LEDs with realistic glow.
"""

import math
import time
from PySide6.QtCore import Qt, QTimer, QRectF
from PySide6.QtGui import QColor, QPainter, QRadialGradient, QBrush, QPen
from PySide6.QtWidgets import QWidget
from emu.core.telemetry_model import SystemSettings, TelemetrySnapshot, RpmDisplayMode


class LedBarWidget(QWidget):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setMinimumHeight(44)
        self.setMaximumHeight(44)
        self.led_colors = [QColor(30, 30, 30)] * 7
        self.last_strobe_toggle = time.time()
        self.strobe_state = False
        self.in_test_mode = False
        self.test_start_time = 0.0

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
                for i in range(7):
                    hue = int(((elapsed * 360.0) + (i * 360.0 / 7.0))) % 360
                    self.led_colors[i] = QColor.fromHsv(hue, 255, 255)
                self.update()
                return

        brightness_scale = settings.led_brightness / 100.0

        # 1. Shift Lights (LEDs 0..4)
        if settings.led_shift_enable and settings.rpm_display_mode != RpmDisplayMode.DISPLAY_ONLY:
            shift_rpm = settings.shift_rpm
            rpm = telemetry.rpm

            if rpm >= shift_rpm:
                # Shift point reached -> Flash all 5 in Bright Cyan / White Strobe
                strobe_col = QColor(0, 180, 255) if self.strobe_state else QColor(20, 20, 25)
                for i in range(5):
                    self.led_colors[i] = strobe_col
            else:
                rpm_start = max(0, shift_rpm - 1600)
                step = (shift_rpm - rpm_start) / 4.0

                # LED 0: Green
                self.led_colors[0] = QColor(0, 255, 0) if rpm >= rpm_start else QColor(25, 30, 25)
                # LED 1: Green
                self.led_colors[1] = QColor(0, 255, 0) if rpm >= rpm_start + step else QColor(25, 30, 25)
                # LED 2: Yellow
                self.led_colors[2] = QColor(255, 210, 0) if rpm >= rpm_start + step * 2 else QColor(30, 30, 25)
                # LED 3: Amber / Orange
                self.led_colors[3] = QColor(255, 120, 0) if rpm >= rpm_start + step * 3 else QColor(30, 25, 25)
                # LED 4: Red
                self.led_colors[4] = QColor(255, 0, 0) if rpm >= shift_rpm - 50 else QColor(30, 20, 20)
        else:
            for i in range(5):
                self.led_colors[i] = QColor(20, 20, 20)

        # 2. Alarm Lights (LED 5 = Left Alarm, LED 6 = Right Alarm)
        if settings.led_alarm_enable:
            # Left Alarm: Water Overheat (> threshold) or Low Battery (< 3.4V)
            if telemetry.water_temp_c >= settings.water_temp_alarm_c and settings.water_temp_alarm_c > 0:
                self.led_colors[5] = QColor(255, 0, 0) if self.strobe_state else QColor(25, 20, 20)
            elif telemetry.battery_voltage < settings.low_bat_alarm_v and telemetry.battery_voltage > 1.0:
                self.led_colors[5] = QColor(255, 120, 0)
            else:
                self.led_colors[5] = QColor(25, 25, 25)

            # Right Alarm: High EGT (> threshold) or Over-Rev (> threshold)
            if telemetry.exhaust_temp_c >= settings.exhaust_temp_alarm_c and settings.exhaust_temp_alarm_c > 0:
                self.led_colors[6] = QColor(255, 0, 220) if self.strobe_state else QColor(25, 20, 25)
            elif telemetry.rpm >= settings.over_rev_rpm and settings.over_rev_rpm > 0:
                self.led_colors[6] = QColor(255, 255, 255) if self.strobe_state else QColor(255, 0, 0)
            else:
                self.led_colors[6] = QColor(25, 25, 25)
        else:
            self.led_colors[5] = QColor(20, 20, 20)
            self.led_colors[6] = QColor(20, 20, 20)

        # Apply global brightness
        for i in range(7):
            c = self.led_colors[i]
            self.led_colors[i] = QColor(
                int(c.red() * brightness_scale),
                int(c.green() * brightness_scale),
                int(c.blue() * brightness_scale)
            )

        self.update()

    def paintEvent(self, event):
        painter = QPainter(self)
        painter.setRenderHint(QPainter.Antialiasing)

        width = self.width()
        height = self.height()

        # LED Layout positions:
        # Left Alarm (LED 5), 5 Shift LEDs (LEDs 0..4), Right Alarm (LED 6)
        led_spacing = 38
        start_x = (width - (6 * led_spacing)) / 2.0
        cy = height / 2.0
        radius = 11.0

        draw_order = [5, 0, 1, 2, 3, 4, 6]  # Physical layout on steering wheel

        for idx, led_idx in enumerate(draw_order):
            cx = start_x + (idx * led_spacing)
            color = self.led_colors[led_idx]
            is_active = (color.red() > 40 or color.green() > 40 or color.blue() > 40)

            # Draw outer bezel ring
            painter.setPen(QPen(QColor(45, 45, 55), 1.5))
            painter.setBrush(QBrush(QColor(18, 18, 22)))
            painter.drawEllipse(QRectF(cx - radius - 2, cy - radius - 2, (radius + 2) * 2, (radius + 2) * 2))

            # Draw radial glow halo if lit
            if is_active:
                glow = QRadialGradient(cx, cy, radius * 2.2)
                glow_color = QColor(color.red(), color.green(), color.blue(), 140)
                glow.setColorAt(0.0, glow_color)
                glow.setColorAt(1.0, QColor(0, 0, 0, 0))
                painter.setPen(Qt.NoPen)
                painter.setBrush(QBrush(glow))
                painter.drawEllipse(QRectF(cx - radius * 2.2, cy - radius * 2.2, radius * 4.4, radius * 4.4))

            # Draw inner LED lens with reflection highlight
            lens_gradient = QRadialGradient(cx - radius * 0.3, cy - radius * 0.3, radius)
            lens_gradient.setColorAt(0.0, color.lighter(130) if is_active else QColor(40, 40, 45))
            lens_gradient.setColorAt(1.0, color if is_active else QColor(15, 15, 20))

            painter.setPen(QPen(QColor(0, 0, 0, 180), 1.0))
            painter.setBrush(QBrush(lens_gradient))
            painter.drawEllipse(QRectF(cx - radius, cy - radius, radius * 2, radius * 2))

            # Small specular reflection point
            painter.setPen(Qt.NoPen)
            painter.setBrush(QBrush(QColor(255, 255, 255, 160 if is_active else 40)))
            painter.drawEllipse(QRectF(cx - radius * 0.5, cy - radius * 0.5, radius * 0.45, radius * 0.35))
