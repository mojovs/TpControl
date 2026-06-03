#!/usr/bin/env python3
"""Upload a resource file to TpControl over USART1.

Usage:
    python Tools/upload_resource.py COM3 /images/fire.bin fire.bin
    python Tools/upload_resource.py COM3 /font/ascii_1206.bin ascii_1206.bin --baud 115200
"""

import argparse
import binascii
import pathlib
import sys
import time

try:
    import serial
except ImportError:  # pragma: no cover
    serial = None


def read_line(ser, timeout=5.0):
    deadline = time.time() + timeout
    data = bytearray()
    while time.time() < deadline:
        ch = ser.read(1)
        if not ch:
            continue
        if ch == b"\r":
            continue
        if ch == b"\n":
            return data.decode("ascii", errors="replace")
        data.extend(ch)
    raise TimeoutError("timeout waiting for line")


def write_line(ser, line):
    ser.write(line.encode("ascii") + b"\n")
    ser.flush()


def wait_for_prefix(ser, prefix, timeout=10.0):
    while True:
        line = read_line(ser, timeout)
        print("<", line)
        if line.startswith(prefix):
            return line
        if line.startswith("ERR"):
            raise RuntimeError(line)


def main():
    parser = argparse.ArgumentParser(description="Upload a file to W25Q80 littlefs over USART1")
    parser.add_argument("port", help="serial port, for example COM3 or /dev/ttyUSB0")
    parser.add_argument("target", help="target littlefs path, for example /images/fire.bin")
    parser.add_argument("file", help="local file to upload")
    parser.add_argument("--baud", type=int, default=115200, help="baud rate, default 115200")
    parser.add_argument("--chunk", type=int, default=128, help="write chunk size, default 128")
    args = parser.parse_args()

    if serial is None:
        print("pyserial is required: pip install pyserial", file=sys.stderr)
        return 2

    data = pathlib.Path(args.file).read_bytes()
    crc = binascii.crc32(data) & 0xFFFFFFFF

    with serial.Serial(args.port, args.baud, timeout=0.1, write_timeout=5) as ser:
        time.sleep(0.2)
        ser.reset_input_buffer()

        print("> PING")
        write_line(ser, "PING")
        wait_for_prefix(ser, "OK PONG", timeout=5)

        cmd = f"FS_UPLOAD {args.target} {len(data)} {crc:08X}"
        print(">", cmd)
        write_line(ser, cmd)
        wait_for_prefix(ser, "OK READY", timeout=10)

        sent = 0
        while sent < len(data):
            chunk = data[sent:sent + args.chunk]
            ser.write(chunk)
            ser.flush()
            sent += len(chunk)
            wait_for_prefix(ser, f"OK CHUNK {sent}", timeout=10)
            print(f"\ruploaded {sent}/{len(data)}", end="")
        print()

        wait_for_prefix(ser, "OK DONE", timeout=30)
        print("upload ok")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
