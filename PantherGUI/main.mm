#import <Cocoa/Cocoa.h>
#import "PGView.h"

#include "windowfunctions.h"
#include "xml.h"
#include "c.h"
#include "scheduler.h"

#include "settings.h"
#include "keybindings.h"
#include "workspace.h"

// Compilation: clang++ -framework Cocoa -fobjc-arc -lobjc test.mm AppDelegate.mm -o test

// clang++ -framework Cocoa -fobjc-arc -lobjc -I/Users/myth/Sources/skia/skia/include/core -I/Users/myth/Sources/skia/skia/include/config -L/Users/myth/Sources/skia/skia/out/Static -lskia -std=c++11 main.mm AppDelegate.mm PGView.mm -o main

int main(int argc, const char *argv[])
{
    NSApplication *application = [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    //[[NSBundle mainBundle] loadNibNamed:@"MainMenu" owner:application topLevelObjects:&tl];
            
    PGLanguageManager::AddLanguage(new CLanguage());
    PGLanguageManager::AddLanguage(new XMLLanguage());

    PGSettingsManager::Initialize();
    PGKeyBindingsManager::Initialize();

    Scheduler::Initialize();
    Scheduler::SetThreadCount(8);

    TextFile* textfile = new TextFile(nullptr);
    std::vector<TextFile*> files;
    files.push_back(textfile);

    PGWindowHandle window = PGCreateWindow(files);
    ShowWindow(window);

    
    [NSApp activateIgnoringOtherApps:YES];
    [NSApp run];                                                  // Call the Apps Run method

    return 0;       // App Never gets here.*/
}


