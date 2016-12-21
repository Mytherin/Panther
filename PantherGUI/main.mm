#import <Cocoa/Cocoa.h>
#import "PGView.h"

// Compilation: clang++ -framework Cocoa -fobjc-arc -lobjc test.mm AppDelegate.mm -o test

// clang++ -framework Cocoa -fobjc-arc -lobjc -I/Users/myth/Sources/skia/skia/include/core -I/Users/myth/Sources/skia/skia/include/config -L/Users/myth/Sources/skia/skia/out/Static -lskia -std=c++11 main.mm AppDelegate.mm PGView.mm -o main

int main(int argc, const char *argv[])
{
    NSWindow *window;
    NSApplication *application = [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    //[[NSBundle mainBundle] loadNibNamed:@"MainMenu" owner:application topLevelObjects:&tl];

    NSRect contentSize = NSMakeRect(0, 0, 1000.0, 700.0);
    NSUInteger windowStyleMask = NSTitledWindowMask | NSResizableWindowMask | NSClosableWindowMask | NSMiniaturizableWindowMask;
    window = [[NSWindow alloc] initWithContentRect:contentSize styleMask:windowStyleMask backing:NSBackingStoreBuffered defer:YES];
    window.backgroundColor = [NSColor whiteColor];
    NSView *view = [[PGView alloc] initWithFrame:contentSize:window];
    [view setWantsLayer:YES];
    window.contentView = view;
    [window makeFirstResponder:view];
/*
    AppDelegate *applicationDelegate = [[AppDelegate alloc] init];      // Instantiate App  delegate
    [application setDelegate:applicationDelegate];                      // Assign delegate to the NSApplication
  */  
    [window cascadeTopLeftFromPoint:NSMakePoint(20,20)];
    [window setTitle:@"PantherGUI"];
    [window makeKeyAndOrderFront:nil];
    [NSApp activateIgnoringOtherApps:YES];
    [NSApp run];                                                  // Call the Apps Run method

    return 0;       // App Never gets here.*/
}


