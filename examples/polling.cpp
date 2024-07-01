#include <stdio.h>
#include "RemoteCaptury.h"

// this example uses the polling interface to get the current pose of all actors from CapturyLive
int main(int argc, char** argv)
{
	// this is cheating a little as we're not connecting to a remote machine
	// but you can change the IP. It should work just as well.
	int ret = Captury_connect("127.0.0.1", 2101);
	if (ret == 0)
		return -1;

	// this is optional but sometimes it's quite useful to know the exact time on the CapturyLive machine.
	// The accuracy of the synchronization depends primarily on your OS and the network topology between
	// the CapturyLive machine and this machine. On a LAN running Windows on the target machine you can
	// expect about 100us accuracy.
	Captury_startTimeSynchronizationLoop();

	// start streaming
	// The recommended features are CAPTURY_STREAM_POSES and CAPTURY_STREAM_COMPRESSED.
	// Additional flags should be added as required.
	Captury_startStreaming(CAPTURY_STREAM_POSES | CAPTURY_STREAM_COMPRESSED);

	uint64_t lastTimestamp = 0;
	while (true) {
		// get list of actors - otherwise we won't know whom to poll
		const CapturyActor* actors;
		int numActors = Captury_getActors(&actors);

		for (int i = 0; i < numActors; ++i) {
			CapturyPose* pose = Captury_getCurrentPose(actors[i].id);
			if (pose == NULL)
				continue;

			if (pose->timestamp != lastTimestamp) {
				Captury_log(CAPTURY_LOG_INFO, "actor %x has new pose at %zd\n", pose->actor, pose->timestamp);
				lastTimestamp = pose->timestamp;
			}

			// make sure to free the pose using the provided function - potential binary incompatibilities between different Microsoft compilers
			Captury_freePose(pose);
		}
	}

	// this is never called - I know.
	// Your code will obviously do this right and always clean everything up.
	Captury_stopStreaming();

	Captury_disconnect();

	return 0;
}
