#import <Foundation/Foundation.h>
#import "RemoteCapturyObjC.h"

int main(int argc, const char *argv[]) {
	@autoreleasepool {
		NSLog(@"Initializing CapturyRemote Objective-C wrapper test...");
		CapturyRemote *remote = [[CapturyRemote alloc] init];

		[remote registerLogBlock:^(int logLevel, NSString *message) {
			NSLog(@"[Captury Log %d]: %@", logLevel, message);
		}];

		NSLog(@"Connection status before connect: %d", [remote connectionStatus]);

		BOOL connected = [remote connectToHost:@"127.0.0.1" port:2101];
		NSLog(@"Connection attempt result: %@", connected ? @"YES" : @"NO");

		NSArray<CapturyActorObjC *> *actors = [remote fetchActors];
		NSLog(@"Fetched %lu actors.", (unsigned long)[actors count]);

        [remote startStreaming: CapturyStreamFlagsPoses];
        sleep(5);

		[remote disconnect];
		NSLog(@"Disconnected successfully.");
	}
	return 0;
}
