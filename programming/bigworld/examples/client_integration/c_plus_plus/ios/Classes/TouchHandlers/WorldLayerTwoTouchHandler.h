#import "WorldLayerTouchHandler.h"

@interface WorldLayerTwoTouchHandler : WorldLayerTouchHandler
{
    CGAffineTransform startInvBasis_;
    CGAffineTransform startTransform_;
}

-(id)initWithLayer:(WorldLayer *)aWorldLayer andTouches:(NSSet *)touches;

@end
