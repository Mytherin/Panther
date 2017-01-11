
#include "textfile.h"
#include "control.h"
#include "textfield.h"
#include "time.h"
#include "scheduler.h"
#include "renderer.h"
#include "controlmanager.h"
#include "filemanager.h"
#include "container.h"
#include "statusbar.h"

#include "xml.h"
#include "c.h"

#include <sys/time.h>
#include <cctype>

#import "PGView.h"

#include "control.h"

#import "PGDrawBitmap.h"

// clang++ -framework Cocoa -fobjc-arc -lobjc -I/Users/myth/Sources/skia/skia/include/core -I/Users/myth/Sources/skia/skia/include/config -L/Users/myth/Sources/skia/skia/out/Static -lskia -std=c++11 main.mm AppDelegate.mm PGView.mm c.cpp control.cpp cursor.cpp keywords.cpp logger.cpp scheduler.cpp text.cpp syntaxhighlighter.cpp textfield.cpp textfile.cpp textline.cpp thread.cpp windowfunctions.cpp xml.cpp file.cpp tabcontrol.cpp tabbedtextfield.cpp renderer.cpp filemanager.cpp controlmanager.cpp utils.cpp  -o main

struct PGPopupMenu {
	PGWindowHandle window;
	std::map<int, PGControlCallback> callbacks;
	Control* control;
	NSMenu* menu;
};

struct PGWindow {
	NSWindow *window;
	NSEvent *event;
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
		res->manager = manager;
		res->renderer = InitializeRenderer();
			
		PGLanguageManager::AddLanguage(new CLanguage());
		PGLanguageManager::AddLanguage(new XMLLanguage());

		Scheduler::Initialize();
		Scheduler::SetThreadCount(8);

		CreateTimer(res, MAX_REFRESH_FREQUENCY, PeriodicWindowRedraw, PGTimerFlagsNone);

		TextFile* textfile = FileManager::OpenFile("/Users/myth/pyconversion.c");
		TextFile* textfile2 = FileManager::OpenFile("/Users/myth/Data/tibiawiki_pages_current.xml");
		PGContainer* tabbed = new PGContainer(res);
		tabbed->width = 0;
		tabbed->height = TEXT_TAB_HEIGHT;
		TextField* textfield = new TextField(res, textfile);
		textfield->SetAnchor(PGAnchorTop);
		textfield->percentage_height = 1;
		textfield->percentage_width = 1;
		TabControl* tabs = new TabControl(res, textfield);
		tabs->SetAnchor(PGAnchorTop | PGAnchorLeft);
		tabs->fixed_height = TEXT_TAB_HEIGHT;
		tabs->percentage_width = 1;
		tabbed->AddControl(tabs);
		tabbed->AddControl(textfield);
		textfield->vertical_anchor = tabs;

		StatusBar* bar = new StatusBar(res, textfield);
		bar->SetAnchor(PGAnchorLeft | PGAnchorBottom);
		bar->percentage_width = 1;
		bar->fixed_height = STATUSBAR_HEIGHT;

		tabbed->SetAnchor(PGAnchorBottom);
		tabbed->percentage_height = 1;
		tabbed->percentage_width = 1;
		tabbed->vertical_anchor = bar;
		//tabbed->SetPosition(PGPoint(50, 50));
		//tabbed->SetSize(manager->GetSize() - PGSize(100, 100));

		manager->AddControl(tabbed);
		manager->AddControl(bar);

		manager->statusbar = bar;
		manager->active_textfield = textfield;
		manager->active_tabcontrol = tabs;
		
		manager->SetPosition(PGPoint(0, 0));
		manager->SetSize(PGSize(rect.size.width, rect.size.height));
		manager->SetAnchor(PGAnchorLeft | PGAnchorRight | PGAnchorTop | PGAnchorBottom);

		global_handle = res;
	}
	return self;
}

- (void)drawRect:(NSRect)invalidateRect {
	NSRect window_size = [self getBounds];
	if (global_handle->manager->width != window_size.size.width || 
		global_handle->manager->height != window_size.size.height) {
		global_handle->manager->SetSize(PGSize(window_size.size.width, window_size.size.height));
	}

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
	global_handle->event = event;
	PGMouseFlags flags = [self getMouseFlags:event];
	global_handle->manager->MouseDown(flags.x, flags.y, PGLeftMouseButton, flags.modifiers);
}

- (void)mouseUp:(NSEvent *)event {
	global_handle->event = event;
	PGMouseFlags flags = [self getMouseFlags:event];
	global_handle->manager->MouseUp(flags.x, flags.y, PGLeftMouseButton, flags.modifiers);
}

- (void)rightMouseDown:(NSEvent *)event {
	global_handle->event = event;
	PGMouseFlags flags = [self getMouseFlags:event];
	global_handle->manager->MouseDown(flags.x, flags.y, PGRightMouseButton, flags.modifiers);
}

- (void)rightMouseUp:(NSEvent *)event {
	global_handle->event = event;
	PGMouseFlags flags = [self getMouseFlags:event];
	global_handle->manager->MouseUp(flags.x, flags.y, PGRightMouseButton, flags.modifiers);
}

- (void)otherMouseDown:(NSEvent *)event {
	// FIXME: not just middle mouse button
	global_handle->event = event;
	PGMouseFlags flags = [self getMouseFlags:event];
	global_handle->manager->MouseDown(flags.x, flags.y, PGMiddleMouseButton, flags.modifiers);
}

- (void)otherMouseUp:(NSEvent *)event {
	// FIXME: not just middle mouse button
	global_handle->event = event;
	PGMouseFlags flags = [self getMouseFlags:event];
	global_handle->manager->MouseUp(flags.x, flags.y, PGMiddleMouseButton, flags.modifiers);
}

- (void)scrollWheel:(NSEvent *)event {
	global_handle->event = event;
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
		if (keyChar == 27) {
			button = PGButtonEscape;
		} else if (keyChar == 127) {
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

-(void)controlCallback:(NSValue*)_callback :(NSValue*)_control {
	PGControlCallback callback = (PGControlCallback) [_callback pointerValue];
	Control* control = (Control*) [_control pointerValue];
	callback(control);
}

-(void)popupMenuPress:(NSMenuItem*)sender {
	NSArray* array = (NSArray*)[sender representedObject];
	PGControlCallback callback = (PGControlCallback)[(NSValue*)array[0] pointerValue];
	Control* control = (Control*)[(NSValue*)array[1] pointerValue];
	callback(control);
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
	return window->manager->GetActiveControl();
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
	NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
	[pasteboard clearContents];
	 NSArray *copiedObjects = [NSArray arrayWithObject:[NSString stringWithUTF8String:text.c_str()]];
	[pasteboard writeObjects:copiedObjects];
}

std::string GetClipboardText(PGWindowHandle window) {
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

void* GetWindowManager(PGWindowHandle window) {
	return window->manager;
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

void PGPopupMenuInsertEntry(PGPopupMenuHandle handle, std::string text, PGControlCallback callback, PGPopupMenuFlags flags) {
	NSMenuItem* item = [[NSMenuItem alloc] initWithTitle:[NSString stringWithUTF8String:text.c_str()] action:@selector(popupMenuPress:) keyEquivalent:@""];
	NSValue* _callback = [NSValue valueWithPointer:(void*)callback];
	NSValue* _handle = [NSValue valueWithPointer:(void*)handle->control];
	NSArray* array = @[_callback, _handle];
	[item setRepresentedObject:array];
	[handle->menu addItem:item];
}

void PGPopupMenuInsertSeparator(PGPopupMenuHandle handle) {
	[handle->menu addItem:[NSMenuItem separatorItem]];
	// FIXME
	//assert(0);
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

	// Configure your panel the way you want it
	[panel setCanChooseFiles:allow_files ? YES : NO];
	[panel setCanChooseDirectories:allow_directories ? YES : NO];
	[panel setAllowsMultipleSelection:allow_multiple_selection ? YES : NO];

	std::vector<std::string> selected_files;
	NSInteger result = [panel runModal];
	if (result == NSFileHandlingPanelOKButton) {
		for (NSURL *fileURL in [panel URLs]) {
			std::string str = std::string([[fileURL absoluteString] UTF8String]);
			assert(str.substr(0, 7) == std::string("file://"));
			selected_files.push_back(str.substr(7));
		}
	}
	return selected_files;
}


PGPoint ConvertWindowToScreen(PGWindowHandle window, PGPoint point) {
	assert(0);
	return PGPoint(0, 0);
}

