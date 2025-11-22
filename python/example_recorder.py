#!/usr/bin/env python3
"""
Advanced example: Track multiple actors and save pose data to file
"""

from remotecaptury import RemoteCaptury
import time
import json
from collections import defaultdict

class PoseRecorder:
    """Records pose data for multiple actors"""
    
    def __init__(self):
        self.poses = defaultdict(list)
        self.actor_info = {}
        
    def on_pose(self, actor_id, pose):
        """Store pose data"""
        self.poses[actor_id].append({
            'timestamp': pose['timestamp'],
            'num_transforms': len(pose['transforms']),
            'root_position': pose['transforms'][0]['translation'] if pose['transforms'] else None
        })
        
        # Print summary every 50 frames
        if len(self.poses[actor_id]) % 50 == 0:
            print(f"Actor {actor_id}: {len(self.poses[actor_id])} frames recorded")
    
    def on_actor(self, actor_id, mode):
        """Track actor status"""
        mode_names = {0: "SCALING", 1: "TRACKING", 2: "STOPPED", 3: "DELETED", 4: "UNKNOWN"}
        status = mode_names.get(mode, 'UNKNOWN')
        print(f"Actor {actor_id}: {status}")
        
        if mode == 1:  # TRACKING
            self.actor_info[actor_id] = {'status': 'active', 'start_time': time.time()}
        elif mode == 3:  # DELETED
            if actor_id in self.actor_info:
                self.actor_info[actor_id]['end_time'] = time.time()
    
    def save_to_file(self, filename):
        """Save recorded data to JSON file"""
        data = {
            'actors': self.actor_info,
            'poses': {str(k): v for k, v in self.poses.items()}
        }
        
        with open(filename, 'w') as f:
            json.dump(data, f, indent=2)
        
        print(f"\nSaved data to {filename}")
        
        # Print statistics
        print("\nRecording Statistics:")
        for actor_id, poses in self.poses.items():
            print(f"  Actor {actor_id}: {len(poses)} frames")

def main():
    print("RemoteCaptury Advanced Example - Pose Recorder")
    print("=" * 60)
    
    # Create recorder
    recorder = PoseRecorder()
    
    # Create and connect
    rc = RemoteCaptury()
    print("\nConnecting to CapturyLive...")
    if not rc.connect("127.0.0.1", 2101):
        print("Failed to connect!")
        return
    print("✓ Connected")
    
    # Register callbacks
    rc.register_pose_callback(recorder.on_pose)
    rc.register_actor_callback(recorder.on_actor)
    
    # Start streaming
    print("\nStarting recording...")
    rc.start_streaming(1)
    
    # Record for specified duration
    duration = 30  # seconds
    print(f"Recording for {duration} seconds...")
    print("(Press Ctrl+C to stop early)\n")
    
    try:
        time.sleep(duration)
    except KeyboardInterrupt:
        print("\n\nRecording interrupted by user")
    
    # Stop and disconnect
    print("\nStopping...")
    rc.stop_streaming()
    rc.disconnect()
    
    # Save data
    output_file = f"recording_{int(time.time())}.json"
    recorder.save_to_file(output_file)
    
    print("\n✓ Done!")

if __name__ == "__main__":
    main()
