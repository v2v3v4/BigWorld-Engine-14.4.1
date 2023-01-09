#ifndef VideoScreenGameLogic_HPP
#define VideoScreenGameLogic_HPP

#include "EntityExtensions/VideoScreenExtension.hpp"

/**
 *	This class implements the app-specific logic for the VideoScreen entity.
 */
class VideoScreenGameLogic :
		public VideoScreenExtension
{
public:
	VideoScreenGameLogic( const VideoScreen * pEntity ) :
			VideoScreenExtension( pEntity )
	{}

	// Client methods



	// Client property callbacks (optional)


	virtual void set_textureFeedName( const BW::string & oldValue );

	virtual void set_skipFrames( const BW::uint8 & oldValue );

	virtual void set_triggerRadius( const float & oldValue );

	virtual void set_cameraNode( const BW::UniqueID & oldValue );

	virtual void setNested_cameraNode( const BW::NestedPropertyChange::Path & path, 
			const BW::UniqueID & oldValue );

	virtual void setSlice_cameraNode( const BW::NestedPropertyChange::Path & path,
			int startIndex, int endIndex, 
			const BW::UniqueID & oldValue );

	virtual void set_feedSourcesInput( const BW::string & oldValue );

	virtual void set_feedUpdateDelay( const BW::uint16 & oldValue );
};

#endif // VideoScreenGameLogic_HPP
