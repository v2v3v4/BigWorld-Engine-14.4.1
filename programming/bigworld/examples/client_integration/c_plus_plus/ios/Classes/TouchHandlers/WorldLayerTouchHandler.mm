#import "WorldLayer.h"
#import "WorldUIModel.h"
#import "WorldLayerTouchHandler.h"

@interface UITouch (Compare)

- (NSComparisonResult)compareAddress:(id)object;

@end

@implementation UITouch (Compare)

- (NSComparisonResult) compareAddress: (id)object
{
	if ((void *)self < (void *)object)
	{
		return NSOrderedAscending;
	}
	else if ((void *)self > (void *)object)
	{
		return NSOrderedDescending;
	}
	else 
	{
		return NSOrderedSame;
	}
}

@end



@implementation WorldLayerTouchHandler

-(id)initWithLayer:(WorldLayer *)aWorldLayer
{
	if ((self = [super init]))
	{
		// Note: Not retaining. The TouchHandler is retained by WorldLayer. Avoid circular retain.
		self->worldLayer = aWorldLayer;
	}
	
	return self;
}

-(void) dealloc
{
	[super dealloc];
}

-(BOOL)getPoints:(CGPoint *)points count:(int)count touches:(NSSet*)touches
{
	NSArray * sortedTouches = [[touches allObjects] sortedArrayUsingSelector:@selector(compareAddress:)];

	int i = 0;
	for (UITouch * touch in sortedTouches)
	{
		if (i >= count)
		{
			NSLog( @"getPoints: Wanted %d. Had %d", count, [touches count] );
			return false;
		}
		
		points[i] = [[CCDirector sharedDirector] convertToGL:[touch locationInView: [touch view]]];
		++i;
	}

	if (i != count)
	{
		NSLog( @"getPoints: i = %d. count = %d\n", i, count );
	}
	
	return (i == count);
}

- (BOOL) touchBegan:(UITouch *)touch withEvent:(UIEvent *)event andOwner:(id <TouchHandlerOwner>) owner
{
	return NO;
}

- (void) touchMoved:(UITouch *)touch withEvent:(UIEvent *)event andOwner:(id <TouchHandlerOwner>) owner
{
}

- (void) touchEnded:(UITouch *)touch withEvent:(UIEvent *)event andOwner:(id <TouchHandlerOwner>) owner
{
}

@end
