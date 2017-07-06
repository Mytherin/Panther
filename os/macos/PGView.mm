
#include "textfile.h"
#include "control.h"
#include "textfield.h"
#include "time.h"
#include "controlmanager.h"
#include "filemanager.h"
#include "container.h"
#include "statusbar.h"
#include "workspace.h"
#include "toolbar.h"

#include <sys/time.h>
#include <cctype>

#include "globalsettings.h"

#include "control.h"

#import "PGView.h"
#import "PGWindow.h"
#import "PGDrawBitmap.h"

#include "rust/gitignore.h"

#define RETINA_API_AVAILABLE (defined(MAC_OS_X_VERSION_10_7) && \
							  MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_7)
// clang++ -framework Cocoa -fobjc-arc -lobjc -I/Users/myth/Sources/skia/skia/include/core -I/Users/myth/Sources/skia/skia/include/config -L/Users/myth/Sources/skia/skia/out/Static -lskia -std=c++11 main.mm AppDelegate.mm PGView.mm c.cpp control.cpp cursor.cpp keywords.cpp logger.cpp scheduler.cpp text.cpp syntaxhighlighter.cpp textfield.cpp textfile.cpp textline.cpp thread.cpp windowfunctions.cpp xml.cpp file.cpp tabcontrol.cpp tabbedtextfield.cpp renderer.cpp filemanager.cpp controlmanager.cpp utils.cpp  -o main

struct PGPopupMenu {
	PGWindowHandle window;
	std::map<int, PGControlCallback> callbacks;
	std::vector<PGPopupInformation> info;
	Control* control;
	NSMenu* menu;
};

struct PGWindow {
	NSWindow *window;
	NSEvent *event;
	PGView* view;
	bool pending_destroy = false;
	std::shared_ptr<ControlManager> manager;
	PGRendererHandle renderer;
	PGWorkspace* workspace;

	PGWindow(PGWorkspace* workspace) : workspace(workspace), event(nullptr) { }

	void SetEvent(NSEvent* event) {
		if (this->event) {
			[this->event release];
		}
		this->event = event;
		[event retain];
	}
};

struct PGTooltip {
	PGWindowHandle window;
	NSToolTipTag tooltip;
	PGRect region;
	NSString *text = nullptr;

	PGTooltip() : text(nullptr) { }
	~PGTooltip() {
		if (this->text) {
			[this->text release];
		}
	}
	void SetText(NSString* text) {
		if (this->text) {
			[this->text release];
		}
		this->text = text;
		[text retain];
	}
};


struct PGMouseFlags {
	int x;
	int y;
	PGMouseButton button;
	PGModifier modifiers;
};

@interface PGTimerObject : NSObject {
	PGTimerCallback callback;
	PGWindowHandle handle;
}
-(id)initWithCallback:(PGTimerCallback)callback :(PGWindowHandle)whandle;
-(void)callFunction;
@end

@implementation PGTimerObject
-(id)initWithCallback:(PGTimerCallback)cb :(PGWindowHandle)whandle{
	self = [super init];
	if (self) {
		callback = cb;
		handle = whandle;
	}
	return self;
}
-(void)callFunction {
	callback(handle);
}
@end

struct PGTimer {
	PGTimerObject* user_data;
	NSTimer* timer;
};

#define MAX_REFRESH_FREQUENCY 1000/30

NSDraggingSession* active_session = nullptr;

void PeriodicWindowRedraw(PGWindowHandle handle) {
	handle->manager->Update();
	if (handle->pending_destroy) {
		handle->workspace->RemoveWindow(handle);
		[handle->window performClose:handle->window];
		std::string msg = handle->workspace->WriteWorkspace();
	}
}

@implementation PGView : NSView

- (instancetype)initWithFrame:(NSRect)frameRect :(NSWindow*)window :(PGWorkspace*)workspace :(std::vector<std::shared_ptr<TextView>>)textfiles
{
	if(self = [super initWithFrame:frameRect]) {
#ifdef RETINA_API_AVAILABLE
		[self setWantsBestResolutionOpenGLSurface:YES];
#endif
		NSRect rect = [self getBounds];

		handle = new PGWindow(workspace);
		if (!handle) {
			return self;
		}

		workspace->AddWindow(handle);
		handle->window = window;
		handle->workspace = workspace;
		handle->view = self;
		handle->renderer = InitializeRenderer();

		timer = CreateTimer(handle, MAX_REFRESH_FREQUENCY, PeriodicWindowRedraw, PGTimerFlagsNone);

		PGCreateControlManager(handle, textfiles);
	}
	return self;
}

- (float)scaleFactor {
	NSWindow *window = [self window];
#ifdef RETINA_API_AVAILABLE
	if (window) {
		return [window backingScaleFactor];
	}
	return [[NSScreen mainScreen] backingScaleFactor];
#else
	if (window) {
		return [window userSpaceScaleFactor];
	}
	return [[NSScreen mainScreen] userSpaceScaleFactor];
#endif
}

- (void)drawRect:(NSRect)invalidateRect {
	if (!handle->manager) return;
	NSRect window_size = [self getBounds];
	if (handle->manager->width != window_size.size.width || 
		handle->manager->height != window_size.size.height) {
		handle->manager->SetSize(PGSize(window_size.size.width, window_size.size.height));
	}

	PGIRect rect(
		invalidateRect.origin.x,
		invalidateRect.origin.y,
		invalidateRect.size.width,
		invalidateRect.size.height);

	RenderControlsToBitmap(handle->renderer, bitmap, rect, handle->manager.get(), [self scaleFactor]);

	// draw bitmap
	CGContextRef context = (CGContextRef) [[NSGraphicsContext currentContext] graphicsPort];
	SkCGDrawBitmap(context, bitmap, rect, [self scaleFactor]);
}

-(void)performClose {
	handle->workspace->WriteWorkspace();
	DeleteTimer(timer);
	DeleteRenderer(handle->renderer);
	handle->renderer = nullptr;
	handle->manager = nullptr;
}

-(NSRect)getBounds {
	return self.bounds;
}

-(PGModifier)getModifierFlags:(NSEvent *)event {
	PGModifier modifiers = PGModifierNone;
	NSEventModifierFlags modifierFlags = [event modifierFlags];
	if (modifierFlags & NSShiftKeyMask) {
		modifiers |= PGModifierShift;
	}
	if (modifierFlags & NSControlKeyMask) {
		modifiers |= PGModifierCtrl;
	}
	if (modifierFlags & NSCommandKeyMask) {
		modifiers |= PGModifierCmd;
	}
	if (modifierFlags & NSAlternateKeyMask) {
		modifiers |= PGModifierAlt;
	}
	return modifiers;
}

-(PGMouseFlags)getMouseFlags:(NSEvent *)event {
	PGMouseFlags flags;
	flags.x = (int) [event locationInWindow].x;
	flags.y = self.bounds.size.height - (int) [event locationInWindow].y;
	PGMouseButton button = PGMouseButtonNone;
	PGModifier modifiers = [self getModifierFlags:event];
	NSUInteger pressedButtonMask = [NSEvent pressedMouseButtons];
	if ((pressedButtonMask & (1 << 0)) != 0) {
		button |= PGLeftMouseButton;
	} else if ((pressedButtonMask & (1 << 1)) != 0) {
		button |= PGRightMouseButton;
	} else if ((pressedButtonMask & (1 << 2)) != 0) {
		button |= PGMiddleMouseButton;
	}
	flags.button = button;
	flags.modifiers = modifiers;
	return flags;
}

- (BOOL)acceptsFirstMouse:(NSEvent *)event {
	return YES;
}

- (void)mouseDown:(NSEvent *)event {
	handle->SetEvent(event);
	PGMouseFlags flags = [self getMouseFlags:event];
	handle->manager->MouseDown(flags.x, flags.y, PGLeftMouseButton, flags.modifiers, 0);
}

- (void)mouseUp:(NSEvent *)event {
	handle->SetEvent(event);
	PGMouseFlags flags = [self getMouseFlags:event];
	handle->manager->MouseUp(flags.x, flags.y, PGLeftMouseButton, flags.modifiers);
}

- (void)rightMouseDown:(NSEvent *)event {
	handle->SetEvent(event);
	PGMouseFlags flags = [self getMouseFlags:event];
	handle->manager->MouseDown(flags.x, flags.y, PGRightMouseButton, flags.modifiers, 0);
}

- (void)rightMouseUp:(NSEvent *)event {
	handle->SetEvent(event);
	PGMouseFlags flags = [self getMouseFlags:event];
	handle->manager->MouseUp(flags.x, flags.y, PGRightMouseButton, flags.modifiers);
}

- (void)otherMouseDown:(NSEvent *)event {
	handle->SetEvent(event);
	// FIXME: not just middle mouse button
	PGMouseFlags flags = [self getMouseFlags:event];
	handle->manager->MouseDown(flags.x, flags.y, PGMiddleMouseButton, flags.modifiers, 0);
}

- (void)otherMouseUp:(NSEvent *)event {
	handle->SetEvent(event);
	// FIXME: not just middle mouse button
	PGMouseFlags flags = [self getMouseFlags:event];
	handle->manager->MouseUp(flags.x, flags.y, PGMiddleMouseButton, flags.modifiers);
}

- (void)scrollWheel:(NSEvent *)event {
	if ([event scrollingDeltaX] == 0.0 && [event scrollingDeltaY] == 0.0) return;

	double yscroll = [event scrollingDeltaY];
	if (![event hasPreciseScrollingDeltas]) {
		yscroll *= 20;
	}
	handle->SetEvent(event);
	PGMouseFlags flags = [self getMouseFlags:event];
	handle->manager->MouseWheel(flags.x, flags.y, [event scrollingDeltaX] * 10, yscroll, flags.modifiers);
}
/*
- (void)mouseDragged:(NSEvent *)event {
	PGMouseFlags flags = [self getMouseFlags:event];
	handle->manager->MouseMove(flags.x, flags.y,  flags.button);
}*/

- (void)keyDown:(NSEvent *)event {
	PGButton button = PGButtonNone;
	PGModifier modifiers = [self getModifierFlags:event];
	NSString *string = [event charactersIgnoringModifiers];
	unichar keyChar = 0;
	if ( [string length] == 0 )
		return; // reject dead keys
	if ( [string length] == 1 ) {
		// see: https://developer.apple.com/reference/appkit/nsevent/1535851-function_key_unicodes
		keyChar = [string characterAtIndex:0];
		if (keyChar == 27) {
			button = PGButtonEscape;
		} else if (keyChar == 127) {
			button = PGButtonBackspace;
		} else if (keyChar == '\r') {
			button = PGButtonEnter;
		} else if (keyChar == '\t' || keyChar == 25) {
			button = PGButtonTab;
		} else if (keyChar == NSLeftArrowFunctionKey) {
			button = PGButtonLeft;
		} else if (keyChar == NSRightArrowFunctionKey) {
			button = PGButtonRight;
		} else if (keyChar == NSUpArrowFunctionKey) {
			button = PGButtonUp;
		} else if (keyChar == NSDownArrowFunctionKey) {
			button = PGButtonDown;
		} else if (keyChar == NSInsertFunctionKey) {
			assert(0);
		} else if (keyChar == NSDeleteFunctionKey) {
			button = PGButtonDelete;
		} else if (keyChar == NSHomeFunctionKey) {
			button = PGButtonHome;
		} else if (keyChar == NSEndFunctionKey) {
			button = PGButtonEnd;	
		} else if (keyChar == NSPageUpFunctionKey) {
			button = PGButtonPageUp;
		} else if (keyChar == NSPageDownFunctionKey) {
			button = PGButtonPageDown;
		} else if (keyChar == NSF1FunctionKey) {
			button = PGButtonF1;
		} else if (keyChar == NSF2FunctionKey) {
			button = PGButtonF2;
		} else if (keyChar == NSF3FunctionKey) {
			button = PGButtonF3;
		} else if (keyChar == NSF4FunctionKey) {
			button = PGButtonF4;
		} else if (keyChar == NSF5FunctionKey) {
			button = PGButtonF5;
		} else if (keyChar == NSF6FunctionKey) {
			button = PGButtonF6;
		} else if (keyChar == NSF7FunctionKey) {
			button = PGButtonF7;
		} else if (keyChar == NSF8FunctionKey) {
			button = PGButtonF8;
		} else if (keyChar == NSF9FunctionKey) {
			button = PGButtonF9;
		} else if (keyChar == NSF10FunctionKey) {
			button = PGButtonF10;
		} else if (keyChar == NSF11FunctionKey) {
			button = PGButtonF11;
		} else if (keyChar == NSF12FunctionKey) {
			button = PGButtonF12;
		} else if (keyChar == NSPrintScreenFunctionKey) {
			assert(0);
		} else if (keyChar == NSScrollLockFunctionKey) {
			assert(0);
		} else if (keyChar == NSPauseFunctionKey) {
			assert(0);
		} else if (keyChar == NSUndoFunctionKey) {
			assert(0);
		} else if (keyChar == NSRedoFunctionKey) {
			assert(0);
		} else if (keyChar == NSFindFunctionKey) {
			assert(0);
		}

		if (button != PGButtonNone) {
			handle->manager->KeyboardButton(button, modifiers);
		} else if (keyChar >= 0x20 && keyChar <= 0x7E) {
			if (modifiers == PGModifierShift)
				modifiers = PGModifierNone;
			if (modifiers != PGModifierNone)
				keyChar = toupper(keyChar);
			handle->manager->KeyboardCharacter(keyChar, modifiers);
		}
	}
	/*
	int key = [event keyCode];
	NSLog(@"Characters: %@", [event characters]);
	NSLog(@"KeyCode: %hu", [event keyCode]);
	
	if (key == kUpArrowCharCode) {
		button = PGButtonUp;
	} else if (key == kDownArrowCharCode) {
		button = PGButtonDown;
	} else if (key == NSKeyCodeLeftArrow) {
		button = PGButtonLeft;
	} else if (key == NSKeyCodeRightArrow) {
		button = PGButtonRight;
	}*/
}

-(PGWindowHandle)getHandle {
	return handle;
}

-(ControlManager*)getManager {
	return handle->manager.get();
}

- (void)keyUp:(NSEvent *)event {

}

- (void)targetMethod:(NSTimer*)theTimer {
	[[theTimer userInfo] callFunction];
}

- (PGTimerHandle)scheduleTimer:(PGWindowHandle)window_handle :(int)ms :(PGTimerCallback)callback :(PGTimerFlags)flags {
	PGTimerHandle timer_handle = new PGTimer();
	PGTimerObject* object = [[PGTimerObject alloc] initWithCallback:callback:window_handle];
	timer_handle->user_data = object;
	timer_handle->timer = [NSTimer scheduledTimerWithTimeInterval:ms / 1000.0
		target:self
		selector:@selector(targetMethod:)
		userInfo:object
		repeats:flags & PGTimerExecuteOnce ? NO : YES];
	return timer_handle;
}

-(void)controlCallback:(NSValue*)_callback :(NSValue*)_control {
	PGControlCallback callback = (PGControlCallback) [_callback pointerValue];
	Control* control = (Control*) [_control pointerValue];
	callback(control);
}

-(void)popupMenuPress:(NSMenuItem*)sender {
	NSArray* array = (NSArray*)[sender representedObject];
	PGPopupCallback callback = (PGPopupCallback)[(NSValue*)array[0] pointerValue];
	PGPopupMenuHandle popup = (PGPopupMenuHandle)[(NSValue*)array[1] pointerValue];
	Control* control = popup->control;
	lng entry = [(NSNumber*)array[2] longLongValue];
	PGPopupInformation info = popup->info[entry];
	callback(control, &info);
}


- (NSDraggingSession *)startDragging:(NSArray<NSDraggingItem *> *)items 
		event:(NSEvent *)event 
		source:(id<NSDraggingSource>)source
		callback:(PGDropCallback)callback
		data:(void*)data {
	self->dragging_callback = callback;
	self->dragging_data = data;
	NSDraggingSession* session = [handle->view beginDraggingSessionWithItems:items event:event source:source];
	session.animatesToStartingPositionsOnCancelOrFail = NO;
	active_session = session;
	return session;
}

-(NSDragOperation)draggingSession:(NSDraggingSession *)session sourceOperationMaskForDraggingContext:(NSDraggingContext)context {
	return NSDragOperationEvery;
}


-(void)draggingSession:(NSDraggingSession *)session willBeginAtPoint:(NSPoint) screenPoint {
}

-(void)draggingSession:(NSDraggingSession *)session endedAtPoint:(NSPoint)screenPoint operation:(NSDragOperation)operation {
	active_session = nullptr;
	if (dragging_callback) {
		dragging_callback(PGPoint(screenPoint.x, [self getBounds].size.height - screenPoint.y), dragging_data);
		dragging_callback = nullptr;
	}
}

-(void)draggingSession:(NSDraggingSession *)session movedToPoint:(NSPoint)screenPoint {
}

- (void)windowDidResignKey:(NSNotification *)notification {
	if (handle->manager) {
		handle->manager->LosesFocus();	
	}
}

- (void)windowDidBecomeKey:(NSNotification *)notification {
	if (handle->manager) {
		handle->manager->GainsFocus();	
	}
}

@end

PGPoint GetMousePosition(PGWindowHandle window) {
	//get current mouse position
	NSPoint mouseLoc = [NSEvent mouseLocation];
	// convert screen coordinates to local coordinates of the window
	NSRect mouseScreen = [window->window convertRectFromScreen:NSMakeRect(mouseLoc.x, mouseLoc.y, 0, 0)];
	return PGPoint(mouseScreen.origin.x, window->manager->height - mouseScreen.origin.y);
}

PGMouseButton GetMouseState(PGWindowHandle window) {
	PGMouseButton button = PGMouseButtonNone;
	NSUInteger pressedButtonMask = [NSEvent pressedMouseButtons];
	if ((pressedButtonMask & (1 << 0)) != 0) {
		button |= PGLeftMouseButton;
	} else if ((pressedButtonMask & (1 << 1)) != 0) {
		button |= PGRightMouseButton;
	} else if ((pressedButtonMask & (1 << 2)) != 0) {
		button |= PGMiddleMouseButton;
	}
	return button;
}

PGWindowHandle PGCreateWindow(PGWorkspace* workspace, PGPoint position, std::vector<std::shared_ptr<TextView>> textfiles) {
	PGNSWindow *window;
	NSRect contentSize = NSMakeRect(0, 0, 1000.0, 700.0);
	NSUInteger windowStyleMask = NSTitledWindowMask | NSResizableWindowMask |
			NSClosableWindowMask | NSMiniaturizableWindowMask;

	window = [[PGNSWindow alloc] initWithContentRect:contentSize styleMask:windowStyleMask backing:NSBackingStoreBuffered defer:YES];
	window.backgroundColor = [NSColor whiteColor];
	PGView *view = [[PGView alloc] initWithFrame:contentSize:window:workspace:textfiles];
	[view setWantsLayer:YES];
	window.contentView = view;
	[window makeFirstResponder:view];
	[window registerPGView:view];
	window.sharingType = NSWindowSharingReadWrite;

	[window cascadeTopLeftFromPoint:NSMakePoint(position.x, position.y)];
	[window setTitle:@"Panther"];
	[window setDelegate:view];
	return [view getHandle];
}

PGWindowHandle PGCreateWindow(PGWorkspace* workspace, std::vector<std::shared_ptr<TextView>> textfiles) {
	return PGCreateWindow(workspace, PGPoint(20, 20), textfiles);
}

void ShowWindow(PGWindowHandle handle) {
	[handle->window makeKeyAndOrderFront:nil];
}

void RefreshWindow(PGWindowHandle window, bool redraw_now) {
	RedrawWindow(window);
}

void RefreshWindow(PGWindowHandle window, PGIRect rectangle, bool redraw_now) {
	RedrawWindow(window, rectangle);
}


void RedrawWindow(PGWindowHandle window) {
	[window->view setNeedsDisplay:YES];
}

void RedrawWindow(PGWindowHandle window, PGIRect rectangle) {
	[window->view setNeedsDisplayInRect:[window->view getBounds]];
}

Control* GetFocusedControl(PGWindowHandle window) {
	return window->manager->GetActiveControl();
}

void RegisterControl(PGWindowHandle window, Control *control) {
	if (window->manager != nullptr)
		window->manager->AddControl(std::shared_ptr<Control>(control));
}

PGTime PGGetTimeOS() {
	timeval time;
	gettimeofday(&time, NULL);
	long millis = (time.tv_sec * 1000) + (time.tv_usec / 1000);
	return millis;
}

void SetWindowTitle(PGWindowHandle handle, std::string title) {
	[handle->window setTitle:[NSString stringWithUTF8String:title.c_str()]];
}

void SetClipboardTextOS(PGWindowHandle window, std::string text) {
	NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
	[pasteboard clearContents];
	 NSArray *copiedObjects = [NSArray arrayWithObject:[NSString stringWithUTF8String:text.c_str()]];
	[pasteboard writeObjects:copiedObjects];
}

std::string GetClipboardTextOS(PGWindowHandle window) {
	NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
	NSArray *classArray = [NSArray arrayWithObject:[NSString class]];
	NSDictionary *options = [NSDictionary dictionary];
 
	BOOL ok = [pasteboard canReadObjectForClasses:classArray options:options];
	if (ok) {
		NSArray *objectsToPaste = [pasteboard readObjectsForClasses:classArray options:options];
		NSString *pasted_text = [objectsToPaste objectAtIndex:0];
		return std::string([pasted_text UTF8String]);
	}
	return std::string("");
}

PGLineEnding GetSystemLineEnding() {
	return PGLineEndingUnix;
}

char GetSystemPathSeparator() {
	return '/';
}

bool WindowHasFocus(PGWindowHandle window) {
	// FIXME
	return true;
}

void SetCursor(PGWindowHandle window, PGCursorType type) {
	switch (type) {
		case PGCursorStandard:
			[[NSCursor arrowCursor] set];
			break;
		case PGCursorCrosshair:
			[[NSCursor crosshairCursor] set];
			break;
		case PGCursorHand:
			[[NSCursor pointingHandCursor] set];
			break;
		case PGCursorIBeam:
			[[NSCursor IBeamCursor] set];
			break;
		case PGCursorWait:
			[[NSCursor arrowCursor] set];
			break;
		default:
			break;
	}
	return;
}

void PGCloseWindow(PGWindowHandle handle) {
	handle->pending_destroy = true;
}

PGSize GetWindowSize(PGWindowHandle window) {
	auto size = [window->window frame].size;
	return PGSize(size.width, size.height);
}

PGPoint PGGetWindowPosition(PGWindowHandle window) {
	auto pos = [window->window frame].origin;
	return PGPoint(pos.x, pos.y);
}

PGTimerHandle CreateTimer(PGWindowHandle handle, int ms, PGTimerCallback callback, PGTimerFlags flags) {
	return [handle->view scheduleTimer:handle:ms:callback:flags];
}

void DeleteTimer(PGTimerHandle handle) {
	[handle->timer invalidate];
	delete handle;
}

ControlManager* GetWindowManager(PGWindowHandle window) {
	return window->manager.get();
}


PGPopupMenuHandle PGCreatePopupMenu(PGWindowHandle window, Control* control) {
	PGPopupMenuHandle handle = new PGPopupMenu();
	handle->control = control;
	handle->window = window;
	handle->menu = [[NSMenu alloc] initWithTitle:@"Contextual Menu"];
	return handle;
}

void PGPopupMenuInsertSubmenu(PGPopupMenuHandle handle, PGPopupMenuHandle submenu, std::string submenu_name) {
	// FIXME
	//assert(0);
}

void PGPopupMenuInsertEntry(PGPopupMenuHandle handle, PGPopupInformation information, PGPopupCallback callback, PGPopupMenuFlags flags) {
	NSMenuItem* item = [[NSMenuItem alloc] initWithTitle:[NSString stringWithUTF8String:information.text.c_str()] action:@selector(popupMenuPress:) keyEquivalent:@""];
	NSValue* _callback = [NSValue valueWithPointer:(void*)callback];
	NSValue* _handle = [NSValue valueWithPointer:(void*)handle];
	NSNumber* _info = [NSNumber numberWithLongLong:(lng)handle->info.size()];
	handle->info.push_back(information);

	NSArray* array = @[_callback, _handle, _info];
	[item setRepresentedObject:array];
	[handle->menu addItem:item];
}

void PGPopupMenuInsertSeparator(PGPopupMenuHandle handle) {
	[handle->menu addItem:[NSMenuItem separatorItem]];
}

void PGDisplayPopupMenu(PGPopupMenuHandle handle, PGTextAlign align) {
	 NSObject *newObject = [[NSObject alloc] init];
	[NSMenu popUpContextMenu:handle->menu withEvent:handle->window->event forView:handle->window->view];
}

void PGDisplayPopupMenu(PGPopupMenuHandle, PGPoint, PGTextAlign align) {
	assert(0);
}

void OpenFolderInExplorer(std::string path) {
	assert(0);
}

void OpenFolderInTerminal(std::string path) {
	assert(0);
}

std::vector<std::string> ShowOpenFileDialog(bool allow_files, bool allow_directories, bool allow_multiple_selection) {
	NSOpenPanel *panel = [NSOpenPanel openPanel];

	// panel configuration
	[panel setCanChooseFiles:allow_files ? YES : NO];
	[panel setCanChooseDirectories:allow_directories ? YES : NO];
	[panel setAllowsMultipleSelection:allow_multiple_selection ? YES : NO];

	std::vector<std::string> selected_files;
	NSInteger result = [panel runModal];
	if (result == NSFileHandlingPanelOKButton) {
		for (NSURL *fileURL in [panel URLs]) {
			// we remove the "file://" prefix, we are not dealing with URLs
			// but with local files (hopefully)
			std::string str = std::string([[fileURL absoluteString] UTF8String]);
			assert(str.substr(0, 7) == std::string("file://"));
			selected_files.push_back(str.substr(7));
		}
	}
	return selected_files;
}

std::string ShowSaveFileDialog() {
    NSSavePanel *panel = [NSSavePanel savePanel];

	NSInteger result = [panel runModal];
	if (result == NSFileHandlingPanelOKButton) {
		NSURL* fileURL = [panel URL];

		std::string str = std::string([[fileURL absoluteString] UTF8String]);
		assert(str.substr(0, 7) == std::string("file://"));
		return str;
	}
	return "";
}


PGPoint ConvertWindowToScreen(PGWindowHandle window, PGPoint point) {
	assert(0);
	return PGPoint(0, 0);
}

void PGStartDragDrop(PGWindowHandle handle, PGBitmapHandle image, PGDropCallback callback, void* data, size_t data_length) {
	CGImageRef cg_image = SkCreateCGImageRef(*PGGetBitmap(image));
	NSImage* nsimage = [[NSImage alloc] initWithCGImage:cg_image size:NSZeroSize];

	NSSize size = [nsimage size];

	NSPasteboardItem *pbItem = [NSPasteboardItem new];
	[pbItem setData:[[NSData alloc] initWithBytes:&data length:sizeof(void*)] forType:@"com.panther.draggingobject"];

	if (active_session == nullptr) {
		NSDraggingItem *dragItem = [[NSDraggingItem alloc] initWithPasteboardWriter:pbItem];
		NSPoint dragPosition = [handle->view convertPoint:[handle->event locationInWindow] fromView:nil];
		dragPosition.x -= size.width / 2;
		dragPosition.y -= size.height /2;
		NSRect draggingRect = NSMakeRect(dragPosition.x, dragPosition.y, size.width, size.height);
		[dragItem setDraggingFrame:draggingRect contents:nsimage];

		[handle->view startDragging:[NSArray arrayWithObject:dragItem] event:handle->event source:handle->view callback:callback data:data];
	} else {
		/*
		NSImage* image = [[NSImage alloc] initWithSize:size];
		[active_session enumerateDraggingItemsWithOptions:NSDraggingItemEnumerationClearNonenumeratedImages
									  forView:handle->view
									  classes:[NSArray arrayWithObject:pbItem]
								searchOptions:nil
								   usingBlock:^(NSDraggingItem *draggingItem, NSInteger idx, BOOL *stop) {
									   [draggingItem setDraggingFrame:draggingItem.draggingFrame contents:nsimage];
									   *stop = NO;
								   }];*/
	}
}

void PGCancelDragDrop(PGWindowHandle window) {
	/*
	if (active_session != nullptr) {
		NSSize size;
		size.width = 0;
		size.height = 0;
		NSImage* image = [[NSImage alloc] initWithSize:size];
		[active_session enumerateDraggingItemsWithOptions:NSDraggingItemEnumerationClearNonenumeratedImages
									  forView:window->view
									  classes:@[]
								searchOptions:nil
								   usingBlock:^(NSDraggingItem *draggingItem, NSInteger idx, BOOL *stop) {
									   [draggingItem setDraggingFrame:draggingItem.draggingFrame contents:image];
									   *stop = NO;
								   }];
		PGLogMessage("Cancel drag drop operation.");
	}*/
}

std::string GetOSName() {
	return "macos";
}


PGResponse PGConfirmationBox(PGWindowHandle window, std::string title, std::string message, PGConfirmationBoxType type) {
	NSAlert *alert = [[NSAlert alloc] init];
	if (type == PGConfirmationBoxYesNoCancel) {
		[alert addButtonWithTitle:@"Save"];
		[alert addButtonWithTitle:@"Cancel"];
		[alert addButtonWithTitle:@"Don't Save"];
	} else if (type == PGConfirmationBoxYesNo) {
		[alert addButtonWithTitle:@"Yes"];
		[alert addButtonWithTitle:@"No"];
	}
	[alert setMessageText:[NSString stringWithUTF8String:title.c_str()]];
	[alert setInformativeText:[NSString stringWithUTF8String:message.c_str()]];
	[alert setAlertStyle:NSWarningAlertStyle];
	auto alert_response = [alert runModal];
	PGResponse response;
	if (alert_response == NSAlertFirstButtonReturn) {
		response = PGResponseYes;
	} else if (alert_response == NSAlertSecondButtonReturn) {
		response = PGResponseCancel;
	} else {
		response = PGResponseNo;
	}
	return response;
}

void PGConfirmationBox(PGWindowHandle window, std::string title, std::string message, PGConfirmationCallback callback, Control* control, void* data, PGConfirmationBoxType type) {
	PGResponse response = PGConfirmationBox(window, title, message, type);
	callback(window, control, data, response);
}


PGWorkspace* PGGetWorkspace(PGWindowHandle window) {
	return window->workspace;
}

void PGLoadWorkspace(PGWindowHandle window, nlohmann::json& j) {
	if (j.count("window") > 0) {
		nlohmann::json w = j["window"];
		if (w.count("dimensions") > 0) {
			nlohmann::json dim = w["dimensions"];
			if (dim.count("width") > 0 && dim["width"].is_number() && 
				dim.count("height") > 0 && dim["height"].is_number() && 
				dim.count("x") > 0 && dim["x"].is_number() && 
				dim.count("y") > 0 && dim["y"].is_number()) {
				int x = dim["x"];
				int y = dim["y"];
				int width = dim["width"];
				int height = dim["height"];

				NSRect rect = NSMakeRect(x, y, width, height);
				[window->window setFrame:rect display:false];
			}
		}
	}
	window->manager->LoadWorkspace(j);
}

void PGWriteWorkspace(PGWindowHandle window, nlohmann::json& j) {
	if (!window->manager) return;
	PGSize window_size = GetWindowSize(window);
	j["window"]["dimensions"]["width"] = window_size.width;
	j["window"]["dimensions"]["height"] = window_size.height;
	PGPoint window_position = PGGetWindowPosition(window);
	j["window"]["dimensions"]["x"] = window_position.x;
	j["window"]["dimensions"]["y"] = window_position.y;
	window->manager->WriteWorkspace(j);
}

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

PGFileInformation PGGetFileFlags(std::string path) {
	PGFileInformation info;
	struct stat stat_info;
	info.flags = PGFileFlagsEmpty;

	int ret = stat(path.c_str(), &stat_info);
	if (ret != 0) {
		if (errno == ENOTDIR || errno == ENOENT) {
			info.flags = PGFileFlagsFileNotFound;
		} else {
			info.flags = PGFileFlagsErrorOpeningFile;
		}
		errno = 0;
		return info;
	}
	info.file_size = (lng) stat_info.st_size;
	info.modification_time = (lng) stat_info.st_mtime;
	info.is_directory = S_ISDIR(stat_info.st_mode);

	return info;
}


PGIOError PGRenameFile(std::string source, std::string dest) {
	NSError * _Nullable __autoreleasing err = nil;
	if ([[NSFileManager defaultManager] replaceItemAtURL: 
			[NSURL fileURLWithPath:[NSString stringWithUTF8String:dest.c_str()]]
			withItemAtURL:[NSURL fileURLWithPath:[NSString stringWithUTF8String:source.c_str()]]
			backupItemName:nil
			options:NSFileManagerItemReplacementUsingNewMetadataOnly
			resultingItemURL:nil
			 error:&err] == YES) {
		return PGIOSuccess;
	}
	NSLog(@"%@", [err localizedDescription]);
	return PGIOErrorOther;
}

PGIOError PGRemoveFile(std::string source) {
	NSError * _Nullable __autoreleasing err = nil;
	if ([[NSFileManager defaultManager] removeItemAtPath:
			[NSString stringWithUTF8String:source.c_str()]
			 error:&err] == YES) {
		return PGIOSuccess;
	}
	//NSLog(err);
	return PGIOErrorOther;
}

void PGLogMessage(std::string text) {
	NSLog(@"%s", text.c_str());
}


PGTooltipHandle PGCreateTooltip(PGWindowHandle window, PGRect rect, std::string text) {
	// we have to invert the Y coordinate for OSX
	auto bounds = [window->view getBounds];
	rect = PGRect(rect.x, bounds.size.height - rect.y - rect.height, rect.width, rect.height);
	PGTooltipHandle handle = new PGTooltip();
	handle->region = rect;
	handle->window = window;
	handle->SetText([NSString stringWithUTF8String:text.c_str()]);
	NSRect r = NSMakeRect(rect.x, rect.y, rect.width, rect.height);
	handle->tooltip = [window->view addToolTipRect:r
				owner:handle->text
				userData:nullptr];
	return handle;
}

void PGUpdateTooltipRegion(PGTooltipHandle handle, PGRect rect) {
	// we have to invert the Y coordinate for OSX
	auto bounds = [handle->window->view getBounds];
	rect = PGRect(rect.x, bounds.size.height - rect.y - rect.height, rect.width, rect.height);
	if (!(handle->region.x != rect.x || handle->region.y != rect.y || 
		handle->region.width != rect.width || handle->region.height != rect.height)) {
		// the current region is identical to the old region; don't update
		return;
	}
	[handle->window->view removeToolTip:handle->tooltip];
	NSRect r = NSMakeRect(rect.x, rect.y, rect.width, rect.height);
	handle->region = rect;
	handle->tooltip = [handle->window->view addToolTipRect:r
				owner:handle->text
				userData:nullptr];
}

void PGDestroyTooltip(PGTooltipHandle handle) {
	[handle->window->view removeToolTip:handle->tooltip];
	delete handle;
}

void SetWindowManager(PGWindowHandle window, std::shared_ptr<ControlManager> manager) {
	window->manager = manager;
}

PGPopupMenuHandle PGCreateMenu(PGWindowHandle window, Control* control) {
	return nullptr;
}

void PGSetWindowMenu(PGWindowHandle window, PGPopupMenuHandle menu) {

}

#include <dirent.h>

PGDirectoryFlags PGGetDirectoryFilesOS(std::string directory, std::vector<PGFile>& directories,
	std::vector<PGFile>& files, void* glob) {
	DIR *dp;
	struct dirent *ep;
	dp = opendir(directory.c_str());
	if (dp == NULL) {
		return PGDirectoryNotFound;
	}

	while ((ep = readdir(dp))) {
		std::string filename = ep->d_name;
		if (filename[0] == '.') continue;

		if (PGFileIsIgnored(glob, filename.c_str(), ep->d_type == DT_DIR))
			continue;

		if (ep->d_type == DT_DIR) {
			directories.push_back(PGFile(filename));
		} else if (ep->d_type == DT_REG) {
			files.push_back(PGFile(filename));
		}
	}

	(void)closedir(dp);

	return PGDirectorySuccess;
}

std::string PGApplicationPath() {
	return std::string([[[NSBundle mainBundle] bundlePath] UTF8String]) ;
}

std::string PGCurrentDirectory() {
	char temp[8192];
	return (getcwd(temp, 8192) ? std::string(temp) : std::string(""));
}

