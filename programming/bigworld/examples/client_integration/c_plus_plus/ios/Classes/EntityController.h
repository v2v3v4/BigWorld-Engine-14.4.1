#import "network/basictypes.hpp"

namespace BW
{
	class BWEntity;
}

/**
 * This protocol must be implemented by entity controllers. Entity controllers 
 * are used to catch the setting of entity positions and directions by the UI 
 * and from the server.
 */
@protocol EntityController


/** 
 * This method is invoked to update the position and direction for the given 
 * controlled entity.
 *
 * @param entity        The entity to be updated.
 * @param deltaSeconds  The number of seconds since the most recent update.
 *
 * @return				Whether the controller is finished or not.
 */
-(BOOL) updateForEntity: (BW::BWEntity *) pEntity
					 by: (float) deltaSeconds;


/**
 * This method is invoked in response to the position and direction being set.
 *
 * @param position  The entity's new position.
 * @param yaw       The entity's new yaw.
 * @param pitch     The entity's new pitch.
 * @param roll      The entity's new roll.
 */
-(void) onMoveTo: (BW::Vector3) position 
			 yaw: (float) yaw 
		   pitch: (float) pitch 
			roll: (float) roll;


/**
 * This method is invoked in response to the directional yaw being set.
 *
 * @param yaw       The entity's new yaw.
 */
-(void) onTurnTo: (float)yaw;


@end
