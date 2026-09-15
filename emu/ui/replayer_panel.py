"""
Session Log Replayer Control Panel for Apex-Dash Emulator
Compact horizontal layout for bottom dock.
"""

from pathlib import Path
from PySide6.QtCore import Qt, Signal
from PySide6.QtWidgets import (
    QWidget,
    QVBoxLayout,
    QHBoxLayout,
    QPushButton,
    QSlider,
    QLabel,
    QFileDialog,
    QComboBox,
    QCheckBox,
)

from emu.core.log_player import LogPlayer


class ReplayerPanel(QWidget):
    """
    Compact control widget for replaying recorded telemetry sessions.
    """
    mode_toggled = Signal(bool)

    def __init__(self, log_player: LogPlayer, parent=None):
        super().__init__(parent)
        self.player = log_player
        self.is_active = False

        self._setup_ui()

    def _setup_ui(self):
        main_layout = QVBoxLayout(self)
        main_layout.setContentsMargins(10, 8, 10, 8)
        main_layout.setSpacing(6)

        # Row 1: Enable checkbox, Open file button, File label, Loop
        row1 = QHBoxLayout()
        row1.setSpacing(10)

        self.chk_enable = QCheckBox("Enable Replayer")
        self.chk_enable.setStyleSheet("""
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
        self.chk_enable.toggled.connect(self._on_enable_toggled)
        row1.addWidget(self.chk_enable)

        self.btn_open = QPushButton("📂 Open CSV / GPX...")
        self.btn_open.setStyleSheet("""
            QPushButton {
                background: #21262d;
                color: #c9d1d9;
                border: 1px solid #30363d;
                border-radius: 4px;
                padding: 4px 10px;
                font-weight: bold;
                font-size: 10px;
            }
            QPushButton:hover {
                background: #30363d;
                color: #58a6ff;
            }
        """)
        self.btn_open.clicked.connect(self._open_file_dialog)
        row1.addWidget(self.btn_open)

        self.lbl_file_info = QLabel("No file loaded")
        self.lbl_file_info.setStyleSheet("color: #8b949e; font-size: 10px;")
        row1.addWidget(self.lbl_file_info, stretch=1)

        self.chk_loop = QCheckBox("Loop")
        self.chk_loop.setChecked(True)
        self.chk_loop.setStyleSheet("color: #8b949e; font-size: 10px;")
        row1.addWidget(self.chk_loop)

        main_layout.addLayout(row1)

        # Row 2: Playback controls, Scrubber timeline, Speed
        row2 = QHBoxLayout()
        row2.setSpacing(8)

        self.btn_rewind = QPushButton("⏮")
        self.btn_rewind.setFixedWidth(36)
        self.btn_rewind.clicked.connect(self._on_rewind)
        self._style_ctrl_btn(self.btn_rewind)
        row2.addWidget(self.btn_rewind)

        self.btn_play = QPushButton("▶ Play")
        self.btn_play.setFixedWidth(65)
        self.btn_play.clicked.connect(self._toggle_play)
        self._style_ctrl_btn(self.btn_play)
        row2.addWidget(self.btn_play)

        self.lbl_time = QLabel("0:00")
        self.lbl_time.setStyleSheet("color: #e6edf3; font-family: monospace; font-size: 11px;")
        row2.addWidget(self.lbl_time)

        self.slider = QSlider(Qt.Horizontal)
        self.slider.setRange(0, 1000)
        self.slider.setValue(0)
        self.slider.sliderMoved.connect(self._on_slider_moved)
        self.slider.setStyleSheet("""
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
        row2.addWidget(self.slider, stretch=1)

        self.lbl_total = QLabel("0:00")
        self.lbl_total.setStyleSheet("color: #8b949e; font-family: monospace; font-size: 11px;")
        row2.addWidget(self.lbl_total)

        lbl_spd = QLabel("Speed:")
        lbl_spd.setStyleSheet("color: #8b949e; font-size: 10px;")
        row2.addWidget(lbl_spd)

        self.combo_speed = QComboBox()
        self.combo_speed.addItems(["0.5x", "1.0x", "2.0x", "5.0x", "10.0x"])
        self.combo_speed.setCurrentIndex(1)
        self.combo_speed.currentIndexChanged.connect(self._on_speed_changed)
        self.combo_speed.setStyleSheet("""
            QComboBox {
                background: #21262d;
                color: #c9d1d9;
                border: 1px solid #30363d;
                border-radius: 4px;
                padding: 2px 6px;
                font-size: 10px;
            }
        """)
        row2.addWidget(self.combo_speed)

        main_layout.addLayout(row2)

        self._update_ui_state(False)

    def _style_ctrl_btn(self, btn: QPushButton):
        btn.setStyleSheet("""
            QPushButton {
                background: #21262d;
                color: #c9d1d9;
                border: 1px solid #30363d;
                border-radius: 4px;
                padding: 4px;
                font-size: 10px;
                font-weight: bold;
            }
            QPushButton:hover {
                background: #30363d;
                color: #58a6ff;
            }
            QPushButton:pressed {
                background: #0d1117;
            }
        """)

    def _on_enable_toggled(self, checked: bool):
        self.is_active = checked
        self._update_ui_state(checked)
        self.mode_toggled.emit(checked)

    def _update_ui_state(self, enabled: bool):
        self.btn_open.setEnabled(enabled)
        has_frames = self.player.get_total_frames() > 0
        self.slider.setEnabled(enabled and has_frames)
        self.btn_play.setEnabled(enabled and has_frames)
        self.btn_rewind.setEnabled(enabled and has_frames)
        self.combo_speed.setEnabled(enabled)
        self.chk_loop.setEnabled(enabled)

    def _open_file_dialog(self):
        file_path, _ = QFileDialog.getOpenFileName(
            self,
            "Open Apex Telemetry Session Log",
            "",
            "Apex Telemetry Log (*.csv *.gpx);;CSV Files (*.csv);;GPX Files (*.gpx);;All Files (*.*)",
        )
        if file_path:
            path = Path(file_path)
            success = self.player.load_csv(path)
            if success:
                total_frames = self.player.get_total_frames()
                duration_s = total_frames / 25.0
                mins = int(duration_s // 60)
                secs = int(duration_s % 60)
                self.lbl_file_info.setText(f"{path.name} ({total_frames} pts, ~{mins}:{secs:02d})")
                self.lbl_total.setText(f"{mins}:{secs:02d}")
                self._update_ui_state(self.is_active)
                self.slider.setValue(0)
            else:
                self.lbl_file_info.setText(f"Failed to load: {path.name}")

    def _on_slider_moved(self, value: int):
        pct = value / 10.0
        self.player.seek_pct(pct)
        self._update_time_label()

    def _toggle_play(self):
        self.player.is_playing = not self.player.is_playing
        self.btn_play.setText("⏸ Pause" if self.player.is_playing else "▶ Play")

    def _on_rewind(self):
        self.player.seek_pct(0.0)
        self.slider.setValue(0)
        self._update_time_label()

    def _on_speed_changed(self, idx: int):
        speeds = [0.5, 1.0, 2.0, 5.0, 10.0]
        self.player.speed_multiplier = speeds[idx]

    def _update_time_label(self):
        curr_frame = self.player.current_idx
        elapsed_s = curr_frame / 25.0
        mins = int(elapsed_s // 60)
        secs = int(elapsed_s % 60)
        self.lbl_time.setText(f"{mins}:{secs:02d}")

    def sync_ui_progress(self):
        if self.player.get_total_frames() > 0:
            pct = self.player.get_progress_pct()
            self.slider.blockSignals(True)
            self.slider.setValue(int(pct * 10.0))
            self.slider.blockSignals(False)
            self._update_time_label()
