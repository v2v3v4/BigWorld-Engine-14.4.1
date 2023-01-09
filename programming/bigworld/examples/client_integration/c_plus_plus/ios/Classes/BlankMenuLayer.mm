#import "BlankMenuLayer.h"

// MenuLayer implementation
@implementation BlankMenuLayer

+ (id) scene
{
	CCScene * scene = [CCScene node];
	BlankMenuLayer * layer = [[self class] node];
	[scene addChild: layer];

	return scene;
}


+ (void) activate
{
	[[CCDirector sharedDirector] replaceScene:[[self class] scene]];
}


// on "init" you need to initialize your instance
- (id) init
{
	// always call "super" init
	// Apple recommends to re-assign "self" with the "super" return value
	if( (self=[super init] ))
	{
	}

	return self;
}


- (void) dealloc
{
	[super dealloc];
}

@end
