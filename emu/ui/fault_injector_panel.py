"""
Manual Fault & Telemetry Parameter Injector Panel for Apex-Dash Emulator
Compact, horizontal layout optimized for bottom-docked placement.
"""

from PySide6.QtCore import Qt, Signal
from PySide6.QtWidgets import (
    QWidget,
    QVBoxLayout,
    QHBoxLayout,
    QGridLayout,
    QLabel,
    QSlider,
    QCheckBox,
    QPushButton,
    QGroupBox,
    QComboBox,
)

from emu.core.telemetry_model import TelemetrySnapshot


class FaultInjectorPanel(QWidget):
    """
    Compact parameter override panel for manual testing & fault injection.
    """
    override_changed = Signal(bool)
    trigger_gate = Signal(str)  # 'sf', 's1', 's2'

    def __init__(self, parent=None):
        super().__init__(parent)
        self.manual_override = False
        self.injected_telemetry = TelemetrySnapshot()

        self._setup_ui()

    def _setup_ui(self):
        main_layout = QVBoxLayout(self)
        main_layout.setContentsMargins(10, 6, 10, 6)
        main_layout.setSpacing(6)

        # 1. Top Control Bar: Master Switch + Quick Fault & Gate Triggers
        top_bar = QHBoxLayout()
        top_bar.setSpacing(10)

        self.chk_override = QCheckBox("Manual Override")
        self.chk_override.setStyleSheet("""
            QCheckBox {
                color: #58a6ff;
                font-weight: bold;
                font-size: 11px;
            }
            QCheckBox::indicator {
                width: 14px;
                height: 14px;
            }
        """)
        self.chk_override.toggled.connect(self._on_override_toggled)
        top_bar.addWidget(self.chk_override)

        # Separator line / spacing
        top_bar.addSpacing(6)

        # Fast Triggers
        btn_gate_sf = QPushButton("🏁 S/F Gate")
        btn_gate_sf.setToolTip("Trigger Start/Finish line (New Lap)")
        btn_gate_sf.clicked.connect(lambda: self.trigger_gate.emit("sf"))
        self._style_quick_btn(btn_gate_sf)
        top_bar.addWidget(btn_gate_sf)

        btn_gate_s1 = QPushButton("⏱ S1")
        btn_gate_s1.clicked.connect(lambda: self.trigger_gate.emit("s1"))
        self._style_quick_btn(btn_gate_s1)
        top_bar.addWidget(btn_gate_s1)

        btn_gate_s2 = QPushButton("⏱ S2")
        btn_gate_s2.clicked.connect(lambda: self.trigger_gate.emit("s2"))
        self._style_quick_btn(btn_gate_s2)
        top_bar.addWidget(btn_gate_s2)

        top_bar.addSpacing(10)

        btn_alarm_water = QPushButton("🔥 Overheat")
        btn_alarm_water.clicked.connect(self._set_overheat_fault)
        self._style_quick_btn(btn_alarm_water)
        top_bar.addWidget(btn_alarm_water)

        btn_alarm_egt = QPushButton("⚡ High EGT")
        btn_alarm_egt.clicked.connect(self._set_egt_fault)
        self._style_quick_btn(btn_alarm_egt)
        top_bar.addWidget(btn_alarm_egt)

        btn_alarm_shift = QPushButton("🚀 Shift RPM")
        btn_alarm_shift.clicked.connect(self._set_shift_rpm)
        self._style_quick_btn(btn_alarm_shift)
        top_bar.addWidget(btn_alarm_shift)

        btn_alarm_lowbatt = QPushButton("🪫 Low Batt")
        btn_alarm_lowbatt.clicked.connect(self._set_low_battery)
        self._style_quick_btn(btn_alarm_lowbatt)
        top_bar.addWidget(btn_alarm_lowbatt)

        top_bar.addStretch()
        main_layout.addLayout(top_bar)

        # 2. Sliders Grid: 2 Side-by-Side Balanced Columns
        grid_layout = QHBoxLayout()
        grid_layout.setSpacing(16)

        # --- Left Column ---
        left_box = QGroupBox("Engine & Dynamics")
        self._style_group_box(left_box)
        grid_left = QGridLayout(left_box)
        grid_left.setContentsMargins(8, 6, 8, 6)
        grid_left.setSpacing(4)

        # RPM Slider
        self.lbl_rpm = QLabel("0 RPM")
        self.sld_rpm = QSlider(Qt.Horizontal)
        self.sld_rpm.setRange(0, 16000)
        self.sld_rpm.setValue(0)
        self.sld_rpm.valueChanged.connect(self._on_slider_changed)
        grid_left.addWidget(QLabel("RPM:"), 0, 0)
        grid_left.addWidget(self.sld_rpm, 0, 1)
        grid_left.addWidget(self.lbl_rpm, 0, 2)

        # Speed Slider
        self.lbl_speed = QLabel("0 km/h")
        self.sld_speed = QSlider(Qt.Horizontal)
        self.sld_speed.setRange(0, 160)
        self.sld_speed.setValue(0)
        self.sld_speed.valueChanged.connect(self._on_slider_changed)
        grid_left.addWidget(QLabel("Speed:"), 1, 0)
        grid_left.addWidget(self.sld_speed, 1, 1)
        grid_left.addWidget(self.lbl_speed, 1, 2)

        # Lateral Accel
        self.lbl_lat_g = QLabel("0.0 G")
        self.sld_lat_g = QSlider(Qt.Horizontal)
        self.sld_lat_g.setRange(-30, 30)
        self.sld_lat_g.setValue(0)
        self.sld_lat_g.valueChanged.connect(self._on_slider_changed)
        grid_left.addWidget(QLabel("Lat G:"), 2, 0)
        grid_left.addWidget(self.sld_lat_g, 2, 1)
        grid_left.addWidget(self.lbl_lat_g, 2, 2)

        # Gear Selector
        self.combo_gear = QComboBox()
        self.combo_gear.addItems(["N (0)", "1", "2", "3", "4", "5", "6"])
        self.combo_gear.currentIndexChanged.connect(self._on_slider_changed)
        grid_left.addWidget(QLabel("Gear:"), 3, 0)
        grid_left.addWidget(self.combo_gear, 3, 1, 1, 2)

        grid_layout.addWidget(left_box)

        # --- Right Column ---
        right_box = QGroupBox("Temperatures & System")
        self._style_group_box(right_box)
        grid_right = QGridLayout(right_box)
        grid_right.setContentsMargins(8, 6, 8, 6)
        grid_right.setSpacing(4)

        # Water Temp
        self.lbl_water = QLabel("65 °C")
        self.sld_water = QSlider(Qt.Horizontal)
        self.sld_water.setRange(20, 120)
        self.sld_water.setValue(65)
        self.sld_water.valueChanged.connect(self._on_slider_changed)
        grid_right.addWidget(QLabel("Water:"), 0, 0)
        grid_right.addWidget(self.sld_water, 0, 1)
        grid_right.addWidget(self.lbl_water, 0, 2)

        # EGT
        self.lbl_egt = QLabel("500 °C")
        self.sld_egt = QSlider(Qt.Horizontal)
        self.sld_egt.setRange(100, 850)
        self.sld_egt.setValue(500)
        self.sld_egt.valueChanged.connect(self._on_slider_changed)
        grid_right.addWidget(QLabel("EGT:"), 1, 0)
        grid_right.addWidget(self.sld_egt, 1, 1)
        grid_right.addWidget(self.lbl_egt, 1, 2)

        # Battery (1S Li-ion / 18650: 3.0V - 4.25V)
        self.lbl_batt = QLabel("4.05 V")
        self.sld_batt = QSlider(Qt.Horizontal)
        self.sld_batt.setRange(300, 425)
        self.sld_batt.setValue(405)
        self.sld_batt.valueChanged.connect(self._on_slider_changed)
        grid_right.addWidget(QLabel("Battery:"), 2, 0)
        grid_right.addWidget(self.sld_batt, 2, 1)
        grid_right.addWidget(self.lbl_batt, 2, 2)

        # GPS Satellites
        self.lbl_sats = QLabel("14 sats")
        self.sld_sats = QSlider(Qt.Horizontal)
        self.sld_sats.setRange(0, 24)
        self.sld_sats.setValue(14)
        self.sld_sats.valueChanged.connect(self._on_slider_changed)
        grid_right.addWidget(QLabel("GPS:"), 3, 0)
        grid_right.addWidget(self.sld_sats, 3, 1)
        grid_right.addWidget(self.lbl_sats, 3, 2)

        grid_layout.addWidget(right_box)
        main_layout.addLayout(grid_layout)

        self._update_slider_state(False)

    def _style_group_box(self, box: QGroupBox):
        box.setStyleSheet("""
            QGroupBox {
                color: #8b949e;
                font-weight: bold;
                font-size: 10px;
                border: 1px solid #30363d;
                border-radius: 6px;
                margin-top: 6px;
                padding-top: 6px;
            }
            QGroupBox::title {
                subcontrol-origin: margin;
                left: 8px;
                padding: 0 4px;
            }
            QLabel {
                font-size: 10px;
                color: #c9d1d9;
            }
            QSlider::groove:horizontal {
                height: 4px;
                background: #21262d;
                border-radius: 2px;
            }
            QSlider::sub-page:horizontal {
                background: #58a6ff;
                border-radius: 2px;
            }
            QSlider::handle:horizontal {
                background: #f0f6fc;
                width: 12px;
                margin-top: -4px;
                margin-bottom: -4px;
                border-radius: 6px;
            }
        """)

    def _style_quick_btn(self, btn: QPushButton):
        btn.setStyleSheet("""
            QPushButton {
                background: #21262d;
                color: #c9d1d9;
                border: 1px solid #30363d;
                border-radius: 4px;
                padding: 3px 8px;
                font-size: 10px;
                font-weight: bold;
            }
            QPushButton:hover {
                background: #30363d;
                color: #58a6ff;
                border-color: #58a6ff;
            }
            QPushButton:pressed {
                background: #0d1117;
            }
        """)

    def _on_override_toggled(self, checked: bool):
        self.manual_override = checked
        self._update_slider_state(checked)
        self.override_changed.emit(checked)

    def _update_slider_state(self, enabled: bool):
        self.sld_rpm.setEnabled(enabled)
        self.sld_speed.setEnabled(enabled)
        self.sld_water.setEnabled(enabled)
        self.sld_egt.setEnabled(enabled)
        self.sld_lat_g.setEnabled(enabled)
        self.sld_batt.setEnabled(enabled)
        self.sld_sats.setEnabled(enabled)
        self.combo_gear.setEnabled(enabled)

    def _on_slider_changed(self):
        rpm = self.sld_rpm.value()
        speed = self.sld_speed.value()
        water = self.sld_water.value()
        egt = self.sld_egt.value()
        lat_g = self.sld_lat_g.value() / 10.0
        batt = self.sld_batt.value() / 100.0
        sats = self.sld_sats.value()
        gear = self.combo_gear.currentIndex()

        if batt >= 4.15:
            batt_pct = 100
        elif batt <= 3.20:
            batt_pct = 0
        else:
            batt_pct = int(((batt - 3.20) / (4.15 - 3.20)) * 100.0)

        self.lbl_rpm.setText(f"{rpm} RPM")
        self.lbl_speed.setText(f"{speed} km/h")
        self.lbl_water.setText(f"{water} °C")
        self.lbl_egt.setText(f"{egt} °C")
        self.lbl_lat_g.setText(f"{lat_g:+.1f} G")
        self.lbl_batt.setText(f"{batt:.2f} V ({batt_pct}%)")
        self.lbl_sats.setText(f"{sats} sats")

        self.injected_telemetry.rpm = rpm
        self.injected_telemetry.speed_kmh = float(speed)
        self.injected_telemetry.water_temp_c = float(water)
        self.injected_telemetry.exhaust_temp_c = float(egt)
        self.injected_telemetry.lateral_g = lat_g
        self.injected_telemetry.battery_voltage = batt
        self.injected_telemetry.battery_percent = batt_pct
        self.injected_telemetry.satellites_visible = sats
        self.injected_telemetry.gear = gear

    def _set_overheat_fault(self):
        self.chk_override.setChecked(True)
        self.sld_water.setValue(95)

    def _set_egt_fault(self):
        self.chk_override.setChecked(True)
        self.sld_egt.setValue(720)

    def _set_shift_rpm(self):
        self.chk_override.setChecked(True)
        self.sld_rpm.setValue(13800)

    def _set_low_battery(self):
        self.chk_override.setChecked(True)
        self.sld_batt.setValue(330)  # 3.30 V (~10% battery)

    def get_injected_snapshot(self) -> TelemetrySnapshot:
        return self.injected_telemetry
