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


std::vector<PGWorkspace*> open_workspaces;

void PGInitialize() {
    PGWorkspace* workspace = PGInitializeFirstWorkspace();
    open_workspaces.push_back(workspace);
}

@interface AppDelegate ()
@end

@implementation AppDelegate

-(id)init
{
    if(self = [super init]) {
        PGInitializeEncodings();

        PGInitialize();

        PGWorkspace* workspace = open_workspaces.back();

        auto& windows = workspace->GetWindows();
        for (auto it = windows.begin(); it != windows.end(); it++) {
            ShowWindow(*it);
        }

        [[NSDistributedNotificationCenter defaultCenter]
            addObserver:self
            selector:@selector(notificationEvent:)
            name:@"pantherOpenFile"
            object:nil];
    }
    return self;
 }

-(void)dealloc
{
    [[NSDistributedNotificationCenter defaultCenter]
        removeObserver:self
        name:@"pantherOpenFile"
        object:nil];
    [super dealloc];
}

-(void)notificationEvent:(NSNotification*)notification {
    if (open_workspaces.size() == 0) return;
    PGWorkspace* workspace = open_workspaces[0];
    auto& windows = workspace->GetWindows();
    if (windows.size() == 0) return;
    std::string file = std::string([[notification object] UTF8String]);
    GetWindowManager(windows[0])->DropFile(file);
}

-(void)applicationWillFinishLaunching:(NSNotification *)notification
{
}

-(void)applicationDidFinishLaunching:(NSNotification *)notification
{
}

@end
