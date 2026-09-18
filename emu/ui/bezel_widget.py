"""
Steering Wheel Enclosure / Bezel Widget for Apex-Dash Emulator
Wraps the 7-LED strip, 400x300 RLCD display, and physical push buttons (BOOT, KEY, AUX L, AUX R).
"""

import time
from PySide6.QtCore import Qt, QTimer, Signal
from PySide6.QtGui import QColor, QPainter, QBrush, QPen, QFont, QLinearGradient
from PySide6.QtWidgets import (
    QWidget,
    QVBoxLayout,
    QHBoxLayout,
    QPushButton,
    QLabel,
    QFrame,
    QToolTip,
)

from emu.ui.led_bar_widget import LedBarWidget
from emu.ui.rlcd_renderer import RlcdRenderer
from emu.core.telemetry_model import SystemSettings, TelemetrySnapshot


class HoldableButton(QPushButton):
    """
    Physical push button supporting both short click and long-press (held > 500ms).
    Right-click immediately fires a long-press.
    """
    short_pressed = Signal()
    long_pressed = Signal()

    def __init__(self, text, subtitle="", parent=None):
        super().__init__(parent)
        self.btn_title = text
        self.btn_subtitle = subtitle
        self.setText(f"{text}\n{subtitle}" if subtitle else text)
        self.setMinimumSize(85, 75)
        self.setCursor(Qt.PointingHandCursor)
        
        self.press_start_time = 0.0
        self.hold_timer = QTimer(self)
        self.hold_timer.setSingleShot(True)
        self.hold_timer.setInterval(500)
        self.hold_timer.timeout.connect(self._on_hold_timeout)
        self.is_long_fired = False

        self.setStyleSheet("""
            QPushButton {
                background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                    stop:0 #2c313a, stop:0.5 #1e2227, stop:1 #14171a);
                color: #e6edf3;
                border: 2px solid #3d4450;
                border-radius: 8px;
                font-family: 'Monospace', 'Courier New', monospace;
                font-weight: bold;
                font-size: 11px;
                padding: 4px;
            }
            QPushButton:hover {
                background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                    stop:0 #3a424e, stop:0.5 #282e35, stop:1 #1d2126);
                border: 2px solid #58a6ff;
            }
            QPushButton:pressed {
                background: #0d1117;
                border: 2px solid #1f6feb;
                color: #58a6ff;
            }
        """)

    def mousePressEvent(self, event):
        if event.button() == Qt.RightButton:
            # Direct Long Press via Right Click
            self.long_pressed.emit()
            event.accept()
            return
        elif event.button() == Qt.LeftButton:
            self.press_start_time = time.time()
            self.is_long_fired = False
            self.hold_timer.start()
        super().mousePressEvent(event)

    def mouseReleaseEvent(self, event):
        if event.button() == Qt.LeftButton:
            self.hold_timer.stop()
            if not self.is_long_fired:
                self.short_pressed.emit()
        super().mouseReleaseEvent(event)

    def _on_hold_timeout(self):
        self.is_long_fired = True
        self.long_pressed.emit()


class BezelWidget(QFrame):
    """
    Simulated CNC Billet Aluminium / Carbon Composite Steering Wheel Enclosure
    """
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setObjectName("BezelFrame")
        self.setFrameShape(QFrame.StyledPanel)

        # Internal components
        self.led_bar = LedBarWidget(self)
        self.screen = RlcdRenderer(self)

        # Buttons
        self.btn_boot = HoldableButton("BOOT", "◄ PREV / MENU")
        self.btn_boot.setToolTip("Click: Prev Page / Up\nHold (>0.5s) or Right-Click: Enter/Exit Menu\n[Hotkey: Space / Up]")
        self.btn_boot.short_pressed.connect(self.screen.handle_boot_short)
        self.btn_boot.long_pressed.connect(self.screen.handle_boot_long)

        self.btn_key = HoldableButton("KEY", "NEXT / SEL ►")
        self.btn_key.setToolTip("Click: Next Page / Down\nHold (>0.5s) or Right-Click: Invert Polarity / Select\n[Hotkey: Enter / Down]")
        self.btn_key.short_pressed.connect(self.screen.handle_key_short)
        self.btn_key.long_pressed.connect(self.screen.handle_key_long)

        self.btn_aux_l = QPushButton("AUX L\n[MARK]")
        self.btn_aux_l.setFixedSize(70, 45)
        self.btn_aux_l.setToolTip("Auxiliary Input 1 (Lap Mark / Gate)\n[Hotkey: Tab]")
        self._style_aux_btn(self.btn_aux_l)

        self.btn_aux_r = QPushButton("AUX R\n[VIEW]")
        self.btn_aux_r.setFixedSize(70, 45)
        self.btn_aux_r.setToolTip("Auxiliary Input 2 (Quick View Switch)\n[Hotkey: V]")
        self._style_aux_btn(self.btn_aux_r)

        self._setup_layout()

    def _style_aux_btn(self, btn: QPushButton):
        btn.setCursor(Qt.PointingHandCursor)
        btn.setStyleSheet("""
            QPushButton {
                background: #1c2128;
                color: #8b949e;
                border: 1px solid #30363d;
                border-radius: 6px;
                font-family: monospace;
                font-size: 9px;
                font-weight: bold;
            }
            QPushButton:hover {
                background: #262c36;
                color: #f0f6fc;
                border: 1px solid #58a6ff;
            }
            QPushButton:pressed {
                background: #0d1117;
                color: #58a6ff;
            }
        """)

    def _setup_layout(self):
        main_layout = QVBoxLayout(self)
        main_layout.setContentsMargins(25, 14, 25, 14)
        main_layout.setSpacing(8)

        # Header Row with Brand & Action Buttons
        header_row = QHBoxLayout()
        header_row.setContentsMargins(0, 0, 0, 0)

        lbl_brand = QLabel("APEX-DASH // TELEMETRY & RACING DISPLAY")
        lbl_brand.setStyleSheet("color: #6e7681; font-weight: bold; font-size: 10px; letter-spacing: 2px;")
        header_row.addWidget(lbl_brand)
        header_row.addStretch()

        self.btn_reload = QPushButton("⚡ Reload [F5]")
        self.btn_reload.setCursor(Qt.PointingHandCursor)
        self.btn_reload.setToolTip("Hot-reload UI & Renderer code without restarting\n[Hotkey: F5]")
        self.btn_reload.setStyleSheet("""
            QPushButton {
                background: #21262d;
                color: #58a6ff;
                border: 1px solid #30363d;
                border-radius: 4px;
                padding: 3px 8px;
                font-size: 10px;
                font-weight: bold;
            }
            QPushButton:hover {
                background: #30363d;
                color: #79c0ff;
                border-color: #58a6ff;
            }
            QPushButton:pressed {
                background: #0d1117;
            }
        """)
        header_row.addWidget(self.btn_reload)

        self.btn_reset = QPushButton("🔄 Reset Sim")
        self.btn_reset.setCursor(Qt.PointingHandCursor)
        self.btn_reset.setToolTip("Reset simulation timer, laps, and physics state\n[Hotkey: Ctrl+R]")
        self.btn_reset.setStyleSheet("""
            QPushButton {
                background: #21262d;
                color: #8b949e;
                border: 1px solid #30363d;
                border-radius: 4px;
                padding: 3px 8px;
                font-size: 10px;
                font-weight: bold;
            }
            QPushButton:hover {
                background: #30363d;
                color: #f0f6fc;
                border-color: #8b949e;
            }
            QPushButton:pressed {
                background: #0d1117;
            }
        """)
        header_row.addWidget(self.btn_reset)

        self.btn_screenshot = QPushButton("📸 Screenshot [F12]")
        self.btn_screenshot.setCursor(Qt.PointingHandCursor)
        self.btn_screenshot.setToolTip("Save high-res screenshot to docs/imgs/\n[Hotkey: F12 / Ctrl+S]")
        self.btn_screenshot.setStyleSheet("""
            QPushButton {
                background: #21262d;
                color: #3fb950;
                border: 1px solid #30363d;
                border-radius: 4px;
                padding: 3px 8px;
                font-size: 10px;
                font-weight: bold;
            }
            QPushButton:hover {
                background: #30363d;
                color: #56d364;
                border-color: #3fb950;
            }
            QPushButton:pressed {
                background: #0d1117;
            }
        """)
        self.btn_screenshot.clicked.connect(self._on_screenshot_clicked)
        header_row.addWidget(self.btn_screenshot)

        main_layout.addLayout(header_row)

        # LED Bar
        main_layout.addWidget(self.led_bar, alignment=Qt.AlignCenter)

        # Screen & Side Buttons Row
        center_row = QHBoxLayout()
        center_row.setSpacing(18)
        center_row.setAlignment(Qt.AlignCenter)

        # Left Column (BOOT Button)
        left_col = QVBoxLayout()
        left_col.setAlignment(Qt.AlignCenter)
        left_col.addWidget(self.btn_boot)
        left_col.addSpacing(10)
        left_col.addWidget(self.btn_aux_l)
        center_row.addLayout(left_col)

        # Center: Screen Housing
        screen_frame = QFrame()
        screen_frame.setMinimumSize(404, 304)
        from PySide6.QtWidgets import QSizePolicy
        screen_frame.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        screen_frame.setStyleSheet("""
            background: #000000;
            border: 4px solid #161b22;
            border-radius: 4px;
        """)
        self.screen_layout = QVBoxLayout(screen_frame)
        self.screen_layout.setContentsMargins(0, 0, 0, 0)
        self.screen_layout.addWidget(self.screen)
        center_row.addWidget(screen_frame, stretch=1)

        # Right Column (KEY Button)
        right_col = QVBoxLayout()
        right_col.setAlignment(Qt.AlignCenter)
        right_col.addWidget(self.btn_key)
        right_col.addSpacing(10)
        right_col.addWidget(self.btn_aux_r)
        center_row.addLayout(right_col)

        main_layout.addLayout(center_row)

        # Bottom Subtitle
        lbl_sub = QLabel("SITRONIX ST7305 • 400x300 REFLECTIVE LCD • 25Hz CAN/BLE ENGINE")
        lbl_sub.setAlignment(Qt.AlignCenter)
        lbl_sub.setStyleSheet("color: #30363d; font-size: 8px; font-weight: bold; letter-spacing: 1px;")
        main_layout.addWidget(lbl_sub)

    def reload_screen(self):
        """Hot-reloads RlcdRenderer, i18n, and telemetry model modules dynamically"""
        import importlib
        import emu.core.telemetry_model
        import emu.core.i18n
        import emu.ui.rlcd_renderer

        importlib.reload(emu.core.telemetry_model)
        importlib.reload(emu.core.i18n)
        importlib.reload(emu.ui.rlcd_renderer)

        from emu.ui.rlcd_renderer import RlcdRenderer

        old_screen = self.screen
        old_settings = old_screen.settings
        old_view = old_screen.current_view
        old_menu_active = old_screen.menu_active
        old_menu_state = old_screen.menu_state
        old_cursor = old_screen.cursor_idx
        old_telemetry = old_screen.telemetry

        new_screen = RlcdRenderer(self)
        new_screen.settings = old_settings
        new_screen.current_view = old_view
        new_screen.menu_active = old_menu_active
        new_screen.menu_state = old_menu_state
        new_screen.cursor_idx = old_cursor
        new_screen.telemetry = old_telemetry

        self.screen_layout.replaceWidget(old_screen, new_screen)
        old_screen.deleteLater()
        self.screen = new_screen

        # Reconnect buttons
        try:
            self.btn_boot.short_pressed.disconnect()
            self.btn_boot.long_pressed.disconnect()
            self.btn_key.short_pressed.disconnect()
            self.btn_key.long_pressed.disconnect()
        except RuntimeError:
            pass

        self.btn_boot.short_pressed.connect(self.screen.handle_boot_short)
        self.btn_boot.long_pressed.connect(self.screen.handle_boot_long)
        self.btn_key.short_pressed.connect(self.screen.handle_key_short)
        self.btn_key.long_pressed.connect(self.screen.handle_key_long)

        self.screen.update()

    def update_hardware(self, telemetry: TelemetrySnapshot, settings: SystemSettings):
        self.led_bar.update_leds(telemetry, settings)
        self.screen.set_data(telemetry, settings)

    def paintEvent(self, event):
        """Draw realistic bevel & carbon bezel frame background"""
        painter = QPainter(self)
        painter.setRenderHint(QPainter.Antialiasing)

        rect = self.rect()
        
        # Outer chassis gradient
        grad = QLinearGradient(0, 0, 0, rect.height())
        grad.setColorAt(0.0, QColor(24, 27, 32))
        grad.setColorAt(0.5, QColor(15, 17, 20))
        grad.setColorAt(1.0, QColor(10, 12, 14))

        painter.setBrush(QBrush(grad))
        painter.setPen(QPen(QColor(50, 56, 66), 2))
        painter.drawRoundedRect(rect.adjusted(1, 1, -1, -1), 16, 16)

        # Subtle metallic accent border
        painter.setBrush(Qt.NoBrush)
        painter.setPen(QPen(QColor(30, 35, 42), 1))
        painter.drawRoundedRect(rect.adjusted(6, 6, -6, -6), 12, 12)

        super().paintEvent(event)

    def _on_screenshot_clicked(self):
        saved_file = self.save_screenshot()
        # Notify via tooltip or status
        QToolTip.showText(self.btn_screenshot.mapToGlobal(self.btn_screenshot.rect().bottomLeft()),
                          f"Saved: {saved_file}", self.btn_screenshot, self.btn_screenshot.rect(), 3000)

    def save_screenshot(self, target_path: str = None, screen_only: bool = False) -> str:
        """
        Capture and save a PNG screenshot of the dashboard.
        If screen_only=True, saves only the RLCD screen (400x300 dot matrix).
        Otherwise saves the full simulated steering wheel bezel with RGB LEDs and buttons.
        """
        from pathlib import Path
        repo_root = Path(__file__).resolve().parent.parent.parent
        out_dir = repo_root / "docs" / "imgs"
        out_dir.mkdir(parents=True, exist_ok=True)

        if not target_path:
            import time
            prefix = "screen" if screen_only else "dash"
            view_name = self.screen.current_view.name.lower().replace("view_", "")
            target_path = str(out_dir / f"{prefix}_{view_name}_{int(time.time())}.png")

        dest = Path(target_path)
        dest.parent.mkdir(parents=True, exist_ok=True)

        if screen_only:
            pixmap = self.screen.render_to_pixmap(800, 600)
        else:
            pixmap = self.grab()
        pixmap.save(str(dest), "PNG")
        return str(dest)
