#import "WorldLayerTouchHandler.h"

@interface WorldLayerThreeTouchHandler : WorldLayerTouchHandler
{
	CGPoint startPosition;
	float startEntityScale;
}

-(id)initWithLayer:(WorldLayer *)aWorldLayer andTouches:(NSSet *)touches;

@end
