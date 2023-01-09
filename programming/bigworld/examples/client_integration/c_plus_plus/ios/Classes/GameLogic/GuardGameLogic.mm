#include "GuardGameLogic.hpp"


// -----------------------------------------------------------------------------
// Section: ClientMethod implementations 
// -----------------------------------------------------------------------------

/**
 *	This method implements the ClientMethod shonk.
 */
void GuardGameLogic::shonk( 
			const BW::int32 & opponentID,
			const BW::uint8 & oppAction,
			const BW::uint8 & ownAction )
{
	// DEBUG_MSG( "GuardGameLogic::set_shonk\n" );
}



/**
 *	This method implements the ClientMethod handshake.
 */
void GuardGameLogic::handshake( 
			const BW::int32 & partnerID )
{
	// DEBUG_MSG( "GuardGameLogic::set_handshake\n" );
}



/**
 *	This method implements the ClientMethod pullUp.
 */
void GuardGameLogic::pullUp( 
			const BW::int32 & partnerID )
{
	// DEBUG_MSG( "GuardGameLogic::set_pullUp\n" );
}



/**
 *	This method implements the ClientMethod pushUp.
 */
void GuardGameLogic::pushUp( 
			const BW::int32 & partnerID )
{
	// DEBUG_MSG( "GuardGameLogic::set_pushUp\n" );
}



/**
 *	This method implements the ClientMethod didGesture.
 */
void GuardGameLogic::didGesture( 
			const BW::uint8 & actionID )
{
	// DEBUG_MSG( "GuardGameLogic::set_didGesture\n" );
}



/**
 *	This method implements the ClientMethod fireWeapon.
 */
void GuardGameLogic::fireWeapon( 
			const BW::int32 & shotid,
			const BW::int8 & lock )
{
	// DEBUG_MSG( "GuardGameLogic::set_fireWeapon\n" );
}



/**
 *	This method implements the ClientMethod castSpell.
 */
void GuardGameLogic::castSpell( 
			const BW::int32 & targetID,
			const BW::Vector3 & hitLocn,
			const BW::uint8 & materialKind )
{
	// DEBUG_MSG( "GuardGameLogic::set_castSpell\n" );
}



/**
 *	This method implements the ClientMethod fragged.
 */
void GuardGameLogic::fragged( 
			const BW::int32 & shooterID )
{
	// DEBUG_MSG( "GuardGameLogic::set_fragged\n" );
}



/**
 *	This method implements the ClientMethod eat.
 */
void GuardGameLogic::eat( 
			const BW::int32 & item )
{
	// DEBUG_MSG( "GuardGameLogic::set_eat\n" );
}



/**
 *	This method implements the ClientMethod assail.
 */
void GuardGameLogic::assail(  )
{
	// DEBUG_MSG( "GuardGameLogic::set_assail\n" );
}



/**
 *	This method implements the ClientMethod salida.
 */
void GuardGameLogic::salida( 
			const BW::int8 & result )
{
	// DEBUG_MSG( "GuardGameLogic::set_salida\n" );
}



/**
 *	This method implements the ClientMethod chat.
 */
void GuardGameLogic::chat( 
			const BW::string & msg )
{
	// DEBUG_MSG( "GuardGameLogic::set_chat\n" );
}



/**
 *	This method implements the ClientMethod scanForTargets.
 */
void GuardGameLogic::scanForTargets( 
			const float & amplitude,
			const float & period,
			const float & offset )
{
	// DEBUG_MSG( "GuardGameLogic::set_scanForTargets\n" );
}



/**
 *	This method implements the ClientMethod trackTarget.
 */
void GuardGameLogic::trackTarget( 
			const BW::int32 & targetID )
{
	// DEBUG_MSG( "GuardGameLogic::set_trackTarget\n" );
}



/**
 *	This method implements the ClientMethod resetFilter.
 */
void GuardGameLogic::resetFilter(  )
{
	// DEBUG_MSG( "GuardGameLogic::set_resetFilter\n" );
}

// -----------------------------------------------------------------------------
// Section: Property setter callbacks
// -----------------------------------------------------------------------------

/**
 *	This method implements the setter callback for the property modeTarget.
 */
void GuardGameLogic::set_modeTarget( const BW::int32 & oldValue )
{
	// DEBUG_MSG( "GuardGameLogic::set_modeTarget\n" );
}

/**
 *	This method implements the setter callback for the property playerName.
 */
void GuardGameLogic::set_playerName( const BW::string & oldValue )
{
	// DEBUG_MSG( "GuardGameLogic::set_playerName\n" );
}

/**
 *	This method implements the setter callback for the property avatarModel.
 */
void GuardGameLogic::set_avatarModel( const PackedAvatarModel & oldValue )
{
	// DEBUG_MSG( "GuardGameLogic::set_avatarModel\n" );
}

/**
 *	This method implements the property setter callback method for the
 *	property avatarModel. 
 *	It is called when a single sub-property element has changed. The
 *	location of the element is described in the given change path.
 */
void GuardGameLogic::setNested_avatarModel( const BW::NestedPropertyChange::Path & path, 
		const PackedAvatarModel & oldValue )
{
	this->set_avatarModel( oldValue );
}

/**
 *	This method implements the property setter callback method for the
 *	property avatarModel. 
 *	It is called when a single sub-property slice has changed. The
 *	location of the ARRAY element is described in the given change path,
 *	and the slice within that element is described in the two given slice
 *	indices. 
 */
void GuardGameLogic::setSlice_avatarModel( const BW::NestedPropertyChange::Path & path,
		int startIndex, int endIndex, 
		const PackedAvatarModel & oldValue )
{
	this->set_avatarModel( oldValue );
}


/**
 *	This method implements the setter callback for the property modelNumber.
 */
void GuardGameLogic::set_modelNumber( const BW::uint16 & oldValue )
{
	// DEBUG_MSG( "GuardGameLogic::set_modelNumber\n" );
}

/**
 *	This method implements the setter callback for the property customColour.
 */
void GuardGameLogic::set_customColour( const BW::int8 & oldValue )
{
	// DEBUG_MSG( "GuardGameLogic::set_customColour\n" );
}

/**
 *	This method implements the setter callback for the property rightHand.
 */
void GuardGameLogic::set_rightHand( const BW::int32 & oldValue )
{
	// DEBUG_MSG( "GuardGameLogic::set_rightHand\n" );
}

/**
 *	This method implements the setter callback for the property shoulder.
 */
void GuardGameLogic::set_shoulder( const BW::int32 & oldValue )
{
	// DEBUG_MSG( "GuardGameLogic::set_shoulder\n" );
}

/**
 *	This method implements the setter callback for the property rightHip.
 */
void GuardGameLogic::set_rightHip( const BW::int32 & oldValue )
{
	// DEBUG_MSG( "GuardGameLogic::set_rightHip\n" );
}

/**
 *	This method implements the setter callback for the property leftHip.
 */
void GuardGameLogic::set_leftHip( const BW::int32 & oldValue )
{
	// DEBUG_MSG( "GuardGameLogic::set_leftHip\n" );
}

/**
 *	This method implements the setter callback for the property healthPercent.
 */
void GuardGameLogic::set_healthPercent( const BW::int8 & oldValue )
{
	// DEBUG_MSG( "GuardGameLogic::set_healthPercent\n" );
}

/**
 *	This method implements the setter callback for the property frags.
 */
void GuardGameLogic::set_frags( const BW::int32 & oldValue )
{
	// DEBUG_MSG( "GuardGameLogic::set_frags\n" );
}

/**
 *	This method implements the setter callback for the property mode.
 */
void GuardGameLogic::set_mode( const BW::int8 & oldValue )
{
	// DEBUG_MSG( "GuardGameLogic::set_mode\n" );
}

/**
 *	This method implements the setter callback for the property stance.
 */
void GuardGameLogic::set_stance( const BW::int8 & oldValue )
{
	// DEBUG_MSG( "GuardGameLogic::set_stance\n" );
}

/**
 *	This method implements the setter callback for the property ownerId.
 */
void GuardGameLogic::set_ownerId( const BW::int32 & oldValue )
{
	// DEBUG_MSG( "GuardGameLogic::set_ownerId\n" );
}

/**
 *	This method implements the setter callback for the property moving.
 */
void GuardGameLogic::set_moving( const BW::int8 & oldValue )
{
	// DEBUG_MSG( "GuardGameLogic::set_moving\n" );
}

/**
 *	This method implements the setter callback for the property modelScale.
 */
void GuardGameLogic::set_modelScale( const float & oldValue )
{
	// DEBUG_MSG( "GuardGameLogic::set_modelScale\n" );
}

// GuardGameLogic.mm
