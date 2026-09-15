import argparse
import csv
import math
import re
from pathlib import Path
from typing import Dict, List, Optional, Tuple

try:
    import folium
except ImportError:
    folium = None

try:
    import matplotlib.pyplot as plt
    from matplotlib.lines import Line2D
except ImportError:
    plt = None
    Line2D = None


GGA_PATTERN = re.compile(r"\$(?:GN|GP)GGA,[^\r\n]*")

QUALITY_LABELS: Dict[int, str] = {
    0: "Invalid",
    1: "GPS",
    2: "DGPS",
    3: "PPS",
    4: "RTK Fixed",
    5: "RTK Float",
    6: "Estimated",
    7: "Manual",
    8: "Simulation",
}

QUALITY_COLORS: Dict[int, str] = {
    0: "#7f8c8d",
    1: "#3498db",
    2: "#16a085",
    3: "#9b59b6",
    4: "#27ae60",
    5: "#f39c12",
    6: "#d35400",
    7: "#8e44ad",
    8: "#2c3e50",
}


def compute_nmea_checksum(payload: str) -> int:
    checksum = 0
    for char in payload:
        checksum ^= ord(char)
    return checksum


def extract_gga_sentence(line: str) -> Optional[str]:
    match = GGA_PATTERN.search(line)
    if not match:
        return None
    return match.group(0).strip()


def verify_checksum(sentence: str) -> bool:
    if "*" not in sentence:
        return True

    if not sentence.startswith("$"):
        return False

    body, checksum_text = sentence[1:].split("*", 1)
    checksum_text = checksum_text.strip()
    if len(checksum_text) < 2:
        return False

    try:
        expected = int(checksum_text[:2], 16)
    except ValueError:
        return False

    return compute_nmea_checksum(body) == expected


def parse_nmea_time_to_seconds(utc_raw: str) -> Optional[float]:
    if not utc_raw:
        return None

    try:
        utc_value = float(utc_raw)
    except ValueError:
        return None

    hours = int(utc_value // 10000)
    minutes = int((utc_value - hours * 10000) // 100)
    seconds = utc_value - hours * 10000 - minutes * 100

    if not (0 <= hours <= 23 and 0 <= minutes <= 59 and 0.0 <= seconds < 60.0):
        return None

    return hours * 3600 + minutes * 60 + seconds


def parse_nmea_coordinate(raw: str, hemisphere: str, is_latitude: bool) -> Optional[float]:
    if not raw or not hemisphere:
        return None

    try:
        value = float(raw)
    except ValueError:
        return None

    if is_latitude:
        degrees = int(value // 100)
        minutes = value - degrees * 100
    else:
        degrees = int(value // 100)
        minutes = value - degrees * 100

    decimal = degrees + minutes / 60.0
    if hemisphere in ("S", "W"):
        decimal *= -1.0

    return decimal


def haversine_meters(lat1: float, lon1: float, lat2: float, lon2: float) -> float:
    radius = 6371000.0
    phi1 = math.radians(lat1)
    phi2 = math.radians(lat2)
    d_phi = math.radians(lat2 - lat1)
    d_lambda = math.radians(lon2 - lon1)

    a = math.sin(d_phi / 2) ** 2 + math.cos(phi1) * math.cos(phi2) * math.sin(d_lambda / 2) ** 2
    c = 2 * math.atan2(math.sqrt(a), math.sqrt(1 - a))
    return radius * c


def parse_gga_logs(input_files: List[Path], strict_checksum: bool) -> List[Dict[str, float]]:
    points: List[Dict[str, float]] = []

    raw_rows: List[Dict[str, float]] = []

    for input_file in input_files:
        with input_file.open("r", encoding="utf-8", errors="replace") as handle:
            for line_number, line in enumerate(handle, start=1):
                sentence = extract_gga_sentence(line)
                if not sentence:
                    continue

                if strict_checksum and not verify_checksum(sentence):
                    continue

                sentence_no_checksum = sentence.split("*", 1)[0]
                fields = sentence_no_checksum.split(",")
                if len(fields) < 10:
                    continue

                utc_raw = fields[1]
                lat_raw = fields[2]
                lat_hemi = fields[3]
                lon_raw = fields[4]
                lon_hemi = fields[5]
                quality_raw = fields[6]

                utc_seconds = parse_nmea_time_to_seconds(utc_raw)
                latitude = parse_nmea_coordinate(lat_raw, lat_hemi, is_latitude=True)
                longitude = parse_nmea_coordinate(lon_raw, lon_hemi, is_latitude=False)

                if utc_seconds is None or latitude is None or longitude is None:
                    continue

                try:
                    quality = int(quality_raw)
                except ValueError:
                    quality = 0

                raw_rows.append(
                    {
                        "file": str(input_file),
                        "line": float(line_number),
                        "utc_seconds": utc_seconds,
                        "latitude": latitude,
                        "longitude": longitude,
                        "quality": float(quality),
                    }
                )

    if not raw_rows:
        return points

    day_offset = 0.0
    previous_utc = raw_rows[0]["utc_seconds"]

    for row in raw_rows:
        utc_seconds = row["utc_seconds"]
        if utc_seconds + day_offset < previous_utc - 12 * 3600:
            day_offset += 24 * 3600

        elapsed_seconds = utc_seconds + day_offset
        previous_utc = elapsed_seconds

        row["elapsed_seconds"] = elapsed_seconds
        points.append(row)

    return points


def add_kinematics(points: List[Dict[str, float]]) -> None:
    if not points:
        return

    points[0]["distance_m"] = 0.0
    points[0]["speed_mps"] = math.nan
    points[0]["accel_mps2"] = math.nan

    cumulative_distance = 0.0

    for idx in range(1, len(points)):
        previous = points[idx - 1]
        current = points[idx]

        # Avoid speed estimates across GPS fix-quality transitions.
        quality_changed = int(current["quality"]) != int(previous["quality"])

        dt = current["elapsed_seconds"] - previous["elapsed_seconds"]
        if dt <= 0 or quality_changed:
            segment_distance = 0.0
            speed = math.nan
        else:
            segment_distance = haversine_meters(
                previous["latitude"],
                previous["longitude"],
                current["latitude"],
                current["longitude"],
            )
            speed = segment_distance / dt

        cumulative_distance += segment_distance
        current["distance_m"] = cumulative_distance
        current["speed_mps"] = speed

    for idx in range(1, len(points)):
        previous = points[idx - 1]
        current = points[idx]
        quality_changed = int(current["quality"]) != int(previous["quality"])
        dt = current["elapsed_seconds"] - previous["elapsed_seconds"]

        if dt <= 0 or quality_changed or math.isnan(previous["speed_mps"]) or math.isnan(current["speed_mps"]):
            current["accel_mps2"] = math.nan
        else:
            current["accel_mps2"] = (current["speed_mps"] - previous["speed_mps"]) / dt


def quality_label(quality_code: int) -> str:
    return QUALITY_LABELS.get(quality_code, f"Unknown ({quality_code})")


def quality_color(quality_code: int) -> str:
    return QUALITY_COLORS.get(quality_code, "#000000")


def write_csv(points: List[Dict[str, float]], output_path: Path) -> None:
    with output_path.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.writer(handle)
        writer.writerow(
            [
                "source_file",
                "source_line",
                "elapsed_seconds",
                "utc_seconds",
                "latitude",
                "longitude",
                "quality_code",
                "quality_label",
                "distance_m",
                "speed_mps",
                "speed_kmh",
                "accel_mps2",
            ]
        )

        for point in points:
            speed_mps = point["speed_mps"]
            speed_kmh = speed_mps * 3.6 if not math.isnan(speed_mps) else math.nan
            writer.writerow(
                [
                    point["file"],
                    int(point["line"]),
                    point["elapsed_seconds"],
                    point["utc_seconds"],
                    point["latitude"],
                    point["longitude"],
                    int(point["quality"]),
                    quality_label(int(point["quality"])),
                    point.get("distance_m", math.nan),
                    speed_mps,
                    speed_kmh,
                    point.get("accel_mps2", math.nan),
                ]
            )


def create_map(points: List[Dict[str, float]], output_path: Path) -> None:
    if folium is None:
        raise ImportError("folium is not installed")

    center_lat = sum(p["latitude"] for p in points) / len(points)
    center_lon = sum(p["longitude"] for p in points) / len(points)

    route_map = folium.Map(location=[center_lat, center_lon], zoom_start=16, tiles="OpenStreetMap")

    polyline = [(p["latitude"], p["longitude"]) for p in points]
    folium.PolyLine(polyline, color="#34495e", weight=2, opacity=0.8).add_to(route_map)

    marker_step = max(1, len(points) // 5000)
    for idx, point in enumerate(points):
        if idx % marker_step != 0 and idx != len(points) - 1:
            continue

        quality = int(point["quality"])
        speed_mps = point["speed_mps"]
        speed_text = f"{speed_mps * 3.6:.2f} km/h" if not math.isnan(speed_mps) else "n/a"

        popup_text = (
            f"t={point['elapsed_seconds']:.2f}s<br>"
            f"quality={quality_label(quality)} ({quality})<br>"
            f"lat={point['latitude']:.7f}<br>"
            f"lon={point['longitude']:.7f}<br>"
            f"speed={speed_text}"
        )

        folium.CircleMarker(
            location=(point["latitude"], point["longitude"]),
            radius=4,
            weight=1,
            color=quality_color(quality),
            fill=True,
            fill_color=quality_color(quality),
            fill_opacity=0.9,
            popup=folium.Popup(popup_text, max_width=280),
        ).add_to(route_map)

    route_map.save(str(output_path))


def create_plots(points: List[Dict[str, float]], output_path: Path) -> None:
    if plt is None or Line2D is None:
        raise ImportError("matplotlib is not installed")

    excluded_qualities = {1, 2}

    base_time = points[0]["elapsed_seconds"]
    times = [p["elapsed_seconds"] - base_time for p in points]
    speeds_kmh = [
        p["speed_mps"] * 3.6
        if int(p["quality"]) not in excluded_qualities and not math.isnan(p["speed_mps"])
        else math.nan
        for p in points
    ]
    accels = [
        p["accel_mps2"] if int(p["quality"]) not in excluded_qualities else math.nan
        for p in points
    ]
    colors = [quality_color(int(p["quality"])) for p in points]

    fig, (ax_speed, ax_accel) = plt.subplots(2, 1, figsize=(12, 8), sharex=True)

    ax_speed.plot(times, speeds_kmh, color="#95a5a6", linewidth=1.0, alpha=0.7)
    ax_speed.scatter(times, speeds_kmh, c=colors, s=12)
    ax_speed.set_ylabel("Speed (km/h)")
    ax_speed.grid(True, alpha=0.3)

    ax_accel.plot(times, accels, color="#95a5a6", linewidth=1.0, alpha=0.7)
    ax_accel.scatter(times, accels, c=colors, s=12)
    ax_accel.set_ylabel("Acceleration (m/s^2)")
    ax_accel.set_xlabel("Elapsed time (s)")
    ax_accel.grid(True, alpha=0.3)

    present_qualities = sorted({int(p["quality"]) for p in points})
    legend_items = [
        Line2D(
            [0],
            [0],
            marker="o",
            color="w",
            markerfacecolor=quality_color(q),
            markeredgecolor=quality_color(q),
            markersize=7,
            label=quality_label(q),
        )
        for q in present_qualities
    ]
    ax_speed.legend(handles=legend_items, loc="best", title="Fix Quality")

    fig.tight_layout()
    fig.savefig(output_path, dpi=150)
    plt.close(fig)


def summarize(points: List[Dict[str, float]]) -> str:
    if not points:
        return "No valid GGA points found."

    elapsed = points[-1]["elapsed_seconds"] - points[0]["elapsed_seconds"]
    total_distance = points[-1].get("distance_m", 0.0)

    valid_speeds = [p["speed_mps"] for p in points if not math.isnan(p["speed_mps"])]
    max_speed_kmh = max(valid_speeds) * 3.6 if valid_speeds else 0.0
    avg_speed_kmh = (sum(valid_speeds) / len(valid_speeds)) * 3.6 if valid_speeds else 0.0

    quality_counts: Dict[int, int] = {}
    for point in points:
        q = int(point["quality"])
        quality_counts[q] = quality_counts.get(q, 0) + 1

    quality_breakdown = ", ".join(
        f"{quality_label(q)}={count}" for q, count in sorted(quality_counts.items())
    )

    return (
        f"Points: {len(points)}\n"
        f"Duration: {elapsed:.1f} s\n"
        f"Distance: {total_distance:.1f} m\n"
        f"Avg speed: {avg_speed_kmh:.2f} km/h\n"
        f"Max speed: {max_speed_kmh:.2f} km/h\n"
        f"Quality counts: {quality_breakdown}"
    )


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Analyze GGA logs and generate map + speed/acceleration plots"
    )
    parser.add_argument(
        "inputs",
        nargs="+",
        help="One or more log files containing $GNGGA/$GPGGA sentences",
    )
    parser.add_argument(
        "-o",
        "--output-dir",
        default="analysis_output",
        help="Directory for generated files",
    )
    parser.add_argument(
        "--skip-checksum",
        action="store_true",
        help="Skip NMEA checksum validation",
    )
    parser.add_argument(
        "--map-name",
        default="route_map.html",
        help="Output HTML map filename",
    )
    parser.add_argument(
        "--plot-name",
        default="speed_acceleration.png",
        help="Output speed/acceleration plot filename",
    )
    parser.add_argument(
        "--csv-name",
        default="derived_metrics.csv",
        help="Output CSV filename with derived metrics",
    )
    return parser


def main() -> int:
    parser = build_parser()
    args = parser.parse_args()

    input_files = [Path(path) for path in args.inputs]
    missing = [path for path in input_files if not path.exists()]
    if missing:
        for path in missing:
            print(f"Missing input file: {path}")
        return 1

    output_dir = Path(args.output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)

    points = parse_gga_logs(input_files, strict_checksum=not args.skip_checksum)
    if not points:
        print("No valid GGA points were parsed from the input logs.")
        return 2

    add_kinematics(points)

    map_path = output_dir / args.map_name
    plot_path = output_dir / args.plot_name
    csv_path = output_dir / args.csv_name

    write_csv(points, csv_path)

    try:
        create_map(points, map_path)
    except ImportError as exc:
        print(f"Map generation skipped: {exc}")
        print("Install dependency with: pip install folium")

    try:
        create_plots(points, plot_path)
    except ImportError as exc:
        print(f"Plot generation skipped: {exc}")
        print("Install dependency with: pip install matplotlib")

    print(summarize(points))
    print(f"CSV:  {csv_path}")
    print(f"Map:  {map_path}")
    print(f"Plot: {plot_path}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
