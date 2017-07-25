//
//  AppDelegate.m
//  TestUI
//
//  Created by Mark Raasveldt on 13/12/16.
//  Copyright © 2016 Mytherin. All rights reserved.
//

#import "AppDelegate.h"
#import "PGView.h"

#include "windowfunctions.h"
#include "xml.h"
#include "c.h"
#include "findresults.h"
#include "scheduler.h"

#include "encoding.h"
#include "settings.h"
#include "keybindings.h"
#include "workspace.h"
#include "globalsettings.h"

#include "PGWindow.h"


std::vector<PGWorkspace*> open_workspaces;

void PGInitialize() {
    PGWorkspace* workspace = PGInitializeFirstWorkspace();
    open_workspaces.push_back(workspace);
    auto& windows = workspace->GetWindows();
    for (auto it = windows.begin(); it != windows.end(); it++) {
        ShowWindow(*it);
    }
}

@interface AppDelegate ()
@end

@implementation AppDelegate

-(id)init
{
    if(self = [super init]) {
        PGInitializeEncodings();

        PGInitializeCursors();
        PGInitializeGlobals();
        PGInitialize();

        [[NSDistributedNotificationCenter defaultCenter]
            addObserver:self
            selector:@selector(activateNotification:)
            name:PANTHER_ACTIVATE_NOTIFICATION
            object:nil];
        [[NSDistributedNotificationCenter defaultCenter]
            addObserver:self
            selector:@selector(openFileNotification:)
            name:PANTHER_OPENFILE_NOTIFICATION
            object:nil];
    }
    return self;
 }

-(void)dealloc
{
    [[NSDistributedNotificationCenter defaultCenter]
        removeObserver:self
        name:PANTHER_ACTIVATE_NOTIFICATION
        object:nil];
    [[NSDistributedNotificationCenter defaultCenter]
        removeObserver:self
        name:PANTHER_OPENFILE_NOTIFICATION
        object:nil];
    [super dealloc];
}

-(void)activateNotification:(NSNotification*)notification {
    if (open_workspaces.size() == 0) {
        PGInitialize();
    }
    assert(open_workspaces.front()->GetWindows().size() > 0);
    [[NSRunningApplication currentApplication]
        activateWithOptions:NSApplicationActivateIgnoringOtherApps];
}

-(void)openFileNotification:(NSNotification*)notification {
    if (open_workspaces.size() == 0) {
        PGInitialize();
    }
    PGWorkspace* workspace = open_workspaces.front();
    auto& windows = workspace->GetWindows();
    assert(windows.size() > 0);
    std::string file = std::string([[notification object] UTF8String]);
    GetWindowManager(windows[0])->DropFile(file);
}

-(void)applicationWillFinishLaunching:(NSNotification *)notification
{
}

-(void)applicationDidFinishLaunching:(NSNotification *)notification
{
}

void PGCloseWorkspace(PGWorkspace* workspace) {
    open_workspaces.erase(std::find(open_workspaces.begin(), open_workspaces.end(), workspace));
}
@end
