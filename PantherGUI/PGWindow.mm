
#include "PGWindow.h"
#include "PGView.h"

@implementation PGNSWindow : NSWindow

-(void)close{
	[((PGView*)[self firstResponder]) performClose];
	[super close];
}

- (instancetype)initWithContentRect:(NSRect)contentRect 
                          styleMask:(NSUInteger)style 
                            backing:(NSBackingStoreType)bufferingType 
                              defer:(BOOL)flag {
	if (self = [super initWithContentRect:contentRect styleMask:style backing:bufferingType defer:flag]) {
    	[self registerForDraggedTypes:[NSArray arrayWithObject:NSFilenamesPboardType]];
	}
	return self;
}

- (NSDragOperation)draggingEntered:(id<NSDraggingInfo>)sender {
    return NSDragOperationCopy;
}

- (NSDragOperation)draggingUpdated:(id<NSDraggingInfo>)sender {
    return NSDragOperationCopy;
}

- (BOOL)performDragOperation:(id<NSDraggingInfo>)sender {
    NSPasteboard *pboard = [sender draggingPasteboard];
    NSArray *filenames = [pboard propertyListForType:NSFilenamesPboardType];

    if (filenames.count > 0) {
    	PGWindowHandle handle = [(PGView*)[self firstResponder] getHandle];
    	for(id object in filenames) {
			DropFile(handle, std::string([object UTF8String]));
    	}
    	return YES;
    }

    return NO;
}

@end
