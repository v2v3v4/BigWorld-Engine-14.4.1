#ifndef VideoScreenChangerGameLogic_HPP
#define VideoScreenChangerGameLogic_HPP

#include "EntityExtensions/VideoScreenChangerExtension.hpp"


/**
 *	This class implements the app-specific logic for the VideoScreenChanger entity.
 */
class VideoScreenChangerGameLogic :
		public VideoScreenChangerExtension
{
public:
	VideoScreenChangerGameLogic( const VideoScreenChanger * pEntity ) :
			VideoScreenChangerExtension( pEntity )
	{}

	// Client methods



	// Client property callbacks (optional)

};

#endif // VideoScreenChangerGameLogic_HPP
