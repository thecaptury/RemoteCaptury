import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BUILD_DIR = ROOT / "build"
if BUILD_DIR.exists():
    sys.path.insert(0, str(BUILD_DIR))

import remotecaptury as rc


def main():
    # use auto-discover
    if not rc.connect():
        # fall-back to hard-coded IP
        if not rc.connect("127.0.0.1", 2101):
            raise SystemExit("failed to connect to 127.0.0.1:2101")

    print("connection status:", rc.getConnectionStatus())

    # Keep remote and local clocks aligned.
    rc.startTimeSynchronizationLoop()

    # Stream poses and basic metadata.
    what = rc.CAPTURY_STREAM_POSES | rc.CAPTURY_STREAM_META_DATA
    if not rc.startStreaming(what):
        raise SystemExit("failed to start streaming")

    time.sleep(2.0)

    actors = rc.getActors() or []
    print(f"actors: {len(actors)}")
    for actor in actors:
        pose = rc.getCurrentPose(actor["id"])
        print(f"actor {actor['id']} pose: {bool(pose)}")
        if pose:
            print(f"  transforms={len(pose.get('transforms', []))}")
            print(f"  timestamp={pose.get('timestamp')}")

    cameras = rc.getCameras(200) or []
    print(f"cameras: {len(cameras)}")
    for camera in cameras[:3]:
        print(f"  camera {camera['id']} {camera['name']}")

    rc.stopStreaming()
    rc.disconnect()
    print("done")


if __name__ == "__main__":
    main()
