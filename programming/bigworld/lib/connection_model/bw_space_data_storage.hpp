#ifndef BW_SPACE_DATA_STORAGE_HPP
#define BW_SPACE_DATA_STORAGE_HPP

#include "network/basictypes.hpp"

BW_BEGIN_NAMESPACE

class SpaceDataMapping;

/**
 *	This is the abstract base class for entity factories.
 */
class BWSpaceDataStorage
{
public:
	/**
	 *	Destructor
	 */
	virtual ~BWSpaceDataStorage() {}

	/**
	 *	This method is used to notify the storage that the space data for the
	 *	given new SpaceID will need to be stored.
	 *
	 *	@return	true unless the space was already known, in which case it may
	 *			have stale data present.
	 */
	bool addNewSpace( SpaceID spaceID )
	{
		return this->addMapping( spaceID );
	}


	/**
	 *	This method is used to notify the storage that the space data for the
	 *	given SpaceID is no longer needed, and should be cleaned up.
	 */
	void deleteSpace( SpaceID spaceID )
	{
		this->clearMapping( spaceID );
	}


	/**
	 *	This method retrieves a SpaceDataMapping for the give SpaceID.
	 *
	 *	@param spaceID			The SpaceID to retrieve the mapping for.
	 *
	 *	@return	A SpaceDataMapping pointer, or NULL if no such mapping has been
	 *			added.
	 */
	SpaceDataMapping * getSpaceMapping( SpaceID spaceID ) const
	{
		return this->getMapping( spaceID );
	}


	/**
	 *	This method notifies this object that all of its managed
	 *	SpaceDataMappings are now invalid, generally because we have been
	 *	disconnected from the server.
	 */
	void clearSpaces()
	{
		return this->clearAllMappings();
	}


private:
	/**
	 *	This virtual method prepares storage for space data for the given
	 *	SpaceID.
	 *
	 *	@param spaceID			The SpaceID to prepare storage for
	 *
	 *	@return	true unless the space was already known, in which case it may
	 *			have stale data present.
	 */
	virtual bool addMapping( SpaceID spaceID ) = 0;

	/**
	 *	This virtual method forgets all the data for the given SpaceID.
	 *
	 *	@param spaceID			The SpaceID to forget the data for
	 */
	virtual void clearMapping( SpaceID spaceID ) = 0;

	/**
	 *	This virtual method retrieves a SpaceDataMapping for the given SpaceID.
	 *
	 *	@param spaceID			The SpaceID to retrieve the mapping for.
	 *
	 *	@return	A SpaceDataMapping pointer, or NULL if no mapping exists for
	 *			this SpaceID.
	 */
	virtual SpaceDataMapping * getMapping( SpaceID spaceID ) const = 0;

	/**
	 *	This virtual method clears all mappings managed through this interface.
	 */
	virtual void clearAllMappings() = 0;
};

BW_END_NAMESPACE

#endif // BW_SPACE_DATA_STORAGE_HPP
