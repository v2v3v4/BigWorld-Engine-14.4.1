#import "WorldLayerThreeTouchHandler.h"

#import "WorldLayerTwoTouchHandler.h"
#import "WorldLayer.h"
#import "WorldUIModel.h"


@implementation WorldLayerThreeTouchHandler


- (CGPoint) positionOfTouches:(NSSet *)touches
{
	CGPoint position = ccp( 0, 0 );
	
	for (UITouch * touch in touches)
	{
		position = ccpAdd( position, [touch locationInView: [touch view]] );
	}
	
	return ccpMult( position, 1.f/[touches count] );
}

- (void) scaleToTouches: (NSSet *)touches
{
	CGPoint newPosition = [self positionOfTouches:touches];

	float change = newPosition.y - startPosition.y;
	
	worldLayer.uiModel.entityScale =startEntityScale * exp( -change/320.f );	
}


-(id) initWithLayer:(WorldLayer *)aWorldLayer andTouches:(NSSet *)touches
{
	if ((self = [super initWithLayer:aWorldLayer]))
	{
		startPosition = [self positionOfTouches:touches];
		startEntityScale = worldLayer.uiModel.entityScale;
	}
	
	return self;
}

- (BOOL) touchBegan:(UITouch *)touch withEvent:(UIEvent *)event andOwner:(id <TouchHandlerOwner>) owner
{
	[super touchBegan:touch withEvent:event andOwner:owner];
	
	return NO;
}

- (void) touchMoved:(UITouch *)touch withEvent:(UIEvent *)event andOwner:(id <TouchHandlerOwner>) owner
{
	[self scaleToTouches: [event allTouches]];
}

- (void) touchEnded:(UITouch *)touch withEvent:(UIEvent *)event andOwner:(id <TouchHandlerOwner>) owner
{
	[super touchEnded:touch withEvent:event andOwner:owner];
	[self scaleToTouches: [event allTouches]];

	NSMutableSet * touches = [[[event allTouches] mutableCopy] autorelease];
	[touches removeObject:touch];
	
	[owner setTouchHandler:
	 [[[WorldLayerTwoTouchHandler alloc] initWithLayer:worldLayer andTouches:touches] autorelease]];
	
}

@end
