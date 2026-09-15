import argparse
import re
import time
from pathlib import Path

import serial


BEGIN_PATTERN = re.compile(rb"^SDDUMP_BEGIN (\d+)\r?\n$")
FILE_PATTERN = re.compile(rb"^SDDUMP_FILE (.+) (\d+)\r?\n$")


def read_line(port):
    return port.readline()


def wait_until_ready(port, timeout):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        line = read_line(port)
        if line in (b"SDDUMP_READY\r\n", b"SDDUMP_READY\n"):
            return
    raise RuntimeError("SD dump target did not become ready")


def list_files(port_name, directory_path, baud_rate, timeout):
    with serial.Serial(port_name, baud_rate, timeout=timeout, write_timeout=timeout) as port:
        wait_until_ready(port, timeout)
        port.write(f"LIST {directory_path}\n".encode("ascii"))

        header = read_line(port)
        expected_header = f"SDDUMP_LIST_BEGIN {directory_path or '/'}".encode("ascii")
        if header.rstrip(b"\r\n") != expected_header:
            raise RuntimeError(f"Unexpected target response: {header!r}")

        files = []
        while True:
            line = read_line(port)
            if line.rstrip(b"\r\n") == b"SDDUMP_LIST_END":
                break
            match = FILE_PATTERN.match(line)
            if not match:
                raise RuntimeError(f"Unexpected target response: {line!r}")
            files.append((match.group(1).decode("utf-8", errors="replace"), int(match.group(2))))

    for path, size in files:
        print(f"{size:>10} {path}")
    print(f"{len(files)} file(s)")


def download(port_name, source_path, destination_path, baud_rate, timeout):
    with serial.Serial(port_name, baud_rate, timeout=timeout, write_timeout=timeout) as port:
        wait_until_ready(port, timeout)

        command = f"DUMP {source_path}\n".encode("ascii")
        port.write(command)
        header = read_line(port)
        match = BEGIN_PATTERN.match(header)
        if not match:
            raise RuntimeError(f"Unexpected target response: {header!r}")

        expected_size = int(match.group(1))
        remaining = expected_size
        with Path(destination_path).open("wb") as destination:
            while remaining:
                chunk = port.read(min(remaining, 4096))
                if not chunk:
                    raise TimeoutError("Timed out while receiving SD data")
                destination.write(chunk)
                remaining -= len(chunk)

        trailer = read_line(port)
        expected_trailer = f"SDDUMP_END {expected_size}".encode("ascii")
        if trailer.rstrip(b"\r\n") != expected_trailer:
            raise RuntimeError(f"Invalid dump trailer: {trailer!r}")

    print(f"Downloaded {expected_size} bytes from {source_path} to {destination_path}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Download a file from the ESP32 SD dump target")
    parser.add_argument("port", help="ESP32 USB serial port, for example COM7")
    parser.add_argument("--list", dest="list_path", metavar="PATH", help="List all files recursively under PATH")
    parser.add_argument("-s", "--source", default="/gngga.log", help="SD path to download")
    parser.add_argument("-o", "--output", default="gngga.log", help="PC output file")
    parser.add_argument("-b", "--baud", type=int, default=460800)
    parser.add_argument("--timeout", type=float, default=10.0)
    args = parser.parse_args()

    if args.list_path is not None:
        list_files(args.port, args.list_path, args.baud, args.timeout)
    else:
        download(args.port, args.source, args.output, args.baud, args.timeout)