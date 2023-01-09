#import "WorldLayerNoTouchHandler.h"

#import "WorldLayerOneTouchHandler.h"

#import "WorldLayer.h"

@implementation WorldLayerNoTouchHandler

- (BOOL) touchBegan:(UITouch *)touch withEvent:(UIEvent *)event andOwner:(id <TouchHandlerOwner>) owner
{
	[super touchBegan: touch 
			withEvent: event 
			 andOwner: owner];
	
	[owner setTouchHandler: [[[WorldLayerOneTouchHandler alloc]
							  initWithLayer: worldLayer 
								   andTouch:touch 
								   andEvent:event] autorelease]];
	return YES;
}

@end
