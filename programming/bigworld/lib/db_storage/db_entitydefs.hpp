#ifndef DATABASE_ENTITYDEF_HPP
#define DATABASE_ENTITYDEF_HPP

#include "entitydef/entity_description_map.hpp"
#include "cstdmf/md5.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class represents the entity definitions in DBApp.
 */
class EntityDefs
{
public:
	EntityDefs() :
		entityDescriptionMap_(),
		md5Digest_(),
		persistentPropertiesMD5Digest_()
	{}

	bool init( DataSectionPtr pEntitiesSection );

	const MD5::Digest& getDigest() const 		{	return md5Digest_;	}

	const MD5::Digest& getPersistentPropertiesDigest() const
		{	return persistentPropertiesMD5Digest_;	}

	const DataDescription * getIdentifierProperty( EntityTypeID index ) const
	{
		return this->getEntityDescription( index ).pIdentifier();
	}


	bool isValidEntityType( EntityTypeID typeID ) const
	{
		return (typeID < entityDescriptionMap_.size()) &&
			entityDescriptionMap_.entityDescription( typeID ).isPersistent();
	}

	EntityTypeID getEntityType( const BW::string& typeName ) const
	{
		EntityTypeID typeID = INVALID_ENTITY_TYPE_ID;
		entityDescriptionMap_.nameToIndex( typeName, typeID );
		return typeID;
	}

	size_t getNumEntityTypes() const
	{
		return entityDescriptionMap_.size();
	}

	const EntityDescription& getEntityDescription( EntityTypeID typeID ) const
	{
		return entityDescriptionMap_.entityDescription( typeID );
	}

	//	This function returns the type name of the given property.
	BW::string getPropertyType( EntityTypeID typeID,
		const BW::string& propName ) const;

	void debugDump( int detailLevel ) const;

private:
	bool findIdentifier( const EntityDescription & entityDesc,
		EntityTypeID entityTypeID );

	EntityDescriptionMap	entityDescriptionMap_;
	MD5::Digest				md5Digest_;
	MD5::Digest				persistentPropertiesMD5Digest_;
};

BW_END_NAMESPACE

#endif // DATABASE_ENTITYDEF_HPP
