# Apex-Dash — 3D CAD Enclosure & Models

This directory contains the parametric 3D CAD enclosure models and assembly files for the **Apex-Dash** steering-wheel display module of the Apex-Twin telemetry system.

---

## 1. Files & Component Overview

All models are authored in **SolidCraft** and stored as native `.sc` container files:

| File | Type | Description |
| :--- | :--- | :--- |
| **`apex-dash-assembly.sc`** | Assembly Container | Full master assembly combining front bezel, rear housing, glass faceplate, external USB-C bulkhead connector, and the Waveshare ESP32-S3-RLCD-4.2 development board reference model. |
| **`apex-dash-front.sc`** | Part Model | Front bezel enclosure with 4.2" screen aperture, integrated 7-segment LED shift light guide bar, peripheral mounting pillars, and outer mating rim. |
| **`apex-dash-back.sc`** | Part Model | Rear housing enclosure with center pod cavity (for battery and electronics), internal PCB mounting rails and bosses, LED retainer bar, and perimeter mounting collars. |
| **`apex-dash-glass.sc`** | Part Model | Front protective screen window / acrylic faceplate for the 4.2" reflective LCD. |
| **`usbc-bulkhead-connector.sc`** | Part Model | Chassis-mount USB Type-C bulkhead pass-through connector body and locking hex nut for external data/firmware flashing access. |

---

## 2. Software Used & File Format

* **CAD Software:** [SolidCraft](https://solidcraft.org) (v0.47+)
* **File Format:** Native SolidCraft container (`.sc`, specification: `solidcraft-container/1`).
  * `.sc` files are zip-compressed archives containing parametric feature trees (`document.json`), container configuration (`container.json`), tessellation cache (`cache/`), and embedded B-rep / STEP geometry blobs (`blobs/`).
* **Viewing & Editing:**
  * Open directly in SolidCraft via **File → Open** or `solidcraft-cli open <file>.sc`.
  * Components can be exported from SolidCraft to standard exchange formats including STEP (`.step` / `.stp`), 3MF (`.3mf`), and STL (`.stl`) for 3D printing or CNC machining.

---

## 3. Third-Party Model Provenance & Licensing

The master assembly (`apex-dash-assembly.sc`) incorporates a reference 3D CAD model of the mainboard:

* **Model Component:** `ESP32-S3-rlcd-4_2_20251208.step` (embedded as feature `ESP32_RLCD`)
* **Hardware:** [Waveshare ESP32-S3-RLCD-4.2](https://docs.waveshare.com/ESP32-S3-RLCD-4.2) (SKU `33298` / `33507`)
* **Source Archive:** Distributed by Waveshare Electronics via [`ESP32-S3-RLCD-4.2-3dFile.rar`](https://files.waveshare.com/wiki/ESP32-S3-RLCD-4.2/ESP32-S3-RLCD-4.2-3dFile.rar) (archived in `apex-dash/refs/datasheets/`).
* **Licensing & Terms of Use:**
  * The ESP32-S3-RLCD-4.2 3D CAD step model is copyright © **Shenzhen Waveshare Electronics Co., Ltd.** and **Espressif Systems**.
  * It is included in this repository strictly as an engineering reference to ensure exact mechanical clearance, PCB mounting hole alignment, and display bezel positioning.
  * All trademarks, trade names, and manufacturer designs remain property of their respective holders.
  * The custom Apex-Dash enclosure parts (`apex-dash-front.sc`, `apex-dash-back.sc`, `apex-dash-glass.sc`, `usbc-bulkhead-connector.sc`, and the enclosure assembly layout) are original designs created for the Apex-Twin project.

---

## 4. Git LFS (Large File Storage)

Due to the embedded binary geometry blobs and parametric container structure (notably `apex-dash-assembly.sc` at ~42 MB), all `*.sc` files in this repository are managed using **Git LFS** (Large File Storage).

Tracking is configured in the root `.gitattributes`:
```gitattributes
*.sc filter=lfs diff=lfs merge=lfs -text
```

When cloning the repository or checking out branches containing `.sc` files, ensure Git LFS is installed and active:
```bash
git lfs install
git lfs pull
```
