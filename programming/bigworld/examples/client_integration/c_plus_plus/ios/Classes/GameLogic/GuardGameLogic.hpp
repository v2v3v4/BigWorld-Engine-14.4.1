#ifndef GuardGameLogic_HPP
#define GuardGameLogic_HPP

#include "EntityExtensions/GuardExtension.hpp"


/**
 *	This class implements the app-specific logic for the Guard entity.
 */
class GuardGameLogic : public GuardExtension
{
public:
	GuardGameLogic( const Guard * pEntity ) :
			GuardExtension( pEntity )
	{}

	// Client methods

	virtual void shonk(
			const BW::int32 & opponentID,
			const BW::uint8 & oppAction,
			const BW::uint8 & ownAction );

	virtual void handshake(
			const BW::int32 & partnerID );

	virtual void pullUp(
			const BW::int32 & partnerID );

	virtual void pushUp(
			const BW::int32 & partnerID );

	virtual void didGesture(
			const BW::uint8 & actionID );

	virtual void fireWeapon(
			const BW::int32 & shotid,
			const BW::int8 & lock );

	virtual void castSpell(
			const BW::int32 & targetID,
			const BW::Vector3 & hitLocn,
			const BW::uint8 & materialKind );

	virtual void fragged(
			const BW::int32 & shooterID );

	virtual void eat(
			const BW::int32 & item );

	virtual void assail( );

	virtual void salida(
			const BW::int8 & result );

	virtual void chat(
			const BW::string & msg );

	virtual void scanForTargets(
			const float & amplitude,
			const float & period,
			const float & offset );

	virtual void trackTarget(
			const BW::int32 & targetID );

	virtual void resetFilter( );



	// Client property callbacks (optional)


	virtual void set_modeTarget( const BW::int32 & oldValue );

	virtual void set_playerName( const BW::string & oldValue );

	virtual void set_avatarModel( const PackedAvatarModel & oldValue );

	virtual void setNested_avatarModel( const BW::NestedPropertyChange::Path & path, 
			const PackedAvatarModel & oldValue );

	virtual void setSlice_avatarModel( const BW::NestedPropertyChange::Path & path,
			int startIndex, int endIndex, 
			const PackedAvatarModel & oldValue );

	virtual void set_modelNumber( const BW::uint16 & oldValue );

	virtual void set_customColour( const BW::int8 & oldValue );

	virtual void set_rightHand( const BW::int32 & oldValue );

	virtual void set_shoulder( const BW::int32 & oldValue );

	virtual void set_rightHip( const BW::int32 & oldValue );

	virtual void set_leftHip( const BW::int32 & oldValue );

	virtual void set_healthPercent( const BW::int8 & oldValue );

	virtual void set_frags( const BW::int32 & oldValue );

	virtual void set_mode( const BW::int8 & oldValue );

	virtual void set_stance( const BW::int8 & oldValue );

	virtual void set_ownerId( const BW::int32 & oldValue );

	virtual void set_moving( const BW::int8 & oldValue );

	virtual void set_modelScale( const float & oldValue );
};

#endif // GuardGameLogic_HPP
