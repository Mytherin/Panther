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

        auto windows = workspace->GetWindows();
        for (auto it = windows.begin(); it != windows.end(); it++) {
            ShowWindow(*it);
        }
    }
    return self;
 }

-(void)applicationWillFinishLaunching:(NSNotification *)notification
{
}

-(void)applicationDidFinishLaunching:(NSNotification *)notification
{
} 

@end
