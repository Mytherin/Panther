#import <Cocoa/Cocoa.h>
#import "AppDelegate.h"

// Compilation: clang++ -framework Cocoa -fobjc-arc -lobjc test.mm AppDelegate.mm -o test

// clang++ -framework Cocoa -fobjc-arc -lobjc -I/Users/myth/Sources/skia/skia/include/core -I/Users/myth/Sources/skia/skia/include/config -L/Users/myth/Sources/skia/skia/out/Static -lskia -std=c++11 main.mm AppDelegate.mm PGView.mm -o main

int main(int argc, const char *argv[])
{
    NSArray *tl;
    NSApplication *application = [NSApplication sharedApplication];
    [[NSBundle mainBundle] loadNibNamed:@"MainMenu" owner:application topLevelObjects:&tl];

    AppDelegate *applicationDelegate = [[AppDelegate alloc] init];      // Instantiate App  delegate
    [application setDelegate:applicationDelegate];                      // Assign delegate to the NSApplication
    [application run];                                                  // Call the Apps Run method

    return 0;       // App Never gets here.
}
