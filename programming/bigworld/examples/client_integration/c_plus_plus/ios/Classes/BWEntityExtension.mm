#import "BWEntityExtension.h"
#import "CGPointExtension.h"


@implementation BWEntity (Cocos2D)


- (void) setPosition2d:(CGPoint)pos yaw:(float)aYaw
{
	position.x = pos.x;
	position.z = pos.y;
	yaw = aYaw;
}
 
- (CGPoint) position2d
{
	return ccp( position.x, position.z );
}

@end
