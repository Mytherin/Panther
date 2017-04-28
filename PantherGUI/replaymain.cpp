#ifdef PANTHER_REPLAY

#include "main.h"
#include "windowfunctions.h"
#include "control.h"
#include "scheduler.h"
#include "filemanager.h"
#include "container.h"
#include "droptarget.h"
#include "encoding.h"
#include "unicode.h"
#include "language.h"
#include "logger.h"
#include "style.h"

#include "statusbar.h"
#include "textfield.h"
#include "tabcontrol.h"
#include "projectexplorer.h"
#include "toolbar.h"

#include "c.h"
#include "findresults.h"
#include "xml.h"

#include <malloc.h>

#include <vector>
#include <map>

#include "keybindings.h"
#include "settings.h"
#include "globalsettings.h"

#include "replaymanager.h"

int main() {
	PGGlobalReplayManager::Initialize("test.replay", PGReplayPlay);
	PGGlobalReplayManager::RunReplay();
}

#endif
