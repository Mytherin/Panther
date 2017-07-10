//
//  AppDelegate.h
//  TestUI
//
//  Created by Mark Raasveldt on 13/12/16.
//  Copyright © 2016 Mytherin. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#define PANTHER_ACTIVATE_NOTIFICATION @"pantherActivate"
#define PANTHER_OPENFILE_NOTIFICATION @"pantherOpenFile"

@interface AppDelegate : NSObject <NSApplicationDelegate>


-(void)activateNotification:(NSNotification*)notification;
-(void)openFileNotification:(NSNotification*)notification;
@end

