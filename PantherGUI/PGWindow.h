
#import <Cocoa/Cocoa.h>
#include <vector>
#include <string>


@class PGNSWindow;

@interface PGNSWindow : NSWindow

- (instancetype)initWithContentRect:(NSRect)contentRect 
                          styleMask:(NSUInteger)style 
                            backing:(NSBackingStoreType)bufferingType 
                              defer:(BOOL)flag;

-(void)close;
-(NSDragOperation)draggingEntered:(id<NSDraggingInfo>)sender;
-(NSDragOperation)draggingUpdated:(id<NSDraggingInfo>)sender;
-(BOOL)performDragOperation:(id<NSDraggingInfo>)sender;
@end