# add location of remotecaptury.pyd or remotecaptury.so
import os
path = os.path.abspath("../../devmode/plugins/RemoteCaptury/Release")
print(f"adding {path} to sys.path")
import sys
sys.path.append(path)


import remotecaptury as rc
import time

log_path = os.path.abspath("./testcallback.log")
log_path2 = os.path.abspath("./testcallback2.log")
count = 0

def callback():
	global count
	count += 1
	with open(log_path, "a") as f:
		f.write(f"callback {count}\n")

def callback2():
	global count
	count += 1
	with open(log_path2, "a") as f:
		f.write(f"callback2 {count}\n")

# start streaming images.
print(f"trying to connect")
host = "127.0.0.1"
connected = rc.connect(host = host)
print(f"is_connected = {connected}")

callback_registered = rc.setNewImageCallback(callback)
print(f"is_callback_registered = {callback_registered}")

streaming_start = rc.startStreamingImages(cameraNumber=0)
print(f"is_streaming = {streaming_start}")

time.sleep(3)

callback_registered = rc.setNewImageCallback(callback2)
print(f"is_callback2_registered = {callback_registered}")
time.sleep(3)

streaming_stop = rc.stopStreaming()