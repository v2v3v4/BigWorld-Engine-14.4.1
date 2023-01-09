#ifndef ENTITY_DESCRIPTION_MAP_HPP
#define ENTITY_DESCRIPTION_MAP_HPP

#include "entity_description.hpp"

#include "cstdmf/md5.hpp"
#include "resmgr/datasection.hpp"

#include "cstdmf/bw_map.hpp"
#include "cstdmf/bw_vector.hpp"


BW_BEGIN_NAMESPACE

class DataSection;
class ExposedPropertyMessageRange;
typedef SmartPointer< DataSection > DataSectionPtr;

/**
 * 	This class parses the entities.xml file, and stores a map of entity
 * 	descriptions.  It provides access to entity descriptions by their name, or
 * 	by index. It also provides fast mapping from name to index.
 *
 * 	@ingroup entity
 */
class EntityDescriptionMap
{
public:
	EntityDescriptionMap();
	~EntityDescriptionMap();

	bool 	parse( DataSectionPtr pSection,
				const ExposedPropertyMessageRange * pClientPropertyMessageRange = NULL,
				const ExposedMethodMessageRange * pClientMessageRange = NULL,
				const ExposedMethodMessageRange * pBaseMessageRange = NULL,
				const ExposedMethodMessageRange * pCellMessageRange = NULL );
	bool	nameToIndex( const BW::string& name, EntityTypeID & index ) const;
	int		size() const;

	const EntityDescription&	entityDescription( EntityTypeID index ) const;

	void addToMD5( MD5 & md5 ) const;
	void addPersistentPropertiesToMD5( MD5 & md5 ) const;

	void clear();
	bool isEntity( const BW::string& name ) const;

	void getNames( BW::vector< BW::string > & names ) const;

	// Only used by editor. Avoid using these. Prefer to add methods to this
	// class instead.
	typedef BW::map< BW::string, EntityTypeID > DescriptionMap;
	DescriptionMap::iterator begin()	{ return map_.begin(); }
	DescriptionMap::iterator end()		{ return map_.end(); }

	DescriptionMap::const_iterator begin() const	{ return map_.begin(); }
	DescriptionMap::const_iterator end() const		{ return map_.end(); }

	unsigned int maxClientServerPropertyCount() const
									{ return maxClientServerPropertyCount_; }
	unsigned int maxExposedClientMethodCount() const
									{ return maxExposedClientMethodCount_; }
	unsigned int maxExposedBaseMethodCount() const
									{ return maxExposedBaseMethodCount_; }
	unsigned int maxExposedCellMethodCount() const
									{ return maxExposedCellMethodCount_; }

	const MD5::Digest digest() const	{ return digest_; }

private:
	typedef BW::vector<EntityDescription> DescriptionVector;

	void setExposedMessageIDs(
				const ExposedMethodMessageRange * pClientMessageRange,
				const ExposedMethodMessageRange * pBaseMessageRange,
				const ExposedMethodMessageRange * pCellMessageRange );

	bool parseInternal( DataSectionPtr pSection,
			const IEntityDistributionDecider & hasScriptDecider,
			DescriptionVector * pServerOnlyDescriptions = NULL );
	bool checkCount( const char * description,
		unsigned int (EntityDescription::*fn)() const,
		int maxEfficient, int maxAllowed ) const;
	bool adjustForClientName();

	bool parseServices( DataSectionPtr pSection, EntityTypeID initialTypeID );

	DescriptionVector 	vector_;

	DescriptionMap 		map_;

	unsigned int maxClientServerPropertyCount_;

	unsigned int maxExposedClientMethodCount_;
	unsigned int maxExposedBaseMethodCount_;
	unsigned int maxExposedCellMethodCount_;

	MD5::Digest digest_;
};

BW_END_NAMESPACE

#endif // ENTITY_DESCRIPTION_MAP_HPP

