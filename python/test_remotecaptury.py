#!/usr/bin/env python3
"""Test script for RemoteCaptury Python wrapper"""

from remotecaptury import RemoteCaptury
import time

# Callback for pose data
def on_pose(actor_id, pose):
    print(f"Pose received for actor {actor_id}")
    print(f"  Timestamp: {pose['timestamp']}")
    print(f"  Number of transforms: {len(pose['transforms'])}")
    if len(pose['transforms']) > 0:
        print(f"  First transform translation: {pose['transforms'][0]['translation']}")

# Callback for actor changes
def on_actor(actor_id, mode):
    mode_names = {0: "SCALING", 1: "TRACKING", 2: "STOPPED", 3: "DELETED", 4: "UNKNOWN"}
    print(f"Actor {actor_id} changed to mode: {mode_names.get(mode, 'UNKNOWN')}")

def main():
    print("RemoteCaptury Python Wrapper Test")
    print("=" * 50)
    
    # Create instance
    rc = RemoteCaptury()
    
    # Connect to server
    print("\nConnecting to CapturyLive at 127.0.0.1:2101...")
    if rc.connect("127.0.0.1", 2101):
        print("✓ Connected successfully!")
    else:
        print("✗ Connection failed!")
        return
    
    # Register callbacks
    print("\nRegistering callbacks...")
    rc.register_pose_callback(on_pose)
    rc.register_actor_callback(on_actor)
    print("✓ Callbacks registered")
    
    # Start streaming
    print("\nStarting streaming...")
    if rc.start_streaming(1):  # 1 = CAPTURY_STREAM_POSES
        print("✓ Streaming started")
    else:
        print("✗ Failed to start streaming")
        return
    
    # Wait for data
    print("\nListening for data (10 seconds)...")
    try:
        time.sleep(10)
    except KeyboardInterrupt:
        print("\nInterrupted by user")
    
    # Stop streaming
    print("\nStopping streaming...")
    rc.stop_streaming()
    print("✓ Streaming stopped")
    
    # Disconnect
    print("\nDisconnecting...")
    rc.disconnect()
    print("✓ Disconnected")
    
    print("\nTest completed!")

if __name__ == "__main__":
    main()
