#import <Cocoa/Cocoa.h>
#import "PGView.h"

#include "windowfunctions.h"
#include "xml.h"
#include "c.h"
#include "scheduler.h"

#include "settings.h"
#include "keybindings.h"

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

    TextFile* textfile = TextFile::OpenTextFile(nullptr, "/Users/myth/pyconversion.c", false);
    TextFile* textfile2 = TextFile::OpenTextFile(nullptr, "/Users/myth/Data/tibiawiki_pages_current.xml", false);
    std::vector<TextFile*> textfiles;
    textfiles.push_back(textfile);
    textfiles.push_back(textfile2);

    PGCreateWindow(textfiles);

    
    [NSApp activateIgnoringOtherApps:YES];
    [NSApp run];                                                  // Call the Apps Run method

    return 0;       // App Never gets here.*/
}


