"""
4.2" Reflective LCD (400x300) Screen Renderer for Apex-Dash Emulator
Pixel-accurate rendering of all 4 racing HUD pages and 7 setup submenus.
"""

import time
from enum import IntEnum
from PySide6.QtCore import Qt, QRectF, QPointF
from PySide6.QtGui import QColor, QPainter, QFont, QPen, QBrush
from PySide6.QtWidgets import QWidget

from emu.core.telemetry_model import SystemSettings, TelemetrySnapshot, DriveType, RpmDisplayMode
from emu.core.i18n import I18n, StrId
from emu.ui.icons import (
    draw_xbm,
    ICON_WATER_16X16,
    ICON_EGT_16X16,
    ICON_REV_16X16,
    ICON_BAT_16X16,
    ICON_LINK_16X16,
    ICON_GEAR_16X16,
    ICON_LIGHTBULB_16X16,
    ICON_FLAG_16X16,
    ICON_SDCARD_16X16,
    ICON_SUN_16X16,
    ICON_GLOBE_16X16,
    ICON_WRENCH_16X16,
    ICON_BACK_16X16,
    ICON_ENGINE_16X16,
    ICON_WARN_24X24,
)


class UiViewMode(IntEnum):

    VIEW_LIVE_RACE = 0
    VIEW_TELEMETRY = 1
    VIEW_GPS_PADDOCK = 2
    VIEW_DATA_RECALL = 3


class MenuState(IntEnum):
    MENU_ROOT = 0
    MENU_RACE_SETUP = 1
    MENU_LEDS_ALARMS = 2
    MENU_TRACK_GPS = 3
    MENU_STORAGE_PC = 4
    MENU_DISPLAY_PWM = 5
    MENU_SYSTEM_LANG = 6
    MENU_DIAGNOSTICS = 7
    MENU_USB_MSC_SCREEN = 8
    MENU_WARN_TRIGGERS = 9



class RlcdRenderer(QWidget):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setMinimumSize(400, 300)
        from PySide6.QtWidgets import QSizePolicy
        self.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)

        self.current_view = UiViewMode.VIEW_LIVE_RACE
        self.menu_active = False
        self.edit_mode = False
        self.menu_state = MenuState.MENU_ROOT
        self.cursor_idx = 0

        self.telemetry = TelemetrySnapshot()
        self.settings = SystemSettings()

    def set_data(self, telemetry: TelemetrySnapshot, settings: SystemSettings):
        self.telemetry = telemetry
        self.settings = settings
        self.update()

    # --- Button / Input Navigation Handlers ---
    def handle_key_short(self):
        """KEY Button Short Press (Next Page in Race / Next Item or + Increase in Menu)"""
        if self.menu_active:
            if self.edit_mode:
                # Increase parameter (+500 RPM / +step)
                if self.menu_state == MenuState.MENU_RACE_SETUP:
                    if self.cursor_idx == 1:
                        self.settings.max_rpm = min(22000, self.settings.max_rpm + 500)
                    elif self.cursor_idx == 2:
                        self.settings.shift_rpm = min(20000, self.settings.shift_rpm + 500)
                    elif self.cursor_idx == 3:
                        self.settings.over_rev_rpm = min(22000, self.settings.over_rev_rpm + 500)
                elif self.menu_state == MenuState.MENU_LEDS_ALARMS:
                    if self.cursor_idx == 0:
                        self.settings.led_brightness = min(100, self.settings.led_brightness + 10)
                    elif self.cursor_idx == 5:
                        self.settings.water_temp_alarm_c = min(95.0, self.settings.water_temp_alarm_c + 5.0)
                    elif self.cursor_idx == 6:
                        self.settings.over_rev_rpm = min(22000, self.settings.over_rev_rpm + 500)
                elif self.menu_state == MenuState.MENU_DISPLAY_PWM:
                    if self.cursor_idx == 0:
                        self.settings.backlight_percent = min(100, self.settings.backlight_percent + 10)
            else:
                max_items = self._get_menu_item_count()
                self.cursor_idx = (self.cursor_idx + 1) % max_items
        else:
            self.current_view = UiViewMode((self.current_view + 1) % 4)
        self.update()

    def handle_key_long(self):
        """KEY Button Long Press (Invert Polarity in Race / Select / Save in Menu)"""
        if self.menu_active:
            if self.edit_mode:
                self.edit_mode = False
            else:
                self._handle_menu_select()
        else:
            self.settings.inverted_display = not self.settings.inverted_display
        self.update()

    def handle_boot_short(self):
        """BOOT Button Short Press (Prev Page in Race / Prev Item or - Decrease in Menu)"""
        if self.menu_active:
            if self.edit_mode:
                # Decrease parameter (-500 RPM / -step)
                if self.menu_state == MenuState.MENU_RACE_SETUP:
                    if self.cursor_idx == 1:
                        self.settings.max_rpm = max(8000, self.settings.max_rpm - 500)
                    elif self.cursor_idx == 2:
                        self.settings.shift_rpm = max(6000, self.settings.shift_rpm - 500)
                    elif self.cursor_idx == 3:
                        self.settings.over_rev_rpm = max(8000, self.settings.over_rev_rpm - 500)
                elif self.menu_state == MenuState.MENU_LEDS_ALARMS:
                    if self.cursor_idx == 0:
                        self.settings.led_brightness = max(10, self.settings.led_brightness - 10)
                    elif self.cursor_idx == 5:
                        self.settings.water_temp_alarm_c = max(40.0, self.settings.water_temp_alarm_c - 5.0)
                    elif self.cursor_idx == 6:
                        self.settings.over_rev_rpm = max(8000, self.settings.over_rev_rpm - 500)
                elif self.menu_state == MenuState.MENU_DISPLAY_PWM:
                    if self.cursor_idx == 0:
                        self.settings.backlight_percent = max(0, self.settings.backlight_percent - 10)
            else:
                max_items = self._get_menu_item_count()
                self.cursor_idx = (self.cursor_idx - 1 + max_items) % max_items
        else:
            self.current_view = UiViewMode((self.current_view - 1 + 4) % 4)
        self.update()

    def handle_boot_long(self):
        """BOOT Button Long Press (Open/Exit Menu or Exit Edit Mode)"""
        if self.edit_mode:
            self.edit_mode = False
            self.update()
            return

        if not self.menu_active:
            self.menu_active = True
            self.menu_state = MenuState.MENU_ROOT
            self.cursor_idx = 0
        else:
            if self.menu_state == MenuState.MENU_ROOT or self.menu_state == MenuState.MENU_USB_MSC_SCREEN:
                self.menu_active = False
            elif self.menu_state == MenuState.MENU_WARN_TRIGGERS:
                self.menu_state = MenuState.MENU_LEDS_ALARMS
                self.cursor_idx = 7
            else:
                self.menu_state = MenuState.MENU_ROOT
                self.cursor_idx = 0
        self.update()

    def _get_menu_item_count(self) -> int:
        if self.menu_state == MenuState.MENU_ROOT:
            return 8
        elif self.menu_state == MenuState.MENU_RACE_SETUP:
            return 5
        elif self.menu_state == MenuState.MENU_LEDS_ALARMS:
            return 9
        elif self.menu_state == MenuState.MENU_WARN_TRIGGERS:
            return 8
        elif self.menu_state == MenuState.MENU_TRACK_GPS:
            return 6
        elif self.menu_state == MenuState.MENU_STORAGE_PC:
            return 2
        elif self.menu_state == MenuState.MENU_DISPLAY_PWM:
            return 6
        elif self.menu_state == MenuState.MENU_SYSTEM_LANG:
            return 3
        return 1

    def _handle_menu_select(self):
        if self.menu_state == MenuState.MENU_ROOT:
            if self.cursor_idx == 0:
                self.menu_state = MenuState.MENU_RACE_SETUP
                self.cursor_idx = 0
            elif self.cursor_idx == 1:
                self.menu_state = MenuState.MENU_LEDS_ALARMS
                self.cursor_idx = 0
            elif self.cursor_idx == 2:
                self.menu_state = MenuState.MENU_TRACK_GPS
                self.cursor_idx = 0
            elif self.cursor_idx == 3:
                self.menu_state = MenuState.MENU_STORAGE_PC
                self.cursor_idx = 0
            elif self.cursor_idx == 4:
                self.menu_state = MenuState.MENU_DISPLAY_PWM
                self.cursor_idx = 0
            elif self.cursor_idx == 5:
                self.menu_state = MenuState.MENU_SYSTEM_LANG
                self.cursor_idx = 0
            elif self.cursor_idx == 6:
                self.menu_state = MenuState.MENU_DIAGNOSTICS
                self.cursor_idx = 0
            elif self.cursor_idx == 7:
                self.menu_active = False
        elif self.menu_state == MenuState.MENU_RACE_SETUP:
            if self.cursor_idx == 0:
                self.settings.drive_type = DriveType((self.settings.drive_type + 1) % 3)
            elif self.cursor_idx in (1, 2, 3):
                self.edit_mode = True
            else:
                self.menu_state = MenuState.MENU_ROOT
                self.cursor_idx = 0
        elif self.menu_state == MenuState.MENU_LEDS_ALARMS:
            if self.cursor_idx == 0:
                self.edit_mode = True
            elif self.cursor_idx == 1:
                self.settings.rpm_display_mode = RpmDisplayMode((self.settings.rpm_display_mode + 1) % 3)
            elif self.cursor_idx == 3:
                self.settings.led_shift_enable = not self.settings.led_shift_enable
            elif self.cursor_idx == 4:
                self.settings.led_alarm_enable = not self.settings.led_alarm_enable
            elif self.cursor_idx in (5, 6):
                self.edit_mode = True
            elif self.cursor_idx == 7:
                self.menu_state = MenuState.MENU_WARN_TRIGGERS
                self.cursor_idx = 0
            else:
                self.menu_state = MenuState.MENU_ROOT
                self.cursor_idx = 1
        elif self.menu_state == MenuState.MENU_WARN_TRIGGERS:
            if self.cursor_idx == 0:
                self.settings.warn_trigger_water = not self.settings.warn_trigger_water
            elif self.cursor_idx == 1:
                self.settings.warn_trigger_egt = not self.settings.warn_trigger_egt
            elif self.cursor_idx == 2:
                self.settings.warn_trigger_rev = not self.settings.warn_trigger_rev
            elif self.cursor_idx == 3:
                self.settings.warn_trigger_battery = not self.settings.warn_trigger_battery
            elif self.cursor_idx == 4:
                self.settings.warn_trigger_link = not self.settings.warn_trigger_link
            elif self.cursor_idx == 5:
                self.settings.warn_trigger_water = True
                self.settings.warn_trigger_egt = True
                self.settings.warn_trigger_rev = True
                self.settings.warn_trigger_battery = True
                self.settings.warn_trigger_link = True
            elif self.cursor_idx == 6:
                self.settings.warn_trigger_water = False
                self.settings.warn_trigger_egt = False
                self.settings.warn_trigger_rev = False
                self.settings.warn_trigger_battery = False
                self.settings.warn_trigger_link = False
            else:
                self.menu_state = MenuState.MENU_LEDS_ALARMS
                self.cursor_idx = 7


        elif self.menu_state == MenuState.MENU_STORAGE_PC:
            if self.cursor_idx == 0:
                self.menu_state = MenuState.MENU_USB_MSC_SCREEN
            else:
                self.menu_state = MenuState.MENU_ROOT
                self.cursor_idx = 3
        elif self.menu_state == MenuState.MENU_DISPLAY_PWM:
            if self.cursor_idx == 0:
                self.edit_mode = True
            elif self.cursor_idx == 1:
                self.settings.show_speed = not self.settings.show_speed
            elif self.cursor_idx == 2:
                self.settings.inverted_display = not self.settings.inverted_display
            elif self.cursor_idx == 3:
                self.settings.use_kmh = not self.settings.use_kmh
            elif self.cursor_idx == 4:
                self.settings.use_celsius = not self.settings.use_celsius
            else:
                self.menu_state = MenuState.MENU_ROOT
                self.cursor_idx = 4
        elif self.menu_state == MenuState.MENU_SYSTEM_LANG:
            if self.cursor_idx == 0:
                self.settings.language = (self.settings.language + 1) % 4
                I18n.set_language(self.settings.language)
            else:
                self.menu_state = MenuState.MENU_ROOT
                self.cursor_idx = 5
        elif self.menu_state in (MenuState.MENU_TRACK_GPS, MenuState.MENU_DIAGNOSTICS, MenuState.MENU_USB_MSC_SCREEN):
            self.menu_state = MenuState.MENU_ROOT
            self.cursor_idx = 0


    # --- Paint Event ---
    def paintEvent(self, event):
        painter = QPainter(self)
        painter.setRenderHint(QPainter.Antialiasing, False)

        w = self.width()
        h = self.height()
        scale = min(w / 400.0, h / 300.0)

        target_w = 400.0 * scale
        target_h = 300.0 * scale
        offset_x = (w - target_w) / 2.0
        offset_y = (h - target_h) / 2.0

        # Fill background letterbox
        painter.fillRect(0, 0, w, h, QColor(0, 0, 0))

        painter.save()
        painter.translate(offset_x, offset_y)
        painter.scale(scale, scale)

        # Reflective LCD Paper/Silver Palettes
        if self.settings.inverted_display:
            bg_color = QColor(228, 232, 230)  # Silver/Grey paper background
            fg_color = QColor(15, 18, 20)      # High-density black pixels
        else:
            bg_color = QColor(20, 22, 25)      # Deep Black
            fg_color = QColor(210, 215, 212)  # Silver pixels

        painter.fillRect(0, 0, 400, 300, bg_color)
        painter.setPen(QPen(fg_color, 1))

        if self.menu_active:
            self._render_menu(painter, bg_color, fg_color)
        else:
            if self.current_view == UiViewMode.VIEW_LIVE_RACE:
                self._render_live_race(painter, bg_color, fg_color)
            elif self.current_view == UiViewMode.VIEW_TELEMETRY:
                self._render_telemetry(painter, bg_color, fg_color)
            elif self.current_view == UiViewMode.VIEW_GPS_PADDOCK:
                self._render_paddock(painter, bg_color, fg_color)
            elif self.current_view == UiViewMode.VIEW_DATA_RECALL:
                self._render_data_recall(painter, bg_color, fg_color)

            # Footer
            self._render_footer(painter, bg_color, fg_color)

        painter.restore()

    # --- View Renderers ---
    def _render_live_race(self, p: QPainter, bg: QColor, fg: QColor):
        t = self.telemetry
        s = self.settings

        # 1. Top Tachometer Bar (Clean bar without text)
        if s.rpm_display_mode != RpmDisplayMode.LEDS_ONLY:
            p.drawRoundedRect(6, 4, 388, 26, 3, 3)
            shift_x = int(6 + (s.shift_rpm * 384 / max(1, s.max_rpm)))
            if shift_x < 392:
                p.drawLine(shift_x, 2, shift_x, 30)

            rpm_fill = int(t.rpm * 384 / max(1, s.max_rpm))
            rpm_fill = max(0, min(384, rpm_fill))
            if rpm_fill > 0:
                p.fillRect(8, 6, rpm_fill, 22, fg)


        # 2 & 3. Speed/Gear & Lap Time
        has_left_pane = s.show_speed or (s.drive_type == DriveType.SHIFTER_6SPEED)
        is_shift = (t.rpm >= s.shift_rpm and s.shift_rpm > 0)
        shift_blink = is_shift and (int(time.time() * 6.6) % 2 == 0)

        if has_left_pane:
            p.drawRoundedRect(10, 38, 160, 118, 4, 4)
            if s.show_speed:
                disp_speed = t.speed_kmh if s.use_kmh else t.speed_kmh * 0.621371
                speed_unit = "KM/H" if s.use_kmh else "MPH"

                if s.drive_type == DriveType.SHIFTER_6SPEED:
                    # 6-Speed Shifter Kart: Dominant Gear on LEFT, Speed on RIGHT
                    # Left: Gear Box
                    if shift_blink:
                        p.fillRect(18, 50, 60, 96, fg)
                        p.setPen(bg)
                    else:
                        p.drawRoundedRect(18, 50, 60, 96, 4, 4)

                    gear_str = "N" if t.gear == 0 else str(t.gear)
                    if t.gear == 0:
                        p.setFont(QFont("SansSerif", 42, QFont.Bold))
                    else:
                        p.setFont(QFont("SansSerif", 50, QFont.Bold))
                    p.drawText(QRectF(18, 50, 60, 96), Qt.AlignCenter, gear_str)

                    if shift_blink:
                        p.setPen(fg)

                    # Right: Speed Display
                    p.setFont(QFont("SansSerif", 10, QFont.Bold))
                    p.drawText(QRectF(82, 52, 82, 20), Qt.AlignCenter, speed_unit)

                    p.setFont(QFont("SansSerif", 34, QFont.Bold))
                    p.drawText(QRectF(82, 74, 82, 68), Qt.AlignCenter, f"{int(disp_speed):03d}")
                else:
                    # Single Speed (Direct Drive / Clutch) — Large Centered Speed
                    p.setFont(QFont("SansSerif", 46, QFont.Bold))
                    p.drawText(QRectF(10, 48, 160, 58), Qt.AlignCenter, f"{int(disp_speed):03d}")

                    p.setFont(QFont("SansSerif", 10, QFont.Bold))
                    p.drawText(QRectF(10, 116, 160, 24), Qt.AlignCenter, speed_unit)
            else:
                # Speed Hidden Mode (Shifter Kart)
                if shift_blink:
                    p.fillRect(10, 38, 160, 118, fg)
                    p.setPen(bg)

                gear_str = "N" if t.gear == 0 else str(t.gear)
                if t.gear == 0:
                    p.setFont(QFont("SansSerif", 48, QFont.Bold))
                else:
                    p.setFont(QFont("SansSerif", 72, QFont.Bold))
                p.drawText(QRectF(10, 38, 160, 118), Qt.AlignCenter, gear_str)

                if shift_blink:
                    p.setPen(fg)

            # Standard Lap Time (Right Pane)
            p.drawRoundedRect(176, 38, 214, 118, 4, 4)
            p.setFont(QFont("SansSerif", 9, QFont.Bold))
            p.drawText(186, 56, f"{I18n.get(StrId.LABEL_LAP)} {t.lap_number:02d}  [{I18n.get(StrId.LABEL_SECTOR)} {t.current_sector}]")

            lap_min = t.current_lap_time_ms // 60000
            lap_sec = (t.current_lap_time_ms % 60000) // 1000
            lap_cen = (t.current_lap_time_ms % 1000) // 10
            p.setFont(QFont("SansSerif", 30, QFont.Bold))
            p.drawText(184, 102, f"{lap_min:02d}:{lap_sec:02d}.{lap_cen:02d}")

            # Best Lap reference (Inverted Badge - Double Size)
            p.setFont(QFont("SansSerif", 10, QFont.Bold))
            if t.best_lap_time_ms > 0:
                b_sec = (t.best_lap_time_ms % 60000) // 1000
                b_cen = (t.best_lap_time_ms % 1000) // 10
                best_str = f" {I18n.get(StrId.LABEL_BEST)} {b_sec:02d}.{b_cen:02d}s "
            else:
                best_str = f" {I18n.get(StrId.LABEL_BEST)} --.--s "

            fm = p.fontMetrics()
            bw = fm.horizontalAdvance(best_str) + 4
            p.fillRect(182, 122, bw, 22, fg)
            p.setPen(bg)
            p.drawText(QRectF(182, 122, bw, 22), Qt.AlignCenter, best_str)
            p.setPen(fg)

            # Last Lap reference (Normal text - Double Size)
            p.setFont(QFont("SansSerif", 10, QFont.Bold))
            if t.last_lap_time_ms > 0:
                l_sec = (t.last_lap_time_ms % 60000) // 1000
                l_cen = (t.last_lap_time_ms % 1000) // 10
                last_str = f"{I18n.get(StrId.LABEL_LAST)}: {l_sec:02d}.{l_cen:02d}s"
            else:
                last_str = f"{I18n.get(StrId.LABEL_LAST)}: --.--s"
            p.drawText(QRectF(182 + bw + 4, 122, 384 - (182 + bw + 4), 22), Qt.AlignRight | Qt.AlignVCenter, last_str)
        else:
            # Full-Width Lap Time Pane (380 px width)
            p.drawRoundedRect(10, 38, 380, 118, 4, 4)
            p.setFont(QFont("SansSerif", 10, QFont.Bold))
            p.drawText(24, 58, f"{I18n.get(StrId.LABEL_LAP)} {t.lap_number:02d}  [{I18n.get(StrId.LABEL_SECTOR)} {t.current_sector}]")

            lap_min = t.current_lap_time_ms // 60000
            lap_sec = (t.current_lap_time_ms % 60000) // 1000
            lap_cen = (t.current_lap_time_ms % 1000) // 10
            p.setFont(QFont("SansSerif", 42, QFont.Bold))
            p.drawText(QRectF(10, 64, 380, 52), Qt.AlignCenter, f"{lap_min:02d}:{lap_sec:02d}.{lap_cen:02d}")

            # Best Lap reference (Inverted Badge - Double Size)
            p.setFont(QFont("SansSerif", 12, QFont.Bold))
            if t.best_lap_time_ms > 0:
                b_sec = (t.best_lap_time_ms % 60000) // 1000
                b_cen = (t.best_lap_time_ms % 1000) // 10
                best_str = f" {I18n.get(StrId.LABEL_BEST)} {b_sec:02d}.{b_cen:02d}s "
            else:
                best_str = f" {I18n.get(StrId.LABEL_BEST)} --.--s "

            fm = p.fontMetrics()
            bw = fm.horizontalAdvance(best_str) + 6
            p.fillRect(22, 122, bw, 24, fg)
            p.setPen(bg)
            p.drawText(QRectF(22, 122, bw, 24), Qt.AlignCenter, best_str)
            p.setPen(fg)

            # Last Lap reference (Normal text - Double Size)
            if t.last_lap_time_ms > 0:
                l_sec = (t.last_lap_time_ms % 60000) // 1000
                l_cen = (t.last_lap_time_ms % 1000) // 10
                last_str = f"{I18n.get(StrId.LABEL_LAST)}: {l_sec:02d}.{l_cen:02d}s"
            else:
                last_str = f"{I18n.get(StrId.LABEL_LAST)}: --.--s"
            p.drawText(QRectF(200, 122, 176, 24), Qt.AlignRight | Qt.AlignVCenter, last_str)

        # 4. Predictive Delta (Left) & System Alarms (Right)
        # Left Pane: Predictive Delta Bar (185 px width)
        p.drawRoundedRect(10, 162, 185, 52, 4, 4)
        p.setFont(QFont("SansSerif", 9, QFont.Bold))
        p.drawText(16, 178, f"{I18n.get(StrId.LABEL_PRED)} {I18n.get(StrId.LABEL_DELTA)}")

        p.setFont(QFont("SansSerif", 10, QFont.Bold))
        p.drawText(QRectF(100, 164, 90, 16), Qt.AlignRight | Qt.AlignVCenter, f"{t.predictive_delta_s:+0.2f} s")

        center_x = 102
        p.drawRect(18, 186, 169, 16)
        p.drawLine(center_x, 182, center_x, 206)

        delta_px = int(t.predictive_delta_s * 80.0)
        delta_px = max(-80, min(80, delta_px))
        if delta_px < 0:
            p.fillRect(center_x + delta_px, 188, -delta_px, 12, fg)
        elif delta_px > 0:
            p.fillRect(center_x, 188, delta_px, 12, fg)

        # Right Pane: Alarms Grid (185 px width, no header text)
        p.drawRoundedRect(205, 162, 185, 52, 4, 4)

        alm_active = [
            t.water_temp_c >= s.water_temp_alarm_c and s.water_temp_alarm_c > 0,
            t.exhaust_temp_c >= s.exhaust_temp_alarm_c and s.exhaust_temp_alarm_c > 0,
            t.rpm >= s.over_rev_rpm and s.over_rev_rpm > 0,
            t.battery_voltage < s.low_bat_alarm_v and t.battery_voltage > 1.0,
            not t.track_module_connected,
        ]

        alarm_icons = [
            ICON_WATER_16X16,
            ICON_EGT_16X16,
            ICON_REV_16X16,
            ICON_BAT_16X16,
            ICON_LINK_16X16,
        ]

        for i, xbm in enumerate(alarm_icons):
            tx = 211 + (i * 35)
            ty = 171
            tw = 32
            th = 34

            if alm_active[i]:
                # Lit Up Alarm (Inverted Solid Fill)
                p.fillRect(tx, ty, tw, th, fg)
                draw_xbm(p, tx + (tw - 16) // 2, ty + (th - 16) // 2, xbm, 16, 16, color=bg)
            else:
                # Normally OFF (Outline Box)
                p.drawRoundedRect(tx, ty, tw, th, 4, 4)
                draw_xbm(p, tx + (tw - 16) // 2, ty + (th - 16) // 2, xbm, 16, 16, color=fg)

        # 5. Bottom Engine (Left) & Alarm Banner (Right)
        # Left: Water, EGT & Engine Runtime Pane (185 px width)
        p.drawRoundedRect(10, 222, 185, 50, 4, 4)

        w_temp = t.water_temp_c if s.use_celsius else (t.water_temp_c * 1.8 + 32.0)
        e_temp = t.exhaust_temp_c if s.use_celsius else (t.exhaust_temp_c * 1.8 + 32.0)
        t_unit = "°C" if s.use_celsius else "°F"

        # Col 1: Water & EGT temps
        draw_xbm(p, 14, 228, ICON_WATER_16X16, 16, 16, color=fg)
        p.setFont(QFont("SansSerif", 9, QFont.Bold))
        p.drawText(34, 241, f"{w_temp:.1f}{t_unit}")

        draw_xbm(p, 14, 249, ICON_EGT_16X16, 16, 16, color=fg)
        p.drawText(34, 263, f"{int(e_temp)}{t_unit}")

        # Vertical Separator
        p.drawLine(96, 226, 96, 268)

        # Col 2: Absolute Engine Runtime (hours:minutes)
        eng_hrs = t.engine_total_hours_sec // 3600
        eng_min = (t.engine_total_hours_sec % 3600) // 60
        draw_xbm(p, 104, 239, ICON_ENGINE_16X16, 16, 16, color=fg)
        p.setFont(QFont("SansSerif", 11, QFont.Bold))
        p.drawText(124, 253, f"{eng_hrs}:{eng_min:02d}")

        # Evaluate which alarms trigger the blinking WARN alert
        alm_warn = [
            alm_active[0] and s.warn_trigger_water,
            alm_active[1] and s.warn_trigger_egt,
            alm_active[2] and s.warn_trigger_rev,
            alm_active[3] and s.warn_trigger_battery,
            alm_active[4] and s.warn_trigger_link,
        ]
        any_warn = any(alm_warn)

        # Right: Flashing WARN Alert or System Status (185 px width)
        if any_warn:
            flash_state = int(time.time() * 3.3) % 2 == 0

            p.setFont(QFont("SansSerif", 22, QFont.Bold))
            fm = p.fontMetrics()
            w_warn = fm.horizontalAdvance("WARN")
            total_w = 24 + 10 + w_warn
            start_x = int(205 + (185 - total_w) / 2)
            icon_y = int(222 + (50 - 24) / 2)

            if flash_state:
                p.fillRect(205, 222, 185, 50, fg)
                draw_xbm(p, start_x, icon_y, ICON_WARN_24X24, 24, 24, color=bg)
                p.setPen(bg)
                p.setFont(QFont("SansSerif", 22, QFont.Bold))
                p.drawText(QRectF(start_x + 34, 222, w_warn + 10, 50), Qt.AlignVCenter | Qt.AlignLeft, "WARN")
                p.setPen(fg)
            else:
                p.drawRoundedRect(205, 222, 185, 50, 4, 4)
                draw_xbm(p, start_x, icon_y, ICON_WARN_24X24, 24, 24, color=fg)
                p.setFont(QFont("SansSerif", 22, QFont.Bold))
                p.drawText(QRectF(start_x + 34, 222, w_warn + 10, 50), Qt.AlignVCenter | Qt.AlignLeft, "WARN")
        else:
            p.drawRoundedRect(205, 222, 185, 50, 4, 4)
            p.setFont(QFont("SansSerif", 10, QFont.Bold))
            p.drawText(QRectF(205, 222, 185, 50), Qt.AlignCenter, "[ ALL SYSTEMS OK ]")

        # 6. Bottom Line (Track Info & Status)
        p.drawLine(0, 276, 400, 276)
        p.setFont(QFont("SansSerif", 8, QFont.Bold))
        p.drawText(8, 292, f"TRACK: {t.current_track_name}")
        link_str = "LINK OK" if t.track_module_connected else "SIM"
        right_str = f"BAT: {t.battery_voltage:.1f}V ({t.battery_percent}%) | {link_str}"
        p.drawText(QRectF(200, 280, 192, 16), Qt.AlignRight | Qt.AlignVCenter, right_str)


    def _render_telemetry(self, p: QPainter, bg: QColor, fg: QColor):

        t = self.telemetry
        s = self.settings
        p.setFont(QFont("SansSerif", 9, QFont.Bold))
        p.drawText(10, 20, "TELEMETRY & SENSOR MONITOR")
        p.drawText(300, 20, f"LAP {t.lap_number:02d} | SEC {t.current_sector}")
        p.drawLine(10, 26, 390, 26)

        # Card 1: Engine RPM & Gear
        p.drawRoundedRect(10, 32, 185, 110, 3, 3)
        p.fillRect(10, 32, 185, 16, fg)
        p.setPen(bg)
        header_title = "ENGINE RPM & GEAR" if s.drive_type == DriveType.SHIFTER_6SPEED else "ENGINE TACHOMETER"
        p.drawText(16, 44, header_title)
        p.setPen(fg)
        p.setFont(QFont("SansSerif", 24, QFont.Bold))
        p.drawText(16, 84, f"{t.rpm}")

        if s.drive_type == DriveType.SHIFTER_6SPEED:
            p.setFont(QFont("SansSerif", 9, QFont.Bold))
            gear_str = "N" if t.gear == 0 else str(t.gear)
            p.drawText(130, 72, f"Gear: {gear_str}")

        p.drawRect(16, 96, 172, 10)
        fill = int(t.rpm * 168 / max(1, self.settings.max_rpm))
        if fill > 0:
            p.fillRect(18, 98, min(168, fill), 6, fg)

        # Card 2: Dual Temps
        p.drawRoundedRect(205, 32, 185, 110, 3, 3)
        p.fillRect(205, 32, 185, 16, fg)
        p.setPen(bg)
        p.drawText(211, 44, "COOLANT & EXHAUST (EGT)")
        p.setPen(fg)
        p.setFont(QFont("SansSerif", 11, QFont.Bold))
        p.drawText(214, 76, f"H2O:  {t.water_temp_c:.1f} °C")
        p.drawText(214, 106, f"EGT:  {int(t.exhaust_temp_c)} °C")

        # Card 3: G-G Diagram
        p.drawRoundedRect(10, 148, 185, 118, 3, 3)
        p.fillRect(10, 148, 185, 16, fg)
        p.setPen(bg)
        p.drawText(16, 160, "ACCELERATION & G-FORCE")
        p.setPen(fg)

        gx, gy = 55, 208
        p.drawEllipse(gx - 28, gy - 28, 56, 56)
        p.drawLine(gx - 32, gy, gx + 32, gy)
        p.drawLine(gx, gy - 32, gx, gy + 32)

        dot_x = int(gx + t.lateral_g * 14.0)
        dot_y = int(gy - t.longitudinal_g * 14.0)
        p.fillRect(dot_x - 3, dot_y - 3, 6, 6, fg)

        p.setFont(QFont("Monospace", 7))
        p.drawText(100, 195, f"Lat: {t.lateral_g:+0.2f} G")
        p.drawText(100, 220, f"Lon: {t.longitudinal_g:+0.2f} G")
        p.drawText(100, 245, "Peak: 1.85 G")

        # Card 4: Timing & Splits
        p.drawRoundedRect(205, 148, 185, 118, 3, 3)
        p.fillRect(205, 148, 185, 16, fg)
        p.setPen(bg)
        p.drawText(211, 160, "TIMING & SECTORS")
        p.setPen(fg)
        p.setFont(QFont("SansSerif", 9, QFont.Bold))
        p.drawText(214, 186, f"Best: 48.42 s")
        p.drawText(214, 210, f"Last: 48.68 s")
        p.setFont(QFont("Monospace", 7))
        p.drawText(214, 232, "Sector 1: 16.08s [-0.12s]")
        p.drawText(214, 252, "Sector 2: 16.15s [+0.05s]")

    def _render_paddock(self, p: QPainter, bg: QColor, fg: QColor):
        t = self.telemetry
        p.setFont(QFont("SansSerif", 9, QFont.Bold))
        p.drawText(10, 20, "PADDOCK & PRE-RACE STATUS")
        p.drawText(290, 20, "[STANDBY MODE]")
        p.drawLine(10, 26, 390, 26)

        # Card 1: GNSS Radar
        p.drawRoundedRect(10, 32, 185, 130, 3, 3)
        p.fillRect(10, 32, 185, 16, fg)
        p.setPen(bg)
        p.drawText(16, 44, "GNSS SATELLITE RADAR")
        p.setPen(fg)

        sat_bars = [42, 38, 45, 30, 48, 44, 35, 41, 46, 39]
        for i, snr in enumerate(sat_bars):
            bx = 18 + (i * 17)
            bh = int(snr * 38 / 50.0)
            p.fillRect(bx, 96 - bh, 12, bh, fg)
        p.drawLine(16, 97, 188, 97)

        p.setFont(QFont("Monospace", 7))
        p.drawText(16, 114, f"Sats Visible: {t.satellites_visible} (GPS/Gal)")
        p.drawText(16, 132, f"Fix: 3D-DGPS | HDOP: {t.hdop:0.2f}")
        p.drawText(16, 150, "Antenna: Active Helix L1/L5")

        # Card 2: Track Detection
        p.drawRoundedRect(205, 32, 185, 130, 3, 3)
        p.fillRect(205, 32, 185, 16, fg)
        p.setPen(bg)
        p.drawText(211, 44, "TRACK AUTO-DETECTION")
        p.setPen(fg)
        p.setFont(QFont("SansSerif", 9, QFont.Bold))
        p.drawText(214, 70, t.current_track_name[:20])
        p.setFont(QFont("Monospace", 7))
        p.drawText(214, 92, "Distance to S/F: 12 m")
        p.drawText(214, 110, "Status: Ready to Race")
        p.drawText(214, 128, "Auto-Trip: Speed > 15 km/h")
        p.drawText(214, 148, "45.3887N 10.4795E")

        # Card 3: Weather & Maintenance
        p.drawRoundedRect(10, 168, 380, 96, 3, 3)
        p.fillRect(10, 168, 380, 16, fg)
        p.setPen(bg)
        p.drawText(16, 180, "WEATHER & ENGINE MAINTENANCE")
        p.setPen(fg)
        p.setFont(QFont("Monospace", 8))
        p.drawText(20, 204, f"Ambient Temp:   {t.ambient_temp_c:+.1f} °C")
        p.drawText(20, 224, f"Humidity:       {t.ambient_humidity_pct:.1f} % RH")
        p.drawText(20, 244, f"Battery:        {t.battery_voltage:.2f} V ({t.battery_percent}%)")

        eng_hrs = t.engine_total_hours_sec // 3600
        eng_min = (t.engine_total_hours_sec % 3600) // 60
        p.drawText(240, 204, f"Engine Total: {eng_hrs}h {eng_min:02d}m")
        pis_hrs = t.piston_hours_sec // 3600
        pis_min = (t.piston_hours_sec % 3600) // 60
        p.drawText(240, 224, f"Piston Run:   {pis_hrs}h {pis_min:02d}m")
        p.drawText(240, 244, "Apex-Track:   2.4GHz Link OK")

    def _render_data_recall(self, p: QPainter, bg: QColor, fg: QColor):
        p.setFont(QFont("SansSerif", 9, QFont.Bold))
        p.drawText(10, 20, "SESSION DATA RECALL")
        p.drawText(290, 20, "TOTAL LAPS: 12")
        p.drawLine(10, 26, 390, 26)

        # Top 3 Best Laps Table
        p.drawRoundedRect(10, 32, 380, 116, 3, 3)
        p.fillRect(10, 32, 380, 16, fg)
        p.setPen(bg)
        p.drawText(16, 44, "TOP 3 BEST LAPS COMPARISON")
        p.setPen(fg)

        p.setFont(QFont("Monospace", 7, QFont.Bold))
        p.drawText(20, 62, "RANK")
        p.drawText(65, 62, "LAP #")
        p.drawText(115, 62, "LAP TIME")
        p.drawText(190, 62, "S1 / S2 / S3")
        p.drawText(280, 62, "TOP SPD")
        p.drawText(340, 62, "MAX RPM")
        p.drawLine(16, 66, 384, 66)

        table = [
            ("#1", "L02", "48.42s", "16.08 / 16.15 / 16.19", "125.1", "15850"),
            ("#2", "L01", "48.68s", "16.18 / 16.30 / 16.20", "122.4", "15600"),
            ("#3", "L03", "48.75s", "16.22 / 16.29 / 16.24", "121.8", "15500"),
        ]

        for i, row in enumerate(table):
            y = 84 + (i * 20)
            p.drawText(24, y, row[0])
            p.drawText(70, y, row[1])
            p.drawText(115, y, row[2])
            p.drawText(186, y, row[3])
            p.drawText(284, y, row[4])
            p.drawText(340, y, row[5])

        # Summary Box
        p.drawRoundedRect(10, 156, 380, 108, 3, 3)
        p.fillRect(10, 156, 380, 16, fg)
        p.setPen(bg)
        p.drawText(16, 168, "SESSION TELEMETRY STATS & THEORETICAL BEST")
        p.setPen(fg)
        p.setFont(QFont("SansSerif", 9, QFont.Bold))
        p.drawText(20, 194, "Session Best Lap:  L02 (48.42 s)")
        p.setFont(QFont("Monospace", 8))
        p.drawText(20, 216, "Optimal Theoretical Lap:   48.42 s (S1+S2+S3)")
        p.drawText(20, 234, "Peak Cornering G-Force:    1.85 G (Turn 4 Chicane)")
        p.drawText(20, 252, "Session Consistency Index: 98.4 %")

    def _render_menu(self, p: QPainter, bg: QColor, fg: QColor):
        p.fillRect(0, 0, 400, 24, fg)
        p.setPen(bg)
        p.setFont(QFont("SansSerif", 9, QFont.Bold))
        p.drawText(8, 17, I18n.get(StrId.MENU_TITLE))
        p.drawText(310, 17, "[APEX-DASH]")
        p.setPen(fg)

        if self.menu_state == MenuState.MENU_ROOT:
            menu_items = [
                (I18n.get(StrId.CAT_RACE_CONFIG), ICON_GEAR_16X16),
                (I18n.get(StrId.CAT_RPM_ALARM), ICON_LIGHTBULB_16X16),
                (I18n.get(StrId.CAT_TRACK_GPS), ICON_FLAG_16X16),
                (I18n.get(StrId.CAT_STORAGE_PC), ICON_SDCARD_16X16),
                (I18n.get(StrId.CAT_DISPLAY_PWM), ICON_SUN_16X16),
                (I18n.get(StrId.CAT_SYSTEM_LANG), ICON_GLOBE_16X16),
                (I18n.get(StrId.CAT_SENSORS_INFO), ICON_WRENCH_16X16),
                ("< Exit Menu >", ICON_BACK_16X16),
            ]
            for i, (text, icon_xbm) in enumerate(menu_items):
                y = 50 + (i * 28)
                if i == self.cursor_idx:
                    p.fillRect(10, y - 18, 380, 24, fg)
                    p.setPen(bg)
                    draw_xbm(p, 18, y - 14, icon_xbm, 16, 16, color=bg)
                    p.drawText(42, y, text)
                    p.drawText(365, y, ">")
                    p.setPen(fg)
                else:
                    draw_xbm(p, 18, y - 14, icon_xbm, 16, 16, color=fg)
                    p.drawText(42, y, text)


        elif self.menu_state == MenuState.MENU_RACE_SETUP:
            p.drawText(12, 46, I18n.get(StrId.CAT_RACE_CONFIG))
            drive_str = I18n.get(StrId.DRIVE_SHIFTER) if self.settings.drive_type == DriveType.SHIFTER_6SPEED else (
                I18n.get(StrId.DRIVE_CLUTCH) if self.settings.drive_type == DriveType.CLUTCH else I18n.get(StrId.DRIVE_DIRECT))
            
            b1 = f"{I18n.get(StrId.MAX_RPM)}: [- {self.settings.max_rpm} RPM +]" if (self.edit_mode and self.cursor_idx == 1) else f"{I18n.get(StrId.MAX_RPM)}: {self.settings.max_rpm} RPM"
            b2 = f"{I18n.get(StrId.SHIFT_RPM)}: [- {self.settings.shift_rpm} RPM +]" if (self.edit_mode and self.cursor_idx == 2) else f"{I18n.get(StrId.SHIFT_RPM)}: {self.settings.shift_rpm} RPM"
            b3 = f"Over-Rev Alarm: [- {self.settings.over_rev_rpm} RPM +]" if (self.edit_mode and self.cursor_idx == 3) else f"Over-Rev Alarm: {self.settings.over_rev_rpm} RPM"
            
            items = [
                f"{I18n.get(StrId.DRIVE_TYPE)}: {drive_str}",
                b1,
                b2,
                b3,
                "< Return >"
            ]
            for i, text in enumerate(items):
                y = 74 + (i * 32)
                if i == self.cursor_idx:
                    p.fillRect(12, y - 20, 376, 26, fg)
                    p.setPen(bg)
                    p.drawText(24, y, text)
                    p.setPen(fg)
                else:
                    p.drawText(24, y, text)

        elif self.menu_state == MenuState.MENU_LEDS_ALARMS:
            p.drawText(12, 44, I18n.get(StrId.CAT_RPM_ALARM))
            rpm_mode_str = I18n.get(StrId.RPM_DISP_BOTH) if self.settings.rpm_display_mode == RpmDisplayMode.BOTH else (
                I18n.get(StrId.RPM_DISP_DISPLAY) if self.settings.rpm_display_mode == RpmDisplayMode.DISPLAY_ONLY else I18n.get(StrId.RPM_DISP_LEDS))
            
            b0 = f"{I18n.get(StrId.LED_BRIGHTNESS)}: [- {self.settings.led_brightness}% +]" if (self.edit_mode and self.cursor_idx == 0) else f"{I18n.get(StrId.LED_BRIGHTNESS)}: [{self.settings.led_brightness}%]"
            b5 = f"{I18n.get(StrId.WATER_ALARM)}: [- {self.settings.water_temp_alarm_c:.0f}°C +]" if (self.edit_mode and self.cursor_idx == 5) else f"{I18n.get(StrId.WATER_ALARM)}: [{self.settings.water_temp_alarm_c:.0f}°C]"
            b6 = f"Over-Rev Alarm: [- {self.settings.over_rev_rpm} RPM +]" if (self.edit_mode and self.cursor_idx == 6) else f"Over-Rev Alarm: [{self.settings.over_rev_rpm} RPM]"

            items = [
                b0,
                f"{I18n.get(StrId.RPM_DISP_MODE)}: [{rpm_mode_str}]",
                f"{I18n.get(StrId.LED_TEST)} [Click to run]",
                f"Shift LEDs: [{'ON' if self.settings.led_shift_enable else 'OFF'}]",
                f"Alarm LEDs: [{'ON' if self.settings.led_alarm_enable else 'OFF'}]",
                b5,
                b6,
                "WARN Alert Triggers >",
                "< Return >"
            ]
            for i, text in enumerate(items):
                y = 64 + (i * 23)
                if i == self.cursor_idx:
                    p.fillRect(12, y - 16, 376, 20, fg)
                    p.setPen(bg)
                    p.drawText(24, y, text)
                    p.setPen(fg)
                else:
                    p.drawText(24, y, text)

        elif self.menu_state == MenuState.MENU_WARN_TRIGGERS:
            p.drawText(12, 44, "WARN BLINKING TRIGGERS")
            items = [
                f"WARN on Water Temp:   [{'ENABLED' if self.settings.warn_trigger_water else 'OFF'}]",
                f"WARN on Exhaust (EGT): [{'ENABLED' if self.settings.warn_trigger_egt else 'OFF'}]",
                f"WARN on Over-Rev:      [{'ENABLED' if self.settings.warn_trigger_rev else 'OFF'}]",
                f"WARN on Low Battery:   [{'ENABLED' if self.settings.warn_trigger_battery else 'OFF'}]",
                f"WARN on Link Lost:     [{'ENABLED' if self.settings.warn_trigger_link else 'OFF'}]",
                "[ Enable All Triggers ]",
                "[ Disable All Triggers ]",
                "< Return to Alarms Menu >"
            ]
            for i, text in enumerate(items):
                y = 66 + (i * 26)
                if i == self.cursor_idx:
                    p.fillRect(12, y - 17, 376, 22, fg)
                    p.setPen(bg)
                    p.drawText(24, y, text)
                    p.setPen(fg)
                else:
                    p.drawText(24, y, text)


        elif self.menu_state == MenuState.MENU_DISPLAY_PWM:
            p.drawText(12, 46, I18n.get(StrId.CAT_DISPLAY_PWM))
            b0 = f"{I18n.get(StrId.BACKLIGHT_PWM)}: [- {self.settings.backlight_percent}% +]" if (self.edit_mode and self.cursor_idx == 0) else f"{I18n.get(StrId.BACKLIGHT_PWM)} (GPIO 2): [{self.settings.backlight_percent}%]"
            items = [
                b0,
                f"{I18n.get(StrId.SHOW_SPEED)}: [{'ENABLED' if self.settings.show_speed else 'OFF'}]",
                f"{I18n.get(StrId.INVERT_DISP)}: [{'Black/Silver' if self.settings.inverted_display else 'Silver/Black'}]",
                f"{I18n.get(StrId.UNITS_SPEED)}: [{'KM/H' if self.settings.use_kmh else 'MPH'}]",
                f"{I18n.get(StrId.UNITS_TEMP)}: [{'°C' if self.settings.use_celsius else '°F'}]",
                "< Return >"
            ]
            for i, text in enumerate(items):
                y = 70 + (i * 28)
                if i == self.cursor_idx:
                    p.fillRect(12, y - 18, 376, 24, fg)
                    p.setPen(bg)
                    p.drawText(24, y, text)
                    p.setPen(fg)
                else:
                    p.drawText(24, y, text)

        elif self.menu_state == MenuState.MENU_SYSTEM_LANG:
            p.drawText(12, 46, I18n.get(StrId.CAT_SYSTEM_LANG))
            items = [
                f"{I18n.get(StrId.LANGUAGE)}: [{I18n.get_language_name(self.settings.language)}]",
                I18n.get(StrId.RESET_CONFIG),
                "< Return >"
            ]
            for i, text in enumerate(items):
                y = 90 + (i * 45)
                if i == self.cursor_idx:
                    p.fillRect(12, y - 22, 376, 36, fg)
                    p.setPen(bg)
                    p.drawText(24, y, text)
                    p.setPen(fg)
                else:
                    p.drawText(24, y, text)

        elif self.menu_state == MenuState.MENU_USB_MSC_SCREEN:
            p.drawRoundedRect(20, 45, 360, 200, 6, 6)
            p.setFont(QFont("SansSerif", 11, QFont.Bold))
            p.drawText(60, 80, I18n.get(StrId.STATUS_USB_MSC_ACTIVE))
            p.setFont(QFont("Monospace", 8))
            p.drawText(40, 115, "1. Connect USB cable to PC")
            p.drawText(40, 140, "2. Access /tracks/ and /logs/")
            p.drawText(40, 165, "3. Eject drive when finished")
            p.fillRect(50, 195, 300, 32, fg)
            p.setPen(bg)
            p.drawText(80, 217, "PRESS BOOT/KEY TO EXIT")
            p.setPen(fg)

        # Menu Footer Navigation
        if self.menu_state != MenuState.MENU_USB_MSC_SCREEN:
            p.drawLine(0, 276, 400, 276)
            p.setFont(QFont("Monospace", 7))
            if self.edit_mode:
                p.drawText(8, 292, "KEY: + / Inc (+500) | BOOT: - / Dec (-500) | Long: Save")
            else:
                p.drawText(8, 292, "KEY: Next | BOOT: Prev | Long KEY: Edit/Select | Long BOOT: Back")


    def _render_footer(self, p: QPainter, bg: QColor, fg: QColor):
        if self.current_view == UiViewMode.VIEW_LIVE_RACE:
            return  # Live Race HUD renders Track & Status on the bottom line

        p.drawLine(0, 276, 400, 276)
        p.setFont(QFont("Monospace", 7))
        views = ["RACE HUD", "TELEMETRY", "PADDOCK", "DATA RECALL"]
        p.drawText(6, 292, f"KEY: [{views[self.current_view]} {self.current_view+1}/4] | BOOT (Long): Menu | Enter: Invert")

