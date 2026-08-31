import importlib.util
import sys
from pathlib import Path


def _load_backend_module():
    """Load the compiled RemoteCaptury backend without colliding with the package itself."""
    try:
        import _remotecaptury as backend
        return backend
    except ImportError:
        pass

    package_root = Path(__file__).resolve().parents[1]
    project_root = package_root.parent
    for build_dir in (project_root / "build", package_root):
        for filename in ("_remotecaptury.so", "_remotecaptury.pyd", "remotecaptury.so", "remotecaptury.pyd"):
            candidate = build_dir / filename
            if not candidate.exists():
                continue
            spec = importlib.util.spec_from_file_location("_remotecaptury", candidate)
            if spec is None or spec.loader is None:
                continue
            module = importlib.util.module_from_spec(spec)
            sys.modules["_remotecaptury"] = module
            spec.loader.exec_module(module)
            return module

    raise ImportError("Unable to locate the RemoteCaptury Python extension module")


_remotecaptury = _load_backend_module()


def _backend_constant(name, default=None):
    """Read a backend constant if exported; otherwise return a safe fallback."""
    return getattr(_remotecaptury, name, default)


class RemoteCaptury:
    """Thin, Pythonic wrapper around the public RemoteCaptury C-extension API."""

    # Expose the most important stream and connection constants on the class itself.
    # Keep a fallback for older wheel builds that may not export every constant.
    CAPTURY_DISCONNECTED = _backend_constant("CAPTURY_DISCONNECTED", 0)
    CAPTURY_CONNECTING = _backend_constant("CAPTURY_CONNECTING", 1)
    CAPTURY_CONNECTED = _backend_constant("CAPTURY_CONNECTED", 2)
    CAPTURY_STREAM_NOTHING = _backend_constant("CAPTURY_STREAM_NOTHING", 0)
    CAPTURY_STREAM_POSES = _backend_constant("CAPTURY_STREAM_POSES", 1)
    CAPTURY_STREAM_GLOBAL_POSES = _backend_constant("CAPTURY_STREAM_GLOBAL_POSES", 2)
    CAPTURY_STREAM_LOCAL_POSES = _backend_constant("CAPTURY_STREAM_LOCAL_POSES", 4)
    CAPTURY_STREAM_ARTAGS = _backend_constant("CAPTURY_STREAM_ARTAGS", 8)
    CAPTURY_STREAM_IMAGES = _backend_constant("CAPTURY_STREAM_IMAGES", 16)
    CAPTURY_STREAM_META_DATA = _backend_constant("CAPTURY_STREAM_META_DATA", 32)
    CAPTURY_STREAM_IMU_DATA = _backend_constant("CAPTURY_STREAM_IMU_DATA", 64)
    CAPTURY_STREAM_LATENCY_INFO = _backend_constant("CAPTURY_STREAM_LATENCY_INFO", 128)
    CAPTURY_STREAM_FOOT_CONTACT = _backend_constant("CAPTURY_STREAM_FOOT_CONTACT", 256)
    CAPTURY_STREAM_COMPRESSED = _backend_constant("CAPTURY_STREAM_COMPRESSED", 0x0100)
    CAPTURY_STREAM_ANGLES = _backend_constant("CAPTURY_STREAM_ANGLES", 0x0200)
    CAPTURY_STREAM_SCALES = _backend_constant("CAPTURY_STREAM_SCALES", 0x0400)
    CAPTURY_STREAM_BLENDSHAPES = _backend_constant("CAPTURY_STREAM_BLENDSHAPES", 0x0800)
    CAPTURY_STREAM_TCP = _backend_constant("CAPTURY_STREAM_TCP", 0x2000)
    CAPTURY_STREAM_ONLY_ROOT_TRANSLATION = _backend_constant("CAPTURY_STREAM_ONLY_ROOT_TRANSLATION", 0x4000)

    def __init__(self, host="", port=2101):
        self._connected = False
        if host:
            self.connect(host, port)

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.disconnect()
        return False

    def __getattr__(self, name):
        if hasattr(_remotecaptury, name):
            value = getattr(_remotecaptury, name)
            if callable(value):
                return value
            return value
        raise AttributeError(f"{type(self).__name__!r} object has no attribute {name!r}")

    def connect(self, host="", port=2101):
        """Connect to the CapturyLive server. Empty host triggers discovery."""
        if _remotecaptury.connect(host, port):
            self._connected = True
            return True
        return False

    def disconnect(self):
        """Disconnect from the server if connected."""
        if self._connected:
            _remotecaptury.disconnect()
            self._connected = False
        return True

    def get_connection_status(self):
        return _remotecaptury.getConnectionStatus()

    def start_streaming(self, what=None):
        """Start streaming. Default is poses plus compressed poses for callback use."""
        if what is None:
            what = self.CAPTURY_STREAM_POSES | self.CAPTURY_STREAM_COMPRESSED
        return _remotecaptury.startStreaming(what)

    def stop_streaming(self, wait=1):
        return _remotecaptury.stopStreaming(wait)

    def start_time_synchronization_loop(self):
        return _remotecaptury.startTimeSynchronizationLoop()

    def synchronize_time(self):
        return _remotecaptury.synchronizeTime()

    def get_time(self):
        return _remotecaptury.getTime()

    def get_time_offset(self):
        return _remotecaptury.getTimeOffset()

    def register_pose_callback(self, callback):
        return _remotecaptury.registerNewPoseCallback(callback)

    def register_actor_changed_callback(self, callback):
        return _remotecaptury.registerActorChangedCallback(callback)

    def register_angles_callback(self, callback):
        return _remotecaptury.registerNewAnglesCallback(callback)

    def register_artag_callback(self, callback):
        return _remotecaptury.registerARTagCallback(callback)

    def register_image_callback(self, callback):
        return _remotecaptury.registerImageStreamingCallback(callback)

    def register_actor_callback(self, callback):
        """Backward-compatible alias for actor status callbacks."""
        return self.register_actor_changed_callback(callback)

    def get_actors(self):
        return _remotecaptury.getActors()

    def get_actor(self, actor_id):
        return _remotecaptury.getActor(actor_id)

    def get_cameras(self, wait_time_ms=0):
        return _remotecaptury.getCameras(wait_time_ms)

    def get_current_pose_for_actor(self, actor_id):
        return _remotecaptury.getCurrentPoseForActor(actor_id)

    def get_current_pose(self, actor_id=-1):
        return _remotecaptury.getCurrentPose(actor_id)

    def get_current_angles(self, actor_id=-1):
        return _remotecaptury.getCurrentAngles(actor_id)

    def get_current_artags(self):
        return _remotecaptury.getCurrentARTags()

    def get_current_image(self):
        return _remotecaptury.getCurrentImage()

    def request_texture(self, actor_id):
        return _remotecaptury.requestTexture(actor_id)

    def get_texture(self, actor_id):
        return _remotecaptury.getTexture(actor_id)

    def get_tracking_quality(self, actor_id):
        return _remotecaptury.getTrackingQuality(actor_id)

    def get_scaling_progress(self, actor_id):
        return _remotecaptury.getScalingProgress(actor_id)

    def set_actor_name(self, actor_id, name):
        return _remotecaptury.setActorName(actor_id, name)

    def start_tracking(self, actor_id, x=0.0, z=0.0, heading=370.0):
        return _remotecaptury.startTracking(actor_id, x, z, heading)

    def stop_tracking(self, actor_id):
        return _remotecaptury.stopTracking(actor_id)

    def delete_actor(self, actor_id):
        return _remotecaptury.deleteActor(actor_id)

    def rescale_actor(self, actor_id):
        return _remotecaptury.rescaleActor(actor_id)

    def recolor_actor(self, actor_id):
        return _remotecaptury.recolorActor(actor_id)

    def update_actor_colors(self, actor_id):
        return _remotecaptury.updateActorColors(actor_id)

    def snap_actor(self, x, z, heading=370.0):
        return _remotecaptury.snapActor(x, z, heading)

    def snap_actor_ex(self, x, z, radius=0.0, heading=370.0, skeleton_name=None, snap_method=0, quick_scaling=0):
        return _remotecaptury.snapActorEx(x, z, radius, heading, skeleton_name, snap_method, quick_scaling)

    def get_marker_transform(self, actor_id, joint):
        return _remotecaptury.getMarkerTransform(actor_id, joint)

    def get_current_latency(self):
        return _remotecaptury.getCurrentLatency()

    def start_recording(self):
        return _remotecaptury.startRecording()

    def stop_recording(self):
        return _remotecaptury.stopRecording()

    def set_shot_name(self, name):
        return _remotecaptury.setShotName(name)

    def discover_servers(self, port=2101, multicast_address=None):
        if multicast_address is None:
            return _remotecaptury.discoverServers(port)
        return _remotecaptury.discoverServers(port, multicast_address)
