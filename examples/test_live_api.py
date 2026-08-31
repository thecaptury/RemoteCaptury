#!/usr/bin/env python3
"""Smoke test for the public RemoteCaptury Python API against a live server."""

import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BUILD_DIR = ROOT / "build"
if BUILD_DIR.exists():
    sys.path.insert(0, str(BUILD_DIR))

import remotecaptury as rc


def expect(cond, msg):
    if not cond:
        raise AssertionError(msg)


def main():
    # use auto-discover
    if not rc.connect():
        # fall-back to hard-coded IP
        if not rc.connect("127.0.0.1", 2101):
            raise SystemExit("failed to connect to 127.0.0.1:2101")

    expect(rc.getConnectionStatus() in (rc.CAPTURY_CONNECTED, rc.CAPTURY_CONNECTING), "bad status")

    # Basic timing API should respond even before a pose is available.
    ts = rc.getTime()
    offset = rc.getTimeOffset()
    print(f"time={ts} offset={offset}")

    # The server emits the pose callback only when compressed poses are included.
    stream_flags = rc.CAPTURY_STREAM_POSES | rc.CAPTURY_STREAM_ANGLES | rc.CAPTURY_STREAM_COMPRESSED
    assert rc.startStreaming(stream_flags), "startStreaming failed"

    time.sleep(1.0)

    actors = rc.getActors() or []
    print(f"actors={len(actors)}")
    if actors:
        actor_id = actors[0]["id"]
        pose = rc.getCurrentPose(actor_id)
        print(f"pose for actor {actor_id}: {pose is not None}")
        if pose:
            expect("transforms" in pose, "pose missing transforms")
            expect(len(pose["transforms"]) >= 0, "invalid transform count")

        angles = rc.getCurrentAngles(actor_id) or []
        print(f"angles={len(angles)}")

    cameras = rc.getCameras(200) or []
    print(f"cameras={len(cameras)}")
    if cameras:
        print("first camera:", cameras[0]["name"])

    artags = rc.getCurrentARTags() or []
    print(f"artags={len(artags)}")

    assert rc.stopStreaming(), "stopStreaming failed"
    rc.disconnect()
    print("live api smoke test passed")


if __name__ == "__main__":
    main()
