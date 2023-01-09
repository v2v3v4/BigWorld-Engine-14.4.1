#ifdef _MSC_VER 
#pragma once
#endif

#ifndef MATCH_INFO_HPP
#define MATCH_INFO_HPP


#include "resmgr/forward_declarations.hpp"
#include "network/basictypes.hpp" 
#include "math/mathdef.hpp"


BW_BEGIN_NAMESPACE

/**
 * TODO: to be documented.
 */
struct MatchInfo
{
	MatchInfo( DataSectionPtr sect );

	/**
	 * TODO: to be documented.
	 */
	struct Constraints
	{
		Constraints( bool matchAll );

		void load( DataSectionPtr sect );

		bool satisfies( const Capabilities & caps,
						float speed,
						float yaw,
						float aux1 ) const
		{
			return caps.match( capsOn, capsOff ) &&
				minEntitySpeed <= speed && speed <= maxEntitySpeed &&
				inRange( yaw, minModelYaw, maxModelYaw ) &&
				inRange( aux1, minEntityAux1, maxEntityAux1 );
		}

		float	minEntitySpeed;
		float	maxEntitySpeed;
		float	minEntityAux1;
		float	maxEntityAux1;
		float	minModelYaw;
		float	maxModelYaw;

		Capabilities	capsOn;
		Capabilities	capsOff;

	private:
		static bool inRange( float angle,
			float minAngle, float maxAngle )
		{
				return (angle < minAngle ?
							angle + 2.f*MATH_PI : angle) <= maxAngle;
		}
	};

	Constraints		trigger;
	Constraints		cancel;

	bool	scalePlaybackSpeed;
	bool	feetFollowDirection;
	bool	oneShot;
};

BW_END_NAMESPACE


#endif // MATCH_INFO_HPP

