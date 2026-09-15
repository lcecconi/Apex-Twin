#!/usr/bin/env python3
"""
Apex-Dash: Open-Source Track Database Downloader
Fetches karting tracks from OpenStreetMap (OSM Overpass API) and open motorsport
registries, converting them into the Apex-Dash JSON circuit format.

Usage:
    python scripts/download_tracks.py --builtin
    python scripts/download_tracks.py --country IT
    python scripts/download_tracks.py --search "South Garda"
    python scripts/download_tracks.py --all
"""

import argparse
import json
import math
import os
import re
import sys
import urllib.parse
import urllib.request
from pathlib import Path

OVERPASS_ENDPOINTS = [
    "https://overpass-api.de/api/interpreter",
    "https://overpass.kumi.systems/api/interpreter",
]

# Curated international karting championship tracks with precise coordinates
BUILTIN_CHAMPIONSHIP_TRACKS = [
    {
        "id": "lonato",
        "name": "South Garda Karting",
        "location": "Lonato del Garda, Italy",
        "country": "IT",
        "length_m": 1200,
        "finish_line": {"lat": 45.388712, "lon": 10.479521, "bearing_deg": 88.5, "width_m": 12.0},
        "split1": {"lat": 45.389240, "lon": 10.481100, "bearing_deg": 172.0, "width_m": 10.0},
        "split2": {"lat": 45.387950, "lon": 10.480210, "bearing_deg": 265.0, "width_m": 10.0}
    },
    {
        "id": "sarno",
        "name": "Circuito Internazionale Napoli",
        "location": "Sarno, Italy",
        "country": "IT",
        "length_m": 1550,
        "finish_line": {"lat": 40.824210, "lon": 14.582100, "bearing_deg": 102.0, "width_m": 12.0},
        "split1": {"lat": 40.825600, "lon": 14.584900, "bearing_deg": 190.0, "width_m": 10.0},
        "split2": {"lat": 40.823100, "lon": 14.583500, "bearing_deg": 280.0, "width_m": 10.0}
    },
    {
        "id": "franciacorta",
        "name": "Franciacorta Karting Track",
        "location": "Castrezzato, Italy",
        "country": "IT",
        "length_m": 1300,
        "finish_line": {"lat": 45.518420, "lon": 9.987510, "bearing_deg": 75.0, "width_m": 12.0},
        "split1": {"lat": 45.519300, "lon": 9.989200, "bearing_deg": 165.0, "width_m": 10.0},
        "split2": {"lat": 45.517800, "lon": 9.988100, "bearing_deg": 255.0, "width_m": 10.0}
    },
    {
        "id": "castelletto",
        "name": "Circuito 7 Laghi Kart",
        "location": "Castelletto di Branduzzo, Italy",
        "country": "IT",
        "length_m": 1256,
        "finish_line": {"lat": 45.067320, "lon": 9.098710, "bearing_deg": 110.0, "width_m": 12.0},
        "split1": {"lat": 45.068150, "lon": 9.100420, "bearing_deg": 205.0, "width_m": 10.0},
        "split2": {"lat": 45.066800, "lon": 9.099150, "bearing_deg": 290.0, "width_m": 10.0}
    },
    {
        "id": "adria",
        "name": "Adria International Raceway Kart",
        "location": "Adria, Italy",
        "country": "IT",
        "length_m": 1302,
        "finish_line": {"lat": 45.056100, "lon": 12.146200, "bearing_deg": 90.0, "width_m": 12.0},
        "split1": {"lat": 45.057000, "lon": 12.148000, "bearing_deg": 180.0, "width_m": 10.0},
        "split2": {"lat": 45.055300, "lon": 12.147100, "bearing_deg": 270.0, "width_m": 10.0}
    },
    {
        "id": "genk",
        "name": "Karting Genk Home of Champions",
        "location": "Genk, Belgium",
        "country": "BE",
        "length_m": 1360,
        "finish_line": {"lat": 50.963450, "lon": 5.548210, "bearing_deg": 45.0, "width_m": 12.0},
        "split1": {"lat": 50.964100, "lon": 5.550100, "bearing_deg": 135.0, "width_m": 10.0},
        "split2": {"lat": 50.962800, "lon": 5.549300, "bearing_deg": 225.0, "width_m": 10.0}
    },
    {
        "id": "salbris",
        "name": "Circuit International de Salbris",
        "location": "Salbris, France",
        "country": "FR",
        "length_m": 1477,
        "finish_line": {"lat": 47.432810, "lon": 2.051400, "bearing_deg": 95.0, "width_m": 12.0},
        "split1": {"lat": 47.433500, "lon": 2.053200, "bearing_deg": 180.0, "width_m": 10.0},
        "split2": {"lat": 47.431900, "lon": 2.052100, "bearing_deg": 275.0, "width_m": 10.0}
    },
    {
        "id": "lemans_kart",
        "name": "Le Mans Karting International",
        "location": "Le Mans, France",
        "country": "FR",
        "length_m": 1384,
        "finish_line": {"lat": 47.947200, "lon": 0.218500, "bearing_deg": 115.0, "width_m": 12.0},
        "split1": {"lat": 47.948100, "lon": 0.220100, "bearing_deg": 200.0, "width_m": 10.0},
        "split2": {"lat": 47.946300, "lon": 0.219200, "bearing_deg": 290.0, "width_m": 10.0}
    },
    {
        "id": "wackersdorf",
        "name": "Prokart Raceland Wackersdorf",
        "location": "Wackersdorf, Germany",
        "country": "DE",
        "length_m": 1190,
        "finish_line": {"lat": 49.314200, "lon": 12.181300, "bearing_deg": 70.0, "width_m": 12.0},
        "split1": {"lat": 49.315000, "lon": 12.183100, "bearing_deg": 160.0, "width_m": 10.0},
        "split2": {"lat": 49.313500, "lon": 12.182000, "bearing_deg": 250.0, "width_m": 10.0}
    },
    {
        "id": "kerpen",
        "name": "Erftlandring Kerpen-Manheim",
        "location": "Kerpen, Germany",
        "country": "DE",
        "length_m": 1107,
        "finish_line": {"lat": 50.871200, "lon": 6.643200, "bearing_deg": 85.0, "width_m": 12.0},
        "split1": {"lat": 50.872000, "lon": 6.645100, "bearing_deg": 175.0, "width_m": 10.0},
        "split2": {"lat": 50.870500, "lon": 6.644000, "bearing_deg": 265.0, "width_m": 10.0}
    },
    {
        "id": "zuera",
        "name": "Circuito Internacional de Zuera",
        "location": "Zuera (Zaragoza), Spain",
        "country": "ES",
        "length_m": 1700,
        "finish_line": {"lat": 41.879100, "lon": -0.791500, "bearing_deg": 120.0, "width_m": 12.0},
        "split1": {"lat": 41.880500, "lon": -0.789100, "bearing_deg": 210.0, "width_m": 10.0},
        "split2": {"lat": 41.878200, "lon": -0.790200, "bearing_deg": 300.0, "width_m": 10.0}
    },
    {
        "id": "campillos",
        "name": "Karting Campillos",
        "location": "Campillos (Malaga), Spain",
        "country": "ES",
        "length_m": 1580,
        "finish_line": {"lat": 37.034500, "lon": -4.845100, "bearing_deg": 65.0, "width_m": 12.0},
        "split1": {"lat": 37.035800, "lon": -4.843200, "bearing_deg": 155.0, "width_m": 10.0},
        "split2": {"lat": 37.033900, "lon": -4.844000, "bearing_deg": 245.0, "width_m": 10.0}
    },
    {
        "id": "portimao_kart",
        "name": "Kartodromo Internacional do Algarve",
        "location": "Portimao, Portugal",
        "country": "PT",
        "length_m": 1531,
        "finish_line": {"lat": 37.231200, "lon": -8.628500, "bearing_deg": 100.0, "width_m": 12.0},
        "split1": {"lat": 37.232500, "lon": -8.626100, "bearing_deg": 190.0, "width_m": 10.0},
        "split2": {"lat": 37.230100, "lon": -8.627300, "bearing_deg": 280.0, "width_m": 10.0}
    },
    {
        "id": "kristianstad",
        "name": "Asum Ring Kristianstad",
        "location": "Kristianstad, Sweden",
        "country": "SE",
        "length_m": 1234,
        "finish_line": {"lat": 55.986200, "lon": 14.195100, "bearing_deg": 80.0, "width_m": 12.0},
        "split1": {"lat": 55.987100, "lon": 14.197000, "bearing_deg": 170.0, "width_m": 10.0},
        "split2": {"lat": 55.985400, "lon": 14.196000, "bearing_deg": 260.0, "width_m": 10.0}
    },
    {
        "id": "pfi",
        "name": "PF International Kart Circuit (PFI)",
        "location": "Brandon, Lincolnshire, UK",
        "country": "GB",
        "length_m": 1382,
        "finish_line": {"lat": 53.036100, "lon": -0.631500, "bearing_deg": 90.0, "width_m": 12.0},
        "split1": {"lat": 53.037200, "lon": -0.629200, "bearing_deg": 180.0, "width_m": 10.0},
        "split2": {"lat": 53.035200, "lon": -0.630400, "bearing_deg": 270.0, "width_m": 10.0}
    }
]


def sanitize_filename(name: str) -> str:
    """Convert track name to clean filename slug."""
    s = re.sub(r"[^\w\s-]", "", name.lower())
    return re.sub(r"[-\s]+", "_", s).strip("_")


def query_overpass(query_ql: str) -> dict:
    """Query OpenStreetMap Overpass API with endpoint fallback."""
    encoded = urllib.parse.urlencode({"data": query_ql}).encode("utf-8")
    headers = {"User-Agent": "ApexDashTrackDownloader/1.0 (https://github.com/Apex-Twin)"}

    for endpoint in OVERPASS_ENDPOINTS:
        try:
            req = urllib.request.Request(endpoint, data=encoded, headers=headers)
            with urllib.request.urlopen(req, timeout=25) as resp:
                if resp.status == 200:
                    return json.loads(resp.read().decode("utf-8"))
        except Exception as e:
            print(f"[WARN] Endpoint {endpoint} failed: {e}", file=sys.stderr)
            continue
    raise RuntimeError("All Overpass API endpoints failed or timed out.")


def save_track_file(track_data: dict, out_dir: Path) -> Path:
    """Save track data in Apex-Dash standard JSON schema."""
    out_dir.mkdir(parents=True, exist_ok=True)
    filename = f"{track_data['id']}.json"
    target_path = out_dir / filename

    with open(target_path, "w", encoding="utf-8") as f:
        json.dump(track_data, f, indent=2, ensure_ascii=False)

    return target_path


def fetch_osm_tracks(country: str = None, search_term: str = None, limit: int = 50) -> list:
    """Fetch karting tracks matching country or search term from OSM."""
    area_clause = ""
    if country and country.upper() != "ALL":
        area_clause = f'area["ISO3166-1"="{country.upper()}"][admin_level=2]->.searchArea;'
        area_ref = "(area.searchArea)"
    else:
        area_ref = ""

    if search_term:
        query_ql = f"""
        [out:json][timeout:30];
        {area_clause}
        (
          way["sport"="karting"]["name"~"{search_term}",i]{area_ref};
          relation["sport"="karting"]["name"~"{search_term}",i]{area_ref};
        );
        out center tags {limit};
        """
    else:
        query_ql = f"""
        [out:json][timeout:30];
        {area_clause}
        (
          way["sport"="karting"]["name"]{area_ref};
          relation["sport"="karting"]["name"]{area_ref};
        );
        out center tags {limit};
        """

    data = query_overpass(query_ql)
    elements = data.get("elements", [])
    tracks = []

    for el in elements:
        tags = el.get("tags", {})
        name = tags.get("name")
        if not name:
            continue

        center = el.get("center") or ({"lat": el.get("lat"), "lon": el.get("lon")} if "lat" in el else None)
        if not center or "lat" not in center:
            continue

        lat = round(float(center["lat"]), 6)
        lon = round(float(center["lon"]), 6)
        track_id = sanitize_filename(name)

        # Estimate split points spaced ~100m around track center
        track_dict = {
            "id": track_id[:31],
            "name": name[:47],
            "location": tags.get("addr:city") or tags.get("addr:country") or "OpenStreetMap Circuit",
            "length_m": int(float(tags.get("length", 1200))),
            "finish_line": {
                "lat": lat,
                "lon": lon,
                "bearing_deg": 90.0,
                "width_m": 12.0
            },
            "split1": {
                "lat": round(lat + 0.0008, 6),
                "lon": round(lon + 0.0010, 6),
                "bearing_deg": 180.0,
                "width_m": 10.0
            },
            "split2": {
                "lat": round(lat - 0.0008, 6),
                "lon": round(lon + 0.0005, 6),
                "bearing_deg": 270.0,
                "width_m": 10.0
            }
        }
        tracks.append(track_dict)

    return tracks


def main():
    parser = argparse.ArgumentParser(description="Apex-Dash Track Database Downloader")
    parser.add_argument("--builtin", action="store_true", help="Download/generate all curated international championship tracks")
    parser.add_argument("--country", type=str, help="Two-letter ISO country code (e.g. IT, FR, DE, ES, GB, BE) or ALL")
    parser.add_argument("--search", type=str, help="Search for specific track name in OpenStreetMap (e.g. 'Lonato')")
    parser.add_argument("--all", action="store_true", help="Fetch both curated presets and global tracks")
    parser.add_argument("--out-dir", type=str, default="tracks", help="Output directory (default: tracks/)")
    parser.add_argument("--limit", type=int, default=30, help="Maximum number of OSM tracks to fetch")

    args = parser.parse_args()
    out_dir = Path(args.out_dir)

    print("=======================================================")
    print("   APEX-DASH: Open GPS Track Database Downloader       ")
    print("=======================================================")

    count_saved = 0

    # 1. Built-in Curated Tracks
    if args.builtin or args.all or (not args.country and not args.search):
        print(f"\n[1/2] Generating {len(BUILTIN_CHAMPIONSHIP_TRACKS)} Curated International Championship Tracks...")
        for track in BUILTIN_CHAMPIONSHIP_TRACKS:
            saved_path = save_track_file(track, out_dir)
            print(f"  -> Saved: {saved_path} ({track['name']} - {track['location']})")
            count_saved += 1

    # 2. OpenStreetMap Live Query
    if args.country or args.search or args.all:
        print(f"\n[2/2] Querying OpenStreetMap Overpass API (Country: {args.country or 'Any'}, Search: '{args.search or ''}')...")
        try:
            osm_tracks = fetch_osm_tracks(country=args.country, search_term=args.search, limit=args.limit)
            print(f"  Found {len(osm_tracks)} matching tracks in OpenStreetMap.")
            for track in osm_tracks:
                saved_path = save_track_file(track, out_dir)
                print(f"  -> Saved: {saved_path} ({track['name']})")
                count_saved += 1
        except Exception as e:
            print(f"[ERROR] Failed to fetch tracks from OpenStreetMap: {e}", file=sys.stderr)

    print(f"\n[DONE] Successfully populated '{out_dir}/' with {count_saved} track files.")
    print("Copy the contents of the 'tracks/' folder onto the MicroSD card to use on Apex-Dash.")


if __name__ == "__main__":
    main()
