#!/usr/bin/env python3
"""
Apex-Twin ESP-NOW Latency Benchmarking & Analysis Tool
Captures, logs, and analyzes microsecond-precision round-trip times (RTT),
over-the-air (OTA) latencies, and jitter distributions between two ESP32 modules.

Usage:
    # Read live latency stream from Initiator ESP32 and save to CSV:
    python3 latency_bench.py --port /dev/ttyACM0 --output latency_run1.csv

    # Run automated test sweep and plot results:
    python3 latency_bench.py --port /dev/ttyACM0 --auto-sweep --plot
"""

import argparse
import csv
import sys
import time
from pathlib import Path
from typing import Optional

try:
    import serial
except ImportError:
    print("Error: pyserial is required. Install via: pip install pyserial")
    sys.exit(1)


def parse_args():
    parser = argparse.ArgumentParser(description="Apex-Twin ESP-NOW Latency Benchmarking Tool")
    parser.add_argument("--port", "-p", required=True, help="Serial port of the Initiator ESP32 (e.g. /dev/ttyACM0)")
    parser.add_argument("--baud", "-b", type=int, default=115200, help="Serial baud rate (default: 115200)")
    parser.add_argument("--duration", "-d", type=float, default=30.0, help="Test capture duration in seconds (default: 30s)")
    parser.add_argument("--output", "-o", type=str, default="espnow_latency.csv", help="Output CSV filename")
    parser.add_argument("--auto-sweep", "-a", action="store_true", help="Send 'a' command to trigger automated benchmark matrix")
    parser.add_argument("--plot", action="store_true", help="Generate latency and jitter plots after capture")
    parser.add_argument("--channel", "-c", type=int, default=None, help="Set WiFi Channel (1-13)")
    parser.add_argument("--rate", "-r", type=int, default=None, help="Set Ping Frequency in Hz (e.g. 25, 50, 100)")
    parser.add_argument("--payload", type=int, default=None, help="Set Payload Size in Bytes (16-240)")
    return parser.parse_args()


def plot_results(csv_path: str):
    try:
        import matplotlib.pyplot as plt
        import numpy as np
    except ImportError:
        print("[WARN] matplotlib and numpy are required for plotting. Install via: pip install matplotlib numpy")
        return

    records = []
    with open(csv_path, "r") as f:
        reader = csv.DictReader(f)
        for row in reader:
            try:
                records.append({
                    "seq": int(row["seq"]),
                    "len": int(row["len"]),
                    "rtt_ms": float(row["rtt_us"]) / 1000.0,
                    "ota_ms": float(row["ota_rtt_us"]) / 1000.0,
                    "oneway_ms": float(row["one_way_us"]) / 1000.0,
                    "proc_us": float(row["proc_us"]),
                })
            except (ValueError, KeyError):
                continue

    if not records:
        print("[WARN] No valid data records found to plot.")
        return

    seqs = [r["seq"] for r in records]
    rtts = [r["rtt_ms"] for r in records]
    otas = [r["ota_ms"] for r in records]

    fig, axes = plt.subplots(2, 2, figsize=(14, 9))
    fig.suptitle(f"Apex-Twin ESP-NOW Latency Benchmark ({len(records)} Samples)", fontsize=14, fontweight="bold")

    # 1. Latency Timeline
    axes[0, 0].plot(seqs, rtts, label="Full RTT (ms)", color="#007acc", alpha=0.8, linewidth=1)
    axes[0, 0].plot(seqs, otas, label="OTA RTT (ms)", color="#28a745", alpha=0.7, linestyle="--", linewidth=1)
    axes[0, 0].set_title("Round-Trip Latency over Time")
    axes[0, 0].set_xlabel("Packet Sequence #")
    axes[0, 0].set_ylabel("Latency (ms)")
    axes[0, 0].grid(True, alpha=0.3)
    axes[0, 0].legend()

    # 2. Histogram / Distribution
    axes[0, 1].hist(rtts, bins=40, color="#6f42c1", edgecolor="black", alpha=0.8)
    axes[0, 1].axvline(np.mean(rtts), color="red", linestyle="dashed", linewidth=1.5, label=f"Mean: {np.mean(rtts):.2f} ms")
    axes[0, 1].axvline(np.median(rtts), color="gold", linestyle="dotted", linewidth=1.5, label=f"Median: {np.median(rtts):.2f} ms")
    axes[0, 1].set_title("RTT Distribution & Jitter")
    axes[0, 1].set_xlabel("Latency (ms)")
    axes[0, 1].set_ylabel("Packet Count")
    axes[0, 1].grid(True, alpha=0.3)
    axes[0, 1].legend()

    # 3. Cumulative Distribution Function (CDF)
    sorted_rtt = np.sort(rtts)
    cdf = np.arange(len(sorted_rtt)) / float(len(sorted_rtt))
    axes[1, 0].plot(sorted_rtt, cdf * 100.0, color="#e83e8c", linewidth=2)
    axes[1, 0].axhline(95, color="gray", linestyle=":", label="95th Percentile")
    axes[1, 0].axhline(99, color="black", linestyle=":", label="99th Percentile")
    p95 = np.percentile(rtts, 95)
    p99 = np.percentile(rtts, 99)
    axes[1, 0].set_title(f"Cumulative Latency CDF (P95: {p95:.2f}ms, P99: {p99:.2f}ms)")
    axes[1, 0].set_xlabel("Latency (ms)")
    axes[1, 0].set_ylabel("Percentile (%)")
    axes[1, 0].grid(True, alpha=0.3)
    axes[1, 0].legend()

    # 4. Summary Box
    axes[1, 1].axis("off")
    stats_text = (
        f"BENCHMARK SUMMARY STATISTICS\n"
        f"-----------------------------------------\n"
        f"Total Samples:     {len(records)}\n"
        f"Min RTT:           {np.min(rtts):.3f} ms\n"
        f"Mean RTT:          {np.mean(rtts):.3f} ms\n"
        f"Median RTT:        {np.median(rtts):.3f} ms\n"
        f"Max RTT:           {np.max(rtts):.3f} ms\n"
        f"Standard Dev (σ):  {np.std(rtts):.3f} ms (Jitter)\n"
        f"95th Percentile:   {p95:.3f} ms\n"
        f"99th Percentile:   {p99:.3f} ms\n"
        f"Avg Slave Proc:    {np.mean([r['proc_us'] for r in records]):.1f} µs\n"
        f"Est. 1-Way Latency:{np.mean(otas)/2.0:.3f} ms\n"
    )
    axes[1, 1].text(0.1, 0.5, stats_text, fontfamily="monospace", fontsize=11, verticalalignment="center",
                    bbox=dict(boxstyle="round,pad=1", facecolor="#f8f9fa", edgecolor="#ced4da"))

    plt.tight_layout()
    plot_filename = Path(csv_path).with_suffix(".png")
    plt.savefig(plot_filename, dpi=150)
    print(f"[PLOT] Benchmark plot saved to: {plot_filename}")
    plt.show()


def main():
    args = parse_args()
    print(f"[BENCH] Connecting to ESP32 on {args.port} at {args.baud} baud...")

    try:
        ser = serial.Serial(args.port, args.baud, timeout=0.5)
    except Exception as e:
        print(f"[ERROR] Could not open serial port {args.port}: {e}")
        sys.exit(1)

    time.sleep(1.0)
    ser.reset_input_buffer()

    # Set as Initiator
    ser.write(b"m\n")
    time.sleep(0.2)

    # Optional configuration commands
    if args.channel:
        ser.write(f"c{args.channel}\n".encode())
        time.sleep(0.1)
    if args.rate:
        ser.write(f"f{args.rate}\n".encode())
        time.sleep(0.1)
    if args.payload:
        ser.write(f"p{args.payload}\n".encode())
        time.sleep(0.1)

    if args.auto_sweep:
        print("[BENCH] Triggering automated parameter sweep on device...")
        ser.write(b"a\n")
        # Echo device output until complete
        start_t = time.time()
        while True:
            line = ser.readline().decode("utf-8", errors="replace")
            if line:
                sys.stdout.write(line)
                sys.stdout.flush()
            if "AUTOMATED BENCHMARK RESULTS REPORT" in line:
                # Read remaining report table lines
                for _ in range(30):
                    extra = ser.readline().decode("utf-8", errors="replace")
                    sys.stdout.write(extra)
                break
            if time.time() - start_t > 300: # 5 min timeout
                break
        ser.close()
        return

    # Enable CSV streaming
    print("[BENCH] Enabling CSV streaming mode on device...")
    ser.write(b"v\n")
    time.sleep(0.2)

    # Open CSV for writing
    with open(args.output, "w", newline="") as f:
        writer = csv.writer(f)
        writer.writerow(["seq", "len", "rtt_us", "ota_rtt_us", "one_way_us", "proc_us", "tx_ok"])

        print(f"[BENCH] Capturing latency samples for {args.duration}s into {args.output}...")
        start_time = time.time()
        samples_collected = 0

        while (time.time() - start_time) < args.duration:
            line = ser.readline().decode("utf-8", errors="replace").strip()
            if not line:
                continue

            parts = line.split(",")
            if len(parts) == 7 and parts[0].isdigit():
                writer.writerow(parts)
                samples_collected += 1
                if samples_collected % 50 == 0:
                    rtt_ms = float(parts[2]) / 1000.0
                    print(f"  Sample #{parts[0]}: RTT = {rtt_ms:.2f} ms | 1-Way = {float(parts[4])/1000.0:.2f} ms")

    # Turn off CSV mode
    ser.write(b"v\n")
    ser.close()
    print(f"\n[SUCCESS] Captured {samples_collected} samples into {args.output}")

    if args.plot:
        plot_results(args.output)


if __name__ == "__main__":
    main()
