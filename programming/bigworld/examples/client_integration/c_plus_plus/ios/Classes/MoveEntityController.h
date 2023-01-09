#import "EntityController.h"
#import "network/basictypes.hpp"

@interface MoveEntityController : NSObject <EntityController>
{
	CGPoint destinationPosition;
	float destinationYaw;
}

+ (MoveEntityController *) controllerAtPosition: (BW::Vector3) position
										  yaw: (float) yaw;

- (id) initWithPosition: (BW::Vector3) position
				   yaw: (float) yaw;

@end
