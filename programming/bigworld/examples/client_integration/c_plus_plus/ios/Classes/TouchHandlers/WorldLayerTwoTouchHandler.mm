#import "WorldLayerTwoTouchHandler.h"

#import "WorldLayer.h"
#import "WorldLayerOneTouchHandler.h"
#import "WorldLayerThreeTouchHandler.h"

#import "WorldUIModel.h"

// Zoom limits
#define SCALE_MIN 0.25  // zoomed out
#define SCALE_MAX 6.0   // zoomed in

@implementation WorldLayerTwoTouchHandler

// TODO: Should these be straight functions (or even a Category of touches).
-(float)distanceBetween:(NSSet *)touches
{
	CGPoint points[2];
	
	if ([self getPoints:points count:2 touches:touches])
	{
		return ccpDistance( points[0], points[1] );
	}
	
	return 0.f;
}

-(float)angleOf:(NSSet *)touches
{
	CGPoint points[2];
	
	if ([self getPoints:points count:2 touches:touches])
	{
		return ccpToAngle( ccpSub( points[1], points[0] ) );
	}

	NSLog( @"Failed to get points" );
	
	return 0.f;
}

-(CGAffineTransform)basisOf:(NSSet*)touches withScale:(float)scale
{
    CGPoint points[2];
	
	if ([self getPoints:points count:2 touches:touches])
	{
        CGPoint a = ccpMult( ccpSub( points[1], points[0] ), scale );
        CGPoint mid = ccpMidpoint( points[0], points[1] );
        return CGAffineTransformMake( a.x, a.y, -a.y, a.x, mid.x, mid.y );
    }
    
	NSLog( @"BasisOf: Failed to get points" );
	
	return CGAffineTransformIdentity;
}

-(CGAffineTransform)basisOf:(NSSet*)touches
{
    return [self basisOf:touches withScale:1.0f];
}

-(id)initWithLayer:(WorldLayer *)aWorldLayer andTouches:(NSSet *)touches
{
	if ((self = [super initWithLayer:aWorldLayer]))
	{
        startInvBasis_ = CGAffineTransformInvert( [self basisOf:touches] );
        startTransform_ = worldLayer.uiModel.transform;        
    }
	
	return self;
}

- (BOOL) touchBegan:(UITouch *)touch withEvent:(UIEvent *)event andOwner:(id <TouchHandlerOwner>) owner
{
	[super touchBegan:touch withEvent:event andOwner:owner];
	[owner setTouchHandler: [[[WorldLayerThreeTouchHandler alloc]
							  initWithLayer:worldLayer andTouches:[event allTouches]] autorelease]];
	
	return YES;
}

- (void) touchMoved:(UITouch *)touch withEvent:(UIEvent *)event andOwner:(id <TouchHandlerOwner>) owner
{
    CGAffineTransform basis = [self basisOf:[event allTouches]];
    CGAffineTransform delta = CGAffineTransformConcat( startInvBasis_, basis );
    CGAffineTransform transform = CGAffineTransformConcat( startTransform_, delta );

    float len = ccpLength( ccp(transform.a, transform.b) );
    if (len > SCALE_MAX || len < SCALE_MIN)
    {
        // We have scaled too much. Scale back to fit within the limits.
        float numerator = len > SCALE_MAX ? SCALE_MAX : SCALE_MIN;                                         
        basis = [self basisOf:[event allTouches] withScale:numerator/len];
        delta = CGAffineTransformConcat( startInvBasis_, basis );
        transform = CGAffineTransformConcat( startTransform_, delta );
    }

    worldLayer.uiModel.transform = transform;
    worldLayer.uiModel.followPlayer = NO;
}

- (void) touchEnded:(UITouch *)touch withEvent:(UIEvent *)event andOwner:(id <TouchHandlerOwner>) owner
{
	[super touchEnded:touch withEvent:event andOwner:owner];
	[owner setTouchHandler: [[[WorldLayerOneTouchHandler alloc]
							  initWithLayer:worldLayer andTouch:touch andEvent:event shouldMovePlayer:NO] autorelease]];	
}


@end
