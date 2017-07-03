
#include "PGWindow.h"
#include "PGView.h"

@implementation PGNSWindow : NSWindow

-(void)close{
	[pgview performClose];
	[super close];
}

- (instancetype)initWithContentRect:(NSRect)contentRect 
                          styleMask:(NSUInteger)style 
                            backing:(NSBackingStoreType)bufferingType 
                              defer:(BOOL)flag {
	if (self = [super initWithContentRect:contentRect styleMask:style backing:bufferingType defer:flag]) {
        NSArray *types = @[NSFilenamesPboardType, @"com.panther.draggingobject"];
    	[self registerForDraggedTypes:types];
	}
	return self;
}

-(void)registerPGView:(PGView*)view {
    pgview = view;
    manager = [pgview getManager];
}

- (NSDragOperation)draggingEntered:(id<NSDraggingInfo>)sender {
    return NSDragOperationEvery;
}

- (NSDragOperation)draggingUpdated:(id<NSDraggingInfo>)sender {
    NSPasteboard *pboard = [sender draggingPasteboard];
    NSData *data = [pboard dataForType:@"com.panther.draggingobject"];
    if (data) {
        NSPoint point = [sender draggingLocation];
        const void* nsdata_data = [data bytes];
        void* data = *((void**)nsdata_data);
        manager->DragDrop(PGDragDropTabs, point.x, [pgview getBounds].size.height - point.y, data);
    }
    return NSDragOperationEvery;
}

-(BOOL)performDragOperation:(id<NSDraggingInfo>)sender {
    NSPasteboard *pboard = [sender draggingPasteboard];
    NSArray *filenames = [pboard propertyListForType:NSFilenamesPboardType];

    if (filenames.count > 0) {
    	PGWindowHandle handle = [(PGView*)[self firstResponder] getHandle];
    	for(id object in filenames) {
			DropFile(handle, std::string([object UTF8String]));
    	}
    	return YES;
    }
    NSData *data = [pboard dataForType:@"com.panther.draggingobject"];
    if (data) {
        NSPoint point = [sender draggingLocation];
        const void* nsdata_data = [data bytes];
        void* data = *((void**)nsdata_data);
        manager->PerformDragDrop(PGDragDropTabs, point.x, [pgview getBounds].size.height - point.y, data);
        return YES;
    }

    return NO;
}


-(void)draggingExited:(id<NSDraggingInfo>)sender {
    NSPasteboard *pboard = [sender draggingPasteboard];
    NSData *data = [pboard dataForType:@"com.panther.draggingobject"];
    if (data) {
        manager->ClearDragDrop(PGDragDropTabs);
        return;
    }
}

-(void)draggingEnded:(id<NSDraggingInfo>)sender {
    NSPasteboard *pboard = [sender draggingPasteboard];
    NSData *data = [pboard dataForType:@"com.panther.draggingobject"];
    if (data) {
        manager->ClearDragDrop(PGDragDropTabs);
        return;
    }
}

-(BOOL)canBecomeKeyWindow {
    return YES;
}

-(BOOL)canBecomeMainWindow {
    return YES;
}


@end


