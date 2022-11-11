# add location of remotecaptury.pyd or remotecaptury.so
import os
path = os.path.abspath("../../devmode/plugins/RemoteCaptury/Release")
print(f"adding {path} to sys.path")
import sys
sys.path.append(path)


import remotecaptury as rc
import time

print(f"start-")
def callback(args):
	print("callback - python")
	print(f"type of image = {type(args)}")
	print(args)

# start streaming images.
print(f"trying to connect")
connected = rc.connect(host = '127.0.0.1')
print(f"is_connected = {connected}")

streaming_start = rc.startStreamingImages(0, 0)
print(f"is_streaming = {streaming_start}")

if (streaming_start):
	# set new callback function.
	callback_registered = rc.setNewImageCallback(callback)
	print(f"is_callback_registered = {callback_registered}")

	# wait for 10 seconds.
	print(f"waiting for 10 seconds to receive images")
	time.sleep(10)