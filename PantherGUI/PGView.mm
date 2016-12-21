
#include "textfile.h"
#include "control.h"
#include "textfield.h"
#include "time.h"
#include "scheduler.h"
#include "renderer.h"
#include "controlmanager.h"
#include "filemanager.h"
#include "container.h"

#include <sys/time.h>
#include <cctype>

#import "PGView.h"

#include "control.h"

#import "PGDrawBitmap.h"

// clang++ -framework Cocoa -fobjc-arc -lobjc -I/Users/myth/Sources/skia/skia/include/core -I/Users/myth/Sources/skia/skia/include/config -L/Users/myth/Sources/skia/skia/out/Static -lskia -std=c++11 main.mm AppDelegate.mm PGView.mm c.cpp control.cpp cursor.cpp keywords.cpp logger.cpp scheduler.cpp text.cpp syntaxhighlighter.cpp textfield.cpp textfile.cpp textline.cpp thread.cpp windowfunctions.cpp xml.cpp file.cpp tabcontrol.cpp tabbedtextfield.cpp renderer.cpp filemanager.cpp controlmanager.cpp utils.cpp  -o main

struct PGWindow {
    NSWindow *window;
	PGView* view;
	ControlManager* manager;
	PGRendererHandle renderer;
};

struct PGMouseFlags {
	int x;
	int y;
	PGMouseButton button;
	PGModifier modifiers;
};

@interface PGTimerObject : NSObject {
	PGTimerCallback callback;
}
-(id)initWithCallback:(PGTimerCallback)callback;
-(void)callFunction;
@end

@implementation PGTimerObject
-(id)initWithCallback:(PGTimerCallback)cb {
	self = [super init];
	if (self) {
		callback = cb;
	}
	return self;
}
-(void)callFunction {
	callback();
}
@end

PGWindowHandle global_handle;

#define MAX_REFRESH_FREQUENCY 1000/30

void PeriodicWindowRedraw(void) {
	global_handle->manager->PeriodicRender();
}

@implementation PGView : NSView

- (instancetype)initWithFrame:(NSRect)frameRect :(NSWindow*)window
{
    if(self = [super initWithFrame:frameRect]) {
		NSRect rect = [self getBounds];
    	PGWindowHandle res = new PGWindow();
    	res->window = window;
		res->view = self;
		ControlManager* manager = new ControlManager(res);
		manager->SetPosition(PGPoint(0, 0));
		manager->SetSize(PGSize(rect.size.width, rect.size.height));
		manager->SetAnchor(PGAnchorLeft | PGAnchorRight | PGAnchorTop | PGAnchorBottom);
		res->manager = manager;
		res->renderer = InitializeRenderer();
		
		Scheduler::Initialize();
		Scheduler::SetThreadCount(8);

		CreateTimer(res, MAX_REFRESH_FREQUENCY, PeriodicWindowRedraw, PGTimerFlagsNone);
	
		TextFile* textfile = FileManager::OpenFile("/Users/myth/Data/tibiawiki_pages_current.xml");
		TextFile* textfile2 = FileManager::OpenFile("/Users/myth/pyconversion.c");
		PGContainer* tabbed = new PGContainer(res, textfile);
		tabbed->SetAnchor(PGAnchorLeft | PGAnchorRight | PGAnchorTop | PGAnchorBottom);
		tabbed->UpdateParentSize(PGSize(0, 0), manager->GetSize());

		global_handle = res;
    }
    return self;
}

- (void)drawRect:(NSRect)invalidateRect {
	SkBitmap bitmap;

	PGIRect rect(
		invalidateRect.origin.x,
		invalidateRect.origin.y,
		invalidateRect.size.width,
		invalidateRect.size.height);

	RenderControlsToBitmap(global_handle->renderer, bitmap, rect, global_handle->manager);

	// draw bitmap
	CGContextRef context = (CGContextRef) [[NSGraphicsContext currentContext] graphicsPort];
	SkCGDrawBitmap(context, bitmap, 0, 0);
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

- (void)mouseDown:(NSEvent *)event {
	PGMouseFlags flags = [self getMouseFlags:event];
	global_handle->manager->MouseDown(flags.x, flags.y, PGLeftMouseButton, flags.modifiers);
}

- (void)mouseUp:(NSEvent *)event {
	PGMouseFlags flags = [self getMouseFlags:event];
	global_handle->manager->MouseUp(flags.x, flags.y, PGLeftMouseButton, flags.modifiers);
}

- (void)rightMouseDown:(NSEvent *)event {
	PGMouseFlags flags = [self getMouseFlags:event];
	global_handle->manager->MouseDown(flags.x, flags.y, PGRightMouseButton, flags.modifiers);
}

- (void)rightMouseUp:(NSEvent *)event {
	PGMouseFlags flags = [self getMouseFlags:event];
	global_handle->manager->MouseUp(flags.x, flags.y, PGRightMouseButton, flags.modifiers);
}

- (void)otherMouseDown:(NSEvent *)event {
	// FIXME: not just middle mouse button
	PGMouseFlags flags = [self getMouseFlags:event];
	global_handle->manager->MouseDown(flags.x, flags.y, PGMiddleMouseButton, flags.modifiers);
}

- (void)otherMouseUp:(NSEvent *)event {
	// FIXME: not just middle mouse button
	PGMouseFlags flags = [self getMouseFlags:event];
	global_handle->manager->MouseUp(flags.x, flags.y, PGMiddleMouseButton, flags.modifiers);
}

- (void)scrollWheel:(NSEvent *)event {
	PGMouseFlags flags = [self getMouseFlags:event];
	global_handle->manager->MouseWheel(flags.x, flags.y, [event deltaY] < 0 ? -120 : 120, flags.modifiers);
}
/*
- (void)mouseDragged:(NSEvent *)event {
	PGMouseFlags flags = [self getMouseFlags:event];
	global_handle->manager->MouseMove(flags.x, flags.y,  flags.button);
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
		if (keyChar == 127) {
			button = PGButtonBackspace;
		} else if (keyChar == '\r') {
			button = PGButtonEnter;
		} else if (keyChar == NSLeftArrowFunctionKey) {
			button = PGButtonLeft;
		} else if (keyChar == NSRightArrowFunctionKey) {
			button = PGButtonRight;
		} else if (keyChar == NSUpArrowFunctionKey) {
			button = PGButtonUp;
		} else if (keyChar == NSDownArrowFunctionKey) {
			button = PGButtonDown;
		} else if (keyChar == NSInsertFunctionKey) {

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
		} else if (keyChar == NSPrintScreenFunctionKey) {
			
		} else if (keyChar == NSScrollLockFunctionKey) {
			
		} else if (keyChar == NSPauseFunctionKey) {
			
		} else if (keyChar == NSUndoFunctionKey) {
			
		} else if (keyChar == NSRedoFunctionKey) {
			
		} else if (keyChar == NSFindFunctionKey) {
			
		}
		if (button != PGButtonNone) {
			global_handle->manager->KeyboardButton(button, modifiers);
		} else if (keyChar >= 0x20 && keyChar <= 0x7E) {
			if (modifiers == PGModifierShift)
				modifiers = PGModifierNone;
			if (modifiers != PGModifierNone)
				keyChar = toupper(keyChar);
			global_handle->manager->KeyboardCharacter(keyChar, modifiers);
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

- (void)keyUp:(NSEvent *)event {

}

- (void)targetMethod:(NSTimer*)theTimer {
	[[theTimer userInfo] callFunction];
}

- (void)scheduleTimer:(int)ms :(PGTimerCallback)callback :(PGTimerFlags)flags {
	PGTimerObject* object = [[PGTimerObject alloc] initWithCallback:callback];
	[NSTimer scheduledTimerWithTimeInterval:ms / 1000.0
		target:self
		selector:@selector(targetMethod:)
		userInfo:object
		repeats:flags & PGTimerExecuteOnce ? NO : YES];
}
@end

PGPoint GetMousePosition(PGWindowHandle window) {
	//get current mouse position
	NSPoint mouseLoc = [NSEvent mouseLocation];
	// convert screen coordinates to local coordinates of the window
	NSRect mouseScreen = [window->window convertRectFromScreen:NSMakeRect(mouseLoc.x, mouseLoc.y, 0, 0)];
	return PGPoint(mouseScreen.origin.x, [window->view getBounds].size.height - mouseScreen.origin.y);
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

PGWindowHandle PGCreateWindow(void) {
	assert(0);
	return nullptr;
}


void RefreshWindow(PGWindowHandle window) {
	RedrawWindow(window);
}

void RefreshWindow(PGWindowHandle window, PGIRect rectangle) {
	RedrawWindow(window, rectangle);
}


void RedrawWindow(PGWindowHandle window) {
	[window->view setNeedsDisplay:YES];
}

void RedrawWindow(PGWindowHandle window, PGIRect rectangle) {
	[window->view setNeedsDisplayInRect:[window->view getBounds]];
}

Control* GetFocusedControl(PGWindowHandle window) {
	return window->manager->GetFocusedControl();
}

void RegisterControl(PGWindowHandle window, Control *control) {
	if (window->manager != nullptr)
		window->manager->AddControl(control);
}

PGTime GetTime() {
	timeval time;
	gettimeofday(&time, NULL);
	long millis = (time.tv_sec * 1000) + (time.tv_usec / 1000);
	return millis;
}

void SetWindowTitle(PGWindowHandle window, char* title) {
	assert(0);
}

void SetClipboardText(PGWindowHandle window, std::string text) {
	//[[NSPasteboard generalPasteboard] clearContents];
	//[[NSPasteboard generalPasteboard] setString:helloField.stringValue  forType:NSStringPboardType];
	assert(0);
}

std::string GetClipboardText(PGWindowHandle window) {
	assert(0);
	return std::string("");
}

PGLineEnding GetSystemLineEnding() {
	return PGLineEndingMacOS;
}

char GetSystemPathSeparator() {
	return '/';
}

bool WindowHasFocus(PGWindowHandle window) {
	// FIXME
	return true;
}

void SetCursor(PGWindowHandle window, PGCursorType type) {
	return;
}

struct PGTimer {

};

PGTimerHandle CreateTimer(PGWindowHandle handle, int ms, PGTimerCallback callback, PGTimerFlags flags) {
	[handle->view scheduleTimer:ms:callback:flags];
	/*
	dispatch_time_t popTime = dispatch_time(DISPATCH_TIME_NOW, (int64_t)(ms * NSEC_PER_MSEC));
	dispatch_after(popTime, dispatch_get_main_queue(), ^(void){
		callback();
		CreateTimer(ms, callback, flags);
	});*/
	return nullptr;
}

void DeleteTimer(PGTimerHandle handle) {

}

void* GetControlManager(PGWindowHandle window) {
	return window->manager;
}

