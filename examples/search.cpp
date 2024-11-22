#include <stdio.h>
#include "RemoteCaptury.h"
#ifndef WIN32
#include <unistd.h>
#endif

// This example uses the callback interface to get the current pose of all actors from CapturyLive



// This callback is called every time a new pose becomes available.
// Make sure this callback is quick. Otherwise frames may get dropped.
void newPoseCallback(CapturyActor* actor, CapturyPose* pose, int trackingQuality)
{
	Captury_log(CAPTURY_LOG_INFO, "actor %x has a new pose now\n", actor->id);
}

// this is called whenever an actor changes state (starts or stops tracking or finishes scaling or is deleted).
// Make sure this callback is quick. Otherwise frames may get dropped.
void actorChangedCallback(int actorId, int mode)
{
	Captury_log(CAPTURY_LOG_INFO, "actor %x has state %s now\n", actorId, CapturyActorStatusString[mode]);

	switch (mode) {
	case ACTOR_SCALING:
		break;
	case ACTOR_TRACKING:
		break;
	case ACTOR_STOPPED:
		break;
	case ACTOR_DELETED:
		break;
	case ACTOR_UNKNOWN:
		break;
	}

	const CapturyActor* actor = Captury_getActor(actorId);
	// do something with it

	// free it
	Captury_freeActor(actor);
}

int main(int argc, char** argv)
{
	// This is cheating a little as we're not connecting to a remote machine
	// but you can change the IP. There is also a more advanced version of Captury_connect() that gives you more options
	// to control ports or to enable asynchronous connect mode.
	int ret = Captury_connect("127.0.0.1", 2101);
	if (ret == 0)
		return -1;

	// This is optional but sometimes it's quite useful to know the exact time on the CapturyLive machine.
	// The accuracy of the synchronization depends primarily on your OS and the network topology between
	// the CapturyLive machine and this machine. On a LAN running Windows on the target machine you can
	// expect about 100us accuracy.
	Captury_startTimeSynchronizationLoop();

	// register callback that is called in a different thread whenever a new pose becomes available
	ret = Captury_registerNewPoseCallback(newPoseCallback);
	if (ret == 0)
		return -1;

	ret = Captury_registerActorChangedCallback(actorChangedCallback);
	if (ret == 0)
		return -1;

	// start streaming
	// The recommended features are CAPTURY_STREAM_POSES and CAPTURY_STREAM_COMPRESSED.
	// Additional flags should be added as required.
	Captury_startStreaming(CAPTURY_STREAM_POSES | CAPTURY_STREAM_COMPRESSED);

	while (true) {
		// get list of actors - otherwise we won't know whom to poll
		const CapturyActor* actors;
		int numActors = Captury_getActors(&actors);

		// check whether there is a person being tracked already
		bool anyAlive = false;
		for (int i = 0; i < numActors; ++i) {
			int status = Captury_getActorStatus(actors[i].id);

			switch (status) {
			case ACTOR_SCALING:
			case ACTOR_TRACKING:
				anyAlive = true;
				break;
			case ACTOR_STOPPED:
				// keep the list of actors clean
				Captury_deleteActor(actors[i].id);
				break;
			case ACTOR_DELETED:
			case ACTOR_UNKNOWN:
				break;
			}
			if (anyAlive)
				break;
		}

		// if there is no person being tracked, try to find a new one
		if (!anyAlive) {
			// try to find a new actor at the location (1m, 1m) with any orientation (500 > 360deg)
			Captury_snapActor(-1000.0f, 0.0f, 500.0f);
		}

		// potentially Captury_freeActors() here

		// sleep 1 second
		#ifdef WIN32
		Sleep(1000);
		#else
		usleep(1000*1000);
		#endif
	}

	Captury_stopStreaming();

	Captury_disconnect();

	return 0;
}
