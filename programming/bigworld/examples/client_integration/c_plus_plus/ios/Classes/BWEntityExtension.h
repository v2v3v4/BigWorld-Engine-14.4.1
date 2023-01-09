#import <Foundation/Foundation.h>
#import "BWEntity.h"

@interface BWEntity (Cocos2D)

@property (readonly) CGPoint position2d;
-(void) setPosition2d: (CGPoint) pos yaw:(float)aYaw;

@end
