#import "ApplicationDelegate.h"
#import "MoveEntityController.h"

#include "connection_model/bw_connection.hpp"
#include "connection_model/bw_entity.hpp"

#include "cocos2d.h"


@implementation MoveEntityController

+ (MoveEntityController *) controllerAtPosition: (BW::Vector3) position 
											yaw: (float) yaw
{
	return [[[MoveEntityController alloc] initWithPosition: position 
													   yaw: yaw] 
			autorelease];
}


-(id) initWithPosition: (BW::Vector3) position yaw: (float)yaw
{
	if ((self = [super init]))
	{
		destinationPosition = ccp(position.x, position.z);
		destinationYaw = yaw;
	}
	
	return self;
}


-(BOOL) updateForEntity: (BW::BWEntity *) pEntity by: (float) deltaSeconds
{
	// Ensure we're don't disappear during this execution if our entity's
	// controller pointer is reset.
	[self retain];
	[self autorelease];
	
	BW::Position3D position = pEntity->position();
	BW::Direction3D direction = pEntity->direction();
	
	CGPoint src = {position.x, position.z};
	CGPoint dst = destinationPosition;
	CGPoint diff = ccpSub( destinationPosition, src );

	float distance = ccpLength( diff );
	static const float SPEED = 15.f;
	float desiredDistance = SPEED * deltaSeconds;

	static const float MIN_DISTANCE = 0.1f;
	if (distance > desiredDistance)
	{
		dst = ccpAdd( src, ccpMult( diff, desiredDistance/distance) );
	}
	else if (distance < MIN_DISTANCE)
	{
		return YES;
	}

	float srcYaw = direction.yaw;
	float turn = destinationYaw - srcYaw;

	turn = fmodf( turn, 2 * M_PI );
	if (turn > M_PI)
	{
		turn -= 2 * M_PI;
	}
	
	if (turn < -M_PI)
	{
		turn += 2 * M_PI;
	}

	static const float TURN_SPEED = 10.f;
	float desiredTurn = TURN_SPEED * deltaSeconds;
	float dstYaw = srcYaw;
	
	if (fabsf( turn ) > desiredTurn)
	{
		if (turn < 0)
		{
			desiredTurn = -desiredTurn;
		}

		dstYaw = fmodf( srcYaw + desiredTurn, 2 * M_PI );
		
		if (dstYaw < 0.f)
		{
			dstYaw += 2 * M_PI;
		}
	}

	direction.yaw = dstYaw;
	
	
	BW::Vector3 nextStepPosition( dst.x, position.y, dst.y );
	BW::Direction3D nextStepDirection = direction;

	ApplicationDelegate * delegate = [ApplicationDelegate sharedDelegate];
	pEntity->onMoveLocally( [delegate connection]->clientTime(),
						   nextStepPosition, BW::NULL_ENTITY_ID,
						   /* is2DPosition */ true, nextStepDirection );
	
	return NO;
}


- (void) onMoveTo: (BW::Vector3) position 
			  yaw: (float) yaw 
			pitch: (float) pitch 
			 roll: (float) roll
{
	destinationPosition = ccp( position.x, position.z );
	destinationYaw = yaw;
}


- (void) onTurnTo: (float) yaw
{
	destinationYaw = yaw;
}


@end
