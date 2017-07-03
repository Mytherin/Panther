
#import <Cocoa/Cocoa.h>
#import "PGView.h"
#import "AppDelegate.h"

#include <unistd.h>

// Compilation: clang++ -framework Cocoa -fobjc-arc -lobjc test.mm AppDelegate.mm -o test

// clang++ -framework Cocoa -fobjc-arc -lobjc -I/Users/myth/Sources/skia/skia/include/core -I/Users/myth/Sources/skia/skia/include/config -L/Users/myth/Sources/skia/skia/out/Static -lskia -std=c++11 main.mm AppDelegate.mm PGView.mm -o main

int main(int argc, const char *argv[]) {
    NSArray* tl;

    // handle command line arguments
    PGCommandLineSettings settings = PGHandleCommandLineArguments(argc, argv);
    if (settings.exit_code >= 0) {
        return settings.exit_code;
    }
    if (!settings.wait) {
        daemon(false, true);   
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


