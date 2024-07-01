#include <stdio.h>
#include "RemoteCaptury.h"
#ifndef WIN32
#include <unistd.h>
#endif

// This example demonstrates how to stream biomechanical angle data rather than poses from CapturyLive



// This callback is called every time a new pose becomes available.
// Make sure this callback is quick. Otherwise frames may get dropped.
void newAnglesCallback(const CapturyActor* actor, int numAngles, struct CapturyAngleData* values)
{
	for (int i = 0; i < numAngles; ++i)
		Captury_log(CAPTURY_LOG_INFO, "actor %x received new angle %d: %g\n", actor->id, values[i].type, values[i].value);
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
	ret = Captury_registerNewAnglesCallback(newAnglesCallback);
	if (ret == 0)
		return -1;

	// start streaming
	// The recommended features are CAPTURY_STREAM_POSES and CAPTURY_STREAM_COMPRESSED.
	// Here we stream angle data rather than pose data. This can be easier to process, e.g. in biofeedback applications.
	// Additional flags should be added as required.
	uint16_t angles[] = {CAPTURY_LEFT_KNEE_FLEXION_EXTENSION, CAPTURY_RIGHT_KNEE_FLEXION_EXTENSION};
	Captury_startStreamingImagesAndAngles(CAPTURY_STREAM_ANGLES, /*camera*/-1, 2, angles);

	#ifdef WIN32
	Sleep(100000);
	#else
	usleep(100000*1000);
	#endif

	// instead of the callback approach you can also use polling like this:
	// int numAngles;
	// CapturyAngleData* angles = Captury_getCurrentAngles(actorId, &numAngles);

	Captury_stopStreaming();

	Captury_disconnect();

	return 0;
}
