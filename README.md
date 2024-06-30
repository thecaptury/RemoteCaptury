# RemoteCaptury

RemoteCaptury is a library that provides access to data streamed from Captury Studio, Captury Live, and Captury Replay. In particular, it streams poses and angle data for persons that are being tracked a Captury application. It also allows some amount of remote control of Captury Live. In particular, it allows starting and stopping to track and to record, starting and stopping calibration board detection, naming and renaming shots, etc..

The primary purpose of this library is to facilitate streaming of tracking data into third party applications like game engines, or 3D applications like Motion Builder or numerical analysis software like Matlab.

A thin Python wrapper around the core functionality can be built if Python is available and if the Android NDK is present in the build environment the library can also be built for the Android platform.

## Building

We use CMake for configuring the build process. There are two main targets `RemoteCaptury` and `RemoteCapturyStatic` for building a dynamically (.so / .dll) or statically linkable library (.a / .lib).
If Python3 is available the additional target `RemoteCapturyPython` can be built.
If the Android NDK can be found `RemoteCapturyAndroid` can also be built. This last target actually builds four versions of the RemoteCaptury library for the following platforms: `arm64-v8a`, `armeabi-v7a`, `x86`, and `x86_64`.

```
git clone https://github.com/thecaptury/RemoteCaptury.git
mkdir RemoteCaptury/build && cd RemoteCaptury/build
cmake ..
cmake --build . -- RemoteCaptury
```

## Using

There are primarily two ways of using the library:
1. A polling interface allows you to query the most recent pose of a given skeleton. This is fairly straight-forward but like all polling based interfaces introduces either more latency or excessive CPU usage. Use `Captury_getCurrentPose(int actorId)`.
2. You can also register a callback that gets called as soon as a new pose becomes available. This is the recommended way of using RemoteCaptury but can be a bit more complicated as the callback is called from a different thread. Register the callback with `Captury_registerNewPoseCallback(CapturyNewPoseCallback callback)` and be sure your callback finishes quickly because this runs on the socket-communication thread. If this blocks for too long packets may get dropped.

The library also provides a mechanism for synchronizing the clocks of the machine running CapturyLive and the machine running RemoteCaptury. It is recommended that both machines are either not synchronized to NTP or that the NTP client does not do "large" jumps. As this will get the clock synchronization out of step for a few seconds. Enable clock synchronization by calling `Captury_startTimeSynchronizationLoop()` once. Once the clock has locked you can call `Captury_getTime()` to get the current time of the machine running CapturyLive. This is implemented without sending a packet to the machine every time you call this function.

Check the `examples` directory for examples.