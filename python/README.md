# RemoteCaptury Python Wrapper

A Pythonic wrapper for the RemoteCaptury library, providing an easy-to-use interface for streaming motion capture data from CapturyLive.

## Features

- **Callback-based API**: Register Python callbacks for pose data and actor status changes
- **Pythonic Interface**: Clean, object-oriented API that feels natural in Python
- **Easy Installation**: Install via pip with automatic C++ extension building

## Installation

### From Source

```bash
cd python
pip install .
```

### Requirements

- Python 3.6+
- C++ compiler (g++ on Linux, MSVC on Windows)
- CapturyLive server running

## Quick Start

```python
from remotecaptury import RemoteCaptury
import time

# Define callbacks
def on_pose(actor_id, pose):
    print(f"Received pose for actor {actor_id}")
    print(f"  Timestamp: {pose['timestamp']}")
    print(f"  Transforms: {len(pose['transforms'])}")

def on_actor(actor_id, mode):
    print(f"Actor {actor_id} status changed to {mode}")

# Create client and connect
rc = RemoteCaptury()
rc.connect("127.0.0.1", 2101)

# Register callbacks
rc.register_pose_callback(on_pose)
rc.register_actor_callback(on_actor)

# Start streaming
rc.start_streaming()

# Let it run for a while
time.sleep(10)

# Clean up
rc.stop_streaming()
rc.disconnect()
```

## API Reference

### RemoteCaptury Class

#### `connect(host, port=2101)`
Connect to a CapturyLive server.

**Parameters:**
- `host` (str): IP address or hostname of the server
- `port` (int): Port number (default: 2101)

**Returns:** `bool` - True if connected successfully

#### `disconnect()`
Disconnect from the server.

#### `start_streaming(what=1)`
Start streaming data from the server.

**Parameters:**
- `what` (int): Bitmask of what to stream (default: 1 for poses)

**Returns:** `bool` - True if streaming started successfully

#### `stop_streaming()`
Stop streaming data.

**Returns:** `bool` - True if streaming stopped successfully

#### `register_pose_callback(callback)`
Register a callback function for new pose data.

**Parameters:**
- `callback` (callable): Function with signature `callback(actor_id: int, pose: dict)`

The `pose` dictionary contains:
- `actor` (int): Actor ID
- `timestamp` (int): Timestamp in microseconds
- `transforms` (list): List of transform dictionaries, each containing:
  - `translation` (list): [x, y, z] translation
  - `rotation` (list): [x, y, z] Euler angles
- `blendShapes` (list, optional): Blend shape activations

**Returns:** `bool` - True if callback registered successfully

#### `register_actor_callback(callback)`
Register a callback function for actor status changes.

**Parameters:**
- `callback` (callable): Function with signature `callback(actor_id: int, mode: int)`

Mode values:
- `0`: ACTOR_SCALING
- `1`: ACTOR_TRACKING
- `2`: ACTOR_STOPPED
- `3`: ACTOR_DELETED
- `4`: ACTOR_UNKNOWN

**Returns:** `bool` - True if callback registered successfully

## Testing

A test script is included:

```bash
# Make sure CapturyLive is running
python test_remotecaptury.py
```

## License

See the main RemoteCaptury license.

## Contributing

This is a wrapper around the RemoteCaptury C++ library. For issues with the underlying library, please refer to the main repository.
