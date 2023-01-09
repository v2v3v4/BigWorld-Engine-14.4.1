#ifndef ENTITY_DELEGATE_HPP
#define ENTITY_DELEGATE_HPP

#include "entitydef/entity_description.hpp"

#include "cstdmf/shared_ptr.hpp"

BW_BEGIN_NAMESPACE

/**
 * This interface represents an EntityDelegate which is a "bridge" or "gateway"
 * object, through an Entity communicates with the additional functionality, 
 * which is implemented elsewhere.
 */
class IEntityDelegate
{
public:
	virtual ~IEntityDelegate() {};

	virtual bool handleMethodCall( const MethodDescription & methodDescription, 
			BinaryIStream & argStream, EntityID sourceID = NULL_ENTITY_ID ) = 0;

	virtual bool updateProperty( BinaryIStream & pInputStream, 
							const DataDescription & pDescription,
							bool isPersistentOnly ) = 0;

	virtual bool getDataField( BinaryOStream & outputStream, 
							const DataDescription & dataDescription,
							bool isPersistentOnly ) = 0;
	
	virtual bool readDataFromStream( BinaryIStream & inputStream, 
							StreamContentType contentType, 
							const EntityDescription & entityDescription ) = 0;
	
	virtual bool writeDataToStream( BinaryOStream & outputStream, 
							StreamContentType contentType, 
							const EntityDescription & entityDescription ) = 0;

	virtual bool getSpatialData(Position3D & rPosition,
							bool & rIsOnGround,
							Direction3D & rDirection) const = 0;

	/**
	 *	This method is called when the entity has been initialized and is ready to update
	 */
	virtual void onEntityInitialised() = 0;

	/**
	 *	This method is called when the entity's position (any of SpaceID,
	 *	VehicleID, position or direction) is changed.
	 *	It's up to the EntityDelegate to retrieve whichever details are relevant.
	 *	TODO: Flags to indicate which details may have changed.
	 */
	virtual void onEntityPositionUpdated() = 0;
	/**
	 *	This method is called when the entity's position changes from being
	 *	locally-controlled (read-write) to being network-controlled (read-only)
	 *	or vice-versa.
	 */
	virtual void onEntityPositionUpdatable(bool isUpdatable) = 0;

	/**
	 *	This method is called on base delegate when the cell entity is destroyed
	 */
	virtual void onLoseCell() = 0;
};

typedef shared_ptr< IEntityDelegate > IEntityDelegatePtr;


/**
 * The factory interface for creating IEntityDelegate instances
 */
class IEntityDelegateFactory
{
public:
	virtual IEntityDelegatePtr createEntityDelegate( 
										const EntityDescription & entityDesc, 
										EntityID entityID, 
										SpaceID spaceID ) = 0;

	/**
	 *	This method is called to create an IEntityDelegate instance for the
	 *	given EntityDescription, EntityID and spaceID. Its initial properties
	 *	should be loaded according to the supplied TemplateID, and it may
	 *	change the provided position and direction details according to that
	 *	template.
	 */
	virtual IEntityDelegatePtr createEntityDelegateFromTemplate(
										const EntityDescription & entityDesc,
										EntityID entityID,
										SpaceID spaceID,
										const BW::string & templateID ) = 0;
};

BW_END_NAMESPACE

#endif // ENTITY_DELEGATE_HPP
