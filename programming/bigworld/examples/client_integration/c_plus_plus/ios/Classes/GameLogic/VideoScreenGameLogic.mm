#include "VideoScreenGameLogic.hpp"


// -----------------------------------------------------------------------------
// Section: ClientMethod implementations 
// -----------------------------------------------------------------------------// -----------------------------------------------------------------------------
// Section: Property setter callbacks
// -----------------------------------------------------------------------------

/**
 *	This method implements the setter callback for the property textureFeedName.
 */
void VideoScreenGameLogic::set_textureFeedName( const BW::string & oldValue )
{
	// DEBUG_MSG( "VideoScreenGameLogic::set_textureFeedName\n" );
}

/**
 *	This method implements the setter callback for the property skipFrames.
 */
void VideoScreenGameLogic::set_skipFrames( const BW::uint8 & oldValue )
{
	// DEBUG_MSG( "VideoScreenGameLogic::set_skipFrames\n" );
}

/**
 *	This method implements the setter callback for the property triggerRadius.
 */
void VideoScreenGameLogic::set_triggerRadius( const float & oldValue )
{
	// DEBUG_MSG( "VideoScreenGameLogic::set_triggerRadius\n" );
}

/**
 *	This method implements the setter callback for the property cameraNode.
 */
void VideoScreenGameLogic::set_cameraNode( const BW::UniqueID & oldValue )
{
	// DEBUG_MSG( "VideoScreenGameLogic::set_cameraNode\n" );
}

/**
 *	This method implements the property setter callback method for the
 *	property cameraNode. 
 *	It is called when a single sub-property element has changed. The
 *	location of the element is described in the given change path.
 */
void VideoScreenGameLogic::setNested_cameraNode( const BW::NestedPropertyChange::Path & path, 
		const BW::UniqueID & oldValue )
{
	this->set_cameraNode( oldValue );
}

/**
 *	This method implements the property setter callback method for the
 *	property cameraNode. 
 *	It is called when a single sub-property slice has changed. The
 *	location of the ARRAY element is described in the given change path,
 *	and the slice within that element is described in the two given slice
 *	indices. 
 */
void VideoScreenGameLogic::setSlice_cameraNode( const BW::NestedPropertyChange::Path & path,
		int startIndex, int endIndex, 
		const BW::UniqueID & oldValue )
{
	this->set_cameraNode( oldValue );
}


/**
 *	This method implements the setter callback for the property feedSourcesInput.
 */
void VideoScreenGameLogic::set_feedSourcesInput( const BW::string & oldValue )
{
	// DEBUG_MSG( "VideoScreenGameLogic::set_feedSourcesInput\n" );
}

/**
 *	This method implements the setter callback for the property feedUpdateDelay.
 */
void VideoScreenGameLogic::set_feedUpdateDelay( const BW::uint16 & oldValue )
{
	// DEBUG_MSG( "VideoScreenGameLogic::set_feedUpdateDelay\n" );
}

// VideoScreenGameLogic.mm
