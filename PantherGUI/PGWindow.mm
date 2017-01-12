
#include "PGWindow.h"
#include "PGView.h"

@implementation PGNSWindow : NSWindow

-(void)close{
	[((PGView*)[self firstResponder]) performClose];
	[super close];
}

@end
