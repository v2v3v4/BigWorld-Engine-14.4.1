#ifndef IndoorMapInfoGameLogic_HPP
#define IndoorMapInfoGameLogic_HPP

#include "EntityExtensions/IndoorMapInfoExtension.hpp"

/**
 *	This class implements the app-specific logic for the IndoorMapInfo entity.
 */
class IndoorMapInfoGameLogic :
		public IndoorMapInfoExtension
{
public:
	IndoorMapInfoGameLogic( const IndoorMapInfo * pEntity ) :
			IndoorMapInfoExtension( pEntity )
	{}

	// Client methods



	// Client property callbacks (optional)


	virtual void set_radius( const float & oldValue );

	virtual void set_mapName( const BW::string & oldValue );

	virtual void set_worldMapWidth( const float & oldValue );

	virtual void set_worldMapHeight( const float & oldValue );

	virtual void set_worldMapAnchor( const BW::Vector2 & oldValue );

	virtual void setNested_worldMapAnchor( const BW::NestedPropertyChange::Path & path, 
			const BW::Vector2 & oldValue );

	virtual void setSlice_worldMapAnchor( const BW::NestedPropertyChange::Path & path,
			int startIndex, int endIndex, 
			const BW::Vector2 & oldValue );

	virtual void set_minimapRange( const float & oldValue );

	virtual void set_minimapRotate( const BW::uint8 & oldValue );
};

#endif // IndoorMapInfoGameLogic_HPP
