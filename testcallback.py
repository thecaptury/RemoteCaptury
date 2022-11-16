# add location of remotecaptury.pyd or remotecaptury.so
import os
path = os.path.abspath("../../devmode/plugins/RemoteCaptury/Release")
print(f"adding {path} to sys.path")
import sys
sys.path.append(path)


import remotecaptury as rc
import time
import numpy as np
from PIL import Image
import shutil
if os.path.exists("./logs"):
	shutil.rmtree("./logs")
os.mkdir("./logs")

log_path = os.path.abspath("./logs/testcallback.log")
log_path2 = os.path.abspath("./logs/testcallback2.log")
count = 0


def callback(*args):
	global count
	count += 1
	image = args[0]
	# image = image.reshape(height, width, 3)
	print(f"min max of image : {np.min(image)} {np.max(image)}, dtype : {image.dtype}")
	if isinstance(image, np.ndarray):
		print(image.shape)
		# convert to PIL image
		image = Image.fromarray(image)
		image.save(f"./logs/testcallback_{count}.png")

# start streaming images.
print(f"trying to connect")
host = "127.0.0.1"
connected = rc.connect(host = host)
print(f"is_connected = {connected}")
if not connected:
	exit(1)

callback_registered = rc.setNewImageCallback(callback)
print(f"is_callback_registered = {callback_registered}")

streaming_start = rc.startStreamingImages(cameraNumber=0)
print(f"is_streaming = {streaming_start}")
if not streaming_start:
	exit(1)

time.sleep(3)

streaming_stop = rc.stopStreaming()