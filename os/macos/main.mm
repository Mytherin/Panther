
#import <Cocoa/Cocoa.h>
#import "PGView.h"
#import "AppDelegate.h"

#include <unistd.h>

int main(int argc, const char *argv[]) {
    NSArray* tl;

    // handle command line arguments
    PGCommandLineSettings settings = PGHandleCommandLineArguments(argc, argv);
    if (settings.exit_code >= 0) {
        return settings.exit_code;
    }
    if (!settings.wait) {
        int pid = fork();
        if (pid < 0) {
            // failed to fork
            return 1;
        } else if (pid != 0) {
            // parent process: succeeded in forking
            return 0;
        }
     }
    NSArray *apps = [NSRunningApplication runningApplicationsWithBundleIdentifier:@"com.panther.text"];
    if ([apps count] > 0) {
        // app is already running
        // bring it to the top
        NSRunningApplication* app = [apps objectAtIndex:0];
        for(size_t i = 0; i < settings.files.size(); i++) {
            std::string path = PGPathJoin(PGCurrentDirectory(), settings.files[i]);
            [[NSDistributedNotificationCenter defaultCenter] 
                postNotificationName:@"pantherOpenFile" 
                object:[NSString stringWithUTF8String:path.c_str()]
                userInfo:nil
                deliverImmediately:YES];
        }
        [app activateWithOptions:NSApplicationActivateIgnoringOtherApps];
        return 0;
    }

    @autoreleasepool {
        NSApplication *application = [NSApplication sharedApplication];
        AppDelegate *appDelegate = [[AppDelegate alloc] init];
        [application setActivationPolicy:NSApplicationActivationPolicyRegular];
        [application setDelegate:appDelegate];
        [application run];
    }
    return 0;       // App Never gets here.*/
}


