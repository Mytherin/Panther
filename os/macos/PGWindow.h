
#import <Cocoa/Cocoa.h>

#include <vector>
#include <string>

#import "PGView.h"

@class PGNSWindow;

@interface PGNSWindow : NSWindow {
	PGView* pgview;
	ControlManager* manager;
}

- (instancetype)initWithContentRect:(NSRect)contentRect 
                          styleMask:(NSUInteger)style 
                            backing:(NSBackingStoreType)bufferingType 
                              defer:(BOOL)flag;

-(void)registerPGView:(PGView*)view;
-(void)close;
-(NSDragOperation)draggingEntered:(id<NSDraggingInfo>)sender;
-(NSDragOperation)draggingUpdated:(id<NSDraggingInfo>)sender;
-(void)draggingExited:(id<NSDraggingInfo>)sender;
-(void)draggingEnded:(id<NSDraggingInfo>)sender;
-(BOOL)performDragOperation:(id<NSDraggingInfo>)sender;
@end