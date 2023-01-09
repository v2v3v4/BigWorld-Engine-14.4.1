#import "WorldLayerOneTouchHandler.h"

#import "WorldLayerTouchHandler.h"
#import "WorldLayerNoTouchHandler.h"
#import "WorldLayerTwoTouchHandler.h"
#import "ApplicationDelegate.h"
#import "WorldLayer.h"
#import "EntityInfoLayer.h"

@implementation WorldLayerOneTouchHandler

-(id)initWithLayer:(WorldLayer *)aWorldLayer andTouch:(UITouch *)touch andEvent:(UIEvent *)event
{
	return [self initWithLayer:aWorldLayer andTouch:touch andEvent:event shouldMovePlayer:YES];
}

-(id)initWithLayer:(WorldLayer *)aWorldLayer andTouch:(UITouch *)touch andEvent:(UIEvent *)event shouldMovePlayer:(BOOL)movePlayer
{
	if ((self = [super init]))
	{
		self->worldLayer = aWorldLayer;
		self->shouldMovePlayer = movePlayer;
	}
	
	return self;
}

- (BOOL) touchBegan:(UITouch *)touch withEvent:(UIEvent *)event andOwner:(id <TouchHandlerOwner>) owner
{
	[super touchBegan:touch withEvent:event andOwner:owner];
	[owner setTouchHandler: [[[WorldLayerTwoTouchHandler alloc]
								  initWithLayer:worldLayer andTouches:[event allTouches]] autorelease]];
		
	return YES;
}


- (void) touchEnded: (UITouch *) touch
		  withEvent: (UIEvent *) event
		   andOwner: (id <TouchHandlerOwner>) owner
{
	[super touchEnded: touch withEvent: event andOwner: owner];
	
	if (shouldMovePlayer)
	{
		CGPoint location = [[CCDirector sharedDirector] convertToGL: [touch locationInView: [touch view]]];
		EntityView * touchedView = [worldLayer entityViewFromLocation: location];
		
		if (touchedView)
		{
			[[ApplicationDelegate sharedDelegate] entityViewWasClicked: touchedView];
		}
		else 
		{
			[worldLayer movePlayerTo: location];
		}
	}
	
	[owner setTouchHandler:
	 [[[WorldLayerNoTouchHandler alloc] initWithLayer:worldLayer] autorelease]];
	
}

@end

