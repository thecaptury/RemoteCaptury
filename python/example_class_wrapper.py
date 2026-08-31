#!/usr/bin/env python3
"""Example using the higher-level RemoteCaptury class wrapper."""

import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BUILD_DIR = ROOT / "build"
if BUILD_DIR.exists():
    sys.path.insert(0, str(BUILD_DIR))

from remotecaptury import RemoteCaptury


def on_pose(actor_id, pose):
    print(f"pose actor={actor_id} ts={pose.get('timestamp')} transforms={len(pose.get('transforms', []))}")


def on_actor(actor_id, mode):
    print(f"actor_changed actor={actor_id} mode={mode}")


def main():
    rc = RemoteCaptury()
    if not rc.connect():
        if not rc.connect("127.0.0.1", 2101):
            raise SystemExit("failed to connect to CapturyLive")

    rc.register_pose_callback(on_pose)
    rc.register_actor_callback(on_actor)

    # The callback-based pose stream must include compressed pose packets.
    stream_flags = (
        rc.CAPTURY_STREAM_POSES
        | rc.CAPTURY_STREAM_ANGLES
        | rc.CAPTURY_STREAM_COMPRESSED
    )
    if not rc.start_streaming(stream_flags):
        raise SystemExit("failed to start streaming")

    try:
        for _ in range(5):
            actors = rc.get_actors() or []
            print("actors:", [a["id"] for a in actors])
            time.sleep(1.0)
    finally:
        rc.stop_streaming()
        rc.disconnect()


if __name__ == "__main__":
    main()
