import _remotecaptury

class RemoteCaptury:
    """
    A Pythonic wrapper for the RemoteCaptury library.
    """
    def __init__(self):
        self._connected = False

    def connect(self, host, port=2101):
        """
        Connect to the CapturyLive server.
        
        Args:
            host (str): The IP address or hostname of the server.
            port (int): The port number (default: 2101).
            
        Returns:
            bool: True if connected successfully, False otherwise.
        """
        if _remotecaptury.connect(host, port):
            self._connected = True
            return True
        return False

    def disconnect(self):
        """
        Disconnect from the CapturyLive server.
        """
        if self._connected:
            _remotecaptury.disconnect()
            self._connected = False

    def start_streaming(self, what=1):
        """
        Start streaming data.
        
        Args:
            what (int): Bitmask of what to stream (default: 1 for poses).
        """
        return _remotecaptury.startStreaming(what)

    def stop_streaming(self):
        """
        Stop streaming data.
        """
        return _remotecaptury.stopStreaming()

    def register_pose_callback(self, callback):
        """
        Register a callback for new pose data.
        
        Args:
            callback (callable): A function that takes (actor_id, pose_dict).
        """
        return _remotecaptury.registerPoseCallback(callback)

    def register_actor_callback(self, callback):
        """
        Register a callback for actor status changes.
        
        Args:
            callback (callable): A function that takes (actor_id, mode).
        """
        return _remotecaptury.registerActorCallback(callback)
