
#import <Cocoa/Cocoa.h>
#include <vector>
#include <string>
#include "windowfunctions.h"

@class PGView;

@interface PGView : NSView
- (instancetype)initWithFrame:(NSRect)frameRect :(NSWindow*)window;
-(NSRect)getBounds;
- (void)targetMethod:(NSTimer*)timer;
- (void)scheduleTimer:(int)ms :(PGTimerCallback)callback :(PGTimerFlags)flags;
@end