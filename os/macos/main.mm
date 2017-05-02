#import <Cocoa/Cocoa.h>
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

// Compilation: clang++ -framework Cocoa -fobjc-arc -lobjc test.mm AppDelegate.mm -o test

// clang++ -framework Cocoa -fobjc-arc -lobjc -I/Users/myth/Sources/skia/skia/include/core -I/Users/myth/Sources/skia/skia/include/config -L/Users/myth/Sources/skia/skia/out/Static -lskia -std=c++11 main.mm AppDelegate.mm PGView.mm -o main


std::vector<PGWorkspace*> open_workspaces;

void PGInitialize() {
    PGWorkspace* workspace = PGInitializeFirstWorkspace();
    open_workspaces.push_back(workspace);
}

int main(int argc, const char *argv[]) {
    PGInitializeEncodings();
    NSApplication *application = [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    //[[NSBundle mainBundle] loadNibNamed:@"MainMenu" owner:application topLevelObjects:&tl];
            


    PGInitialize();
    PGWorkspace* workspace = open_workspaces.back();

    auto windows = workspace->GetWindows();
    for (auto it = windows.begin(); it != windows.end(); it++) {
        ShowWindow(*it);
    }
/*
    TextFile* textfile = new TextFile(nullptr);
    std::vector<std::shared_ptr<TextFile>> files;
    files.push_back(std::shared_ptr<TextFile>(textfile));

    PGWindowHandle window = PGCreateWindow(files);*/
    //ShowWindow(window);

    
    [NSApp activateIgnoringOtherApps:YES];
    [NSApp run];                                                  // Call the Apps Run method

    return 0;       // App Never gets here.*/
}


