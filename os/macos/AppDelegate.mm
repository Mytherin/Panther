//
//  AppDelegate.m
//  TestUI
//
//  Created by Mark Raasveldt on 13/12/16.
//  Copyright © 2016 Mytherin. All rights reserved.
//

#import "AppDelegate.h"
#import "PGView.h"

@interface AppDelegate ()

@property IBOutlet NSWindow *window;
@end

@implementation AppDelegate

-(id)init
{
    if(self = [super init]) {
        NSRect contentSize = NSMakeRect(500.0, 500.0, 1000.0, 1000.0);
        NSUInteger windowStyleMask = NSTitledWindowMask | NSResizableWindowMask | NSClosableWindowMask | NSMiniaturizableWindowMask;
        self.window = [[NSWindow alloc] initWithContentRect:contentSize styleMask:windowStyleMask backing:NSBackingStoreBuffered defer:YES];
        self.window.backgroundColor = [NSColor whiteColor];
        self.window.title = @"PantherGUI";
        NSView *view = [[PGView alloc] initWithFrame:contentSize];
        [view setWantsLayer:YES];
        self.window.contentView = view;
        [self.window makeFirstResponder:view];
    }
    return self;
 }

-(void)applicationWillFinishLaunching:(NSNotification *)notification
{
}

-(void)applicationDidFinishLaunching:(NSNotification *)notification
{
    [self.window makeKeyAndOrderFront:self];     // Show the window
} 

@end
