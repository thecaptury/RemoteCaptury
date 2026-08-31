#!/usr/bin/env python3
"""Minimal live demo against the local Captury streaming server."""

import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BUILD_DIR = ROOT / "build"
if BUILD_DIR.exists():
    sys.path.insert(0, str(BUILD_DIR))

import remotecaptury as rc


def on_pose(actor_id, pose):
    print(f"pose actor={actor_id} ts={pose.get('timestamp')} transforms={len(pose.get('transforms', []))}")


def on_actor(actor_id, mode):
    print(f"actor_changed actor={actor_id} mode={mode}")


def on_angles(actor, values):
    print(f"angles actor={actor.get('id')} count={len(values)} first={values[0] if values else None}")


def on_artag(num, artags):
    print(f"artags count={num} items={len(artags)}")


def main():
    # use auto-discover
    if not rc.connect():
        # fall-back to hard-coded IP
        if not rc.connect("127.0.0.1", 2101):
            raise SystemExit("failed to connect to 127.0.0.1:2101")


    print("connection status:", rc.getConnectionStatus())
    rc.registerNewPoseCallback(on_pose)
    rc.registerActorChangedCallback(on_actor)
    rc.registerNewAnglesCallback(on_angles)
    rc.registerARTagCallback(on_artag)

    # The C API requires compressed pose packets for the pose callback path to fire.
    stream_flags = rc.CAPTURY_STREAM_POSES | rc.CAPTURY_STREAM_ANGLES | rc.CAPTURY_STREAM_COMPRESSED
    if not rc.startStreaming(stream_flags):
        raise SystemExit("failed to start streaming")

    try:
        for _ in range(10):
            actors = rc.getActors() or []
            if actors:
                print("actors:", [a["id"] for a in actors])
            artags = rc.getCurrentARTags() or []
            if artags:
                print("current artags:", artags[:3])
            time.sleep(1.0)
    finally:
        rc.stopStreaming()
        rc.disconnect()


if __name__ == "__main__":
    main()
