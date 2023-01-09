#import "WorldLayerTouchHandler.h"

@interface WorldLayerOneTouchHandler : WorldLayerTouchHandler {
	BOOL shouldMovePlayer;
}

-(id)initWithLayer:(WorldLayer *)aWorldLayer andTouch:(UITouch *)touch andEvent:(UIEvent *)event;
-(id)initWithLayer:(WorldLayer *)aWorldLayer andTouch:(UITouch *)touch andEvent:(UIEvent *)event shouldMovePlayer:(BOOL)movePlayer;

@end
