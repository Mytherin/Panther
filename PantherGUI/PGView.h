
#import <Cocoa/Cocoa.h>
#include <vector>
#include <string>
#include "windowfunctions.h"
#include "textfile.h"


@class PGView;

@interface PGView : NSView <NSDraggingSource> {
	PGWindowHandle handle;
	PGTimerHandle timer;
}
-(void)performClose;
-(PGWindowHandle)getHandle;
- (instancetype)initWithFrame:(NSRect)frameRect :(NSWindow*)window :(std::vector<TextFile*>)textfiles;
-(NSRect)getBounds;
- (void)targetMethod:(NSTimer*)timer;
- (PGTimerHandle)scheduleTimer:(PGWindowHandle)handle :(int)ms :(PGTimerCallback)callback :(PGTimerFlags)flags;


-(NSDragOperation)draggingSession:(NSDraggingSession *)session sourceOperationMaskForDraggingContext:(NSDraggingContext)context;
-(void)draggingSession:(NSDraggingSession *)session willBeginAtPoint:(NSPoint) screenPoint;
-(void)draggingSession:(NSDraggingSession *)session endedAtPoint:(NSPoint)screenPoint operation:(NSDragOperation)operation;
-(void)draggingSession:(NSDraggingSession *)session movedToPoint:(NSPoint)screenPoint;
@end
