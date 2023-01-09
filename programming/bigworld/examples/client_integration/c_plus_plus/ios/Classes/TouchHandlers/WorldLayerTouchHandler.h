#import "TouchHandler.h"

@class WorldLayer;

@interface WorldLayerTouchHandler : NSObject <TouchHandler> {
	WorldLayer * worldLayer;
}

-(id)initWithLayer:(WorldLayer *)worldLayer;
-(BOOL)getPoints:(CGPoint *)points count:(int)count touches:(NSSet*)touches;

@end
