#ifndef ENTITY_DESCRIPTION_HPP
#define ENTITY_DESCRIPTION_HPP

#include "base_user_data_object_description.hpp"
#include "data_description.hpp"
#include "data_lod_level.hpp"
#include "entity_method_descriptions.hpp"
#include "method_description.hpp"
#include "volatile_info.hpp"

#include "network/basictypes.hpp"
#include "network/compression_type.hpp"

#include "cstdmf/bw_vector.hpp"


BW_BEGIN_NAMESPACE

enum StreamContentType
{
	BASE_ENTITY_BACKUP,
	BASE_ENTITY_OFFLOAD,
	CELL_CREATION,
	CELL_GHOST_CREATION,
	CELL_ENTITY_BACKUP,
	CELL_ENTITY_OFFLOAD,
	CLIENT_ENTITY_CELL_CREATION,
	CLIENT_PLAYER_CREATION,
	CLIENT_PLAYER_CELL_CREATION,
	CLIENT_ENTITY_LOD_DATA,
	DATABASE_ENTITY
};

class DataSection;
typedef SmartPointer< DataSection > DataSectionPtr;

class MD5;
class AddToStreamVisitor;


/**
 *	This interface is used by EntityDescription::visit. Derive from this
 *	interface if you want to visit a subset of an EntityDescription's
 *	DataDescriptions.
 */
class IDataDescriptionVisitor
{
public:
	virtual ~IDataDescriptionVisitor() {};

	/**
	 *	This function is called to visit a DataDescription.
	 *
	 *	@param	propDesc	Info about the property.
	 *	@return	Return true when successful.
	 */
	virtual bool visit( const DataDescription& propDesc ) = 0;
};


class EntityDistribution
{
public:

	enum Tag
	{
		Unspecified,
		True,
		False,
	};

	EntityDistribution( const DataSectionPtr & pDistribution );

	Tag baseTag() const { return baseTag_; }
	Tag cellTag() const { return cellTag_; }
	Tag clientTag() const { return clientTag_; }

private:
	Tag baseTag_;
	Tag cellTag_;
	Tag clientTag_;
};

/**
 *	This class returns whether an entity type has script for specific entity
 *	components.
 */
class IEntityDistributionDecider
{
public:
	virtual bool canBeOnCell( const BW::string & name, 
		const EntityDistribution & distr ) const = 0;
	virtual bool canBeOnBase( const BW::string & name, 
		const EntityDistribution & distr ) const = 0;
	virtual bool canBeOnClient( const BW::string & name, 
		const EntityDistribution & distr ) const = 0;
};

/**
 *	This class returns whether an entity type has script for specific entity
 *	components.
 */
class HasScriptOrTagDistributionDecider : public IEntityDistributionDecider
{
public:
	virtual bool canBeOnCell( const BW::string & name, 
		const EntityDistribution & distr ) const;
	virtual bool canBeOnBase( const BW::string & name, 
		const EntityDistribution & distr ) const;
	virtual bool canBeOnClient( const BW::string & name, 
		const EntityDistribution & distr ) const;

protected:
	static BW::string getPath( const char * path, const BW::string & name );
	static bool fileExists( const char * path, const BW::string & name );
};


/**
 *	This class is used to describe a type of entity. It describes all properties
 *	and methods of an entity type, as well as other information related to
 *	object instantiation, level-of-detail etc. It is normally created on startup
 *	when the entities.xml file is parsed.
 *
 * 	@ingroup entity
 */
class EntityDescription: public BaseUserDataObjectDescription

{
public:
	EntityDescription();
	~EntityDescription();

	bool	parse( const BW::string & name, const BW::string & componentName,
			const IEntityDistributionDecider *  pDistributionDecider );
	bool	parseService( const BW::string & name,
			const BW::string & componentName,
			const IEntityDistributionDecider * pDistributionDecider );
	void	supersede( MethodDescription::Component component );

	enum DataDomain
	{
		BASE_DATA   = 0x1,
		CLIENT_DATA = 0x2,
		CELL_DATA   = 0x4,
		EXACT_MATCH = 0x8,
		ONLY_OTHER_CLIENT_DATA = 0x10,
		ONLY_PERSISTENT_DATA = 0x20,

		FROM_CELL_TO_CLIENT_DATA = (CELL_DATA | CLIENT_DATA | EXACT_MATCH),
		FROM_BASE_TO_CLIENT_DATA = (BASE_DATA | CLIENT_DATA | EXACT_MATCH)
	};

	bool	addSectionToStream( DataSectionPtr pSection,
				BinaryOStream & stream,
				int dataDomains ) const;

	bool	addSectionToDictionary( DataSectionPtr pSection,
				ScriptObject pDict,
				int dataDomains ) const;

	bool	addDictionaryToStream( const ScriptMapping & map,
				BinaryOStream & stream,
				int dataDomains,
				int32 * pDataSizes = NULL,
				int numDataSizes = 0 ) const;

	bool	addAttributesToStream( const ScriptObject & object,
				BinaryOStream & stream,
				int dataDomains,
				int32 * pDataSizes = NULL,
		   		int numDataSizes = 0 ) const;

	bool	readStreamToDict( BinaryIStream & stream,
				int dataDomains,
				const ScriptMapping & dict ) const;

	bool	readTaggedClientStreamToDict( BinaryIStream & stream,
				const ScriptMapping & dict,
				bool allowOwnClientData = true ) const;

	bool	readStreamToSection( BinaryIStream & stream,
				int dataDomains,
				DataSectionPtr pSection ) const;

	BWENTITY_API
	bool	visit( int dataDomains, IDataDescriptionVisitor & visitor ) const;

	EntityTypeID			index() const;
	void					index( EntityTypeID index );

	EntityTypeID			clientIndex() const;
	void					clientIndex( EntityTypeID index );
	const BW::string&		clientName() const;
	void					setParent( const EntityDescription & parent );

	BWENTITY_API float		appealRadius() const	{ return appealRadius_; }
	BWENTITY_API bool		shouldSendDetailedPosition() const
								{ return shouldSendDetailedPosition_; }
	BWENTITY_API bool		isManualAoI() const	{ return isManualAoI_; }

	bool canBeOnCell() const	{ return canBeOnCell_; }
	bool canBeOnBase() const	{ return canBeOnBase_; }
	bool canBeOnClient() const	{ return canBeOnClient_; }
	bool hasComponents() const	{ return hasComponents_; }

	// note: the client script is found under 'clientName' not 'name'
	bool isClientOnlyType() const
	{ return !canBeOnCell_ && !canBeOnBase_; }

	bool isClientType() const		{ return name_ == clientName_; }

	BWENTITY_API const VolatileInfo &	volatileInfo() const;

	// Compression used by some large messages associated with this entity type
	// sent over the internal network.
	BWENTITY_API BWCompressionType internalNetworkCompressionType() const
								{ return internalNetworkCompressionType_; }

	// Compression used by some large messages associated with this entity type
	// sent to the client.
	BWENTITY_API BWCompressionType externalNetworkCompressionType() const
								{ return externalNetworkCompressionType_; }

	BWENTITY_API unsigned int			clientServerPropertyCount() const;
	BWENTITY_API
	const DataDescription *	clientServerProperty( unsigned int n ) const;

	unsigned int			clientMethodCount() const;

	unsigned int			exposedBaseMethodCount() const
								{ return this->base().exposedSize(); }
	unsigned int			exposedCellMethodCount() const
								{ return this->cell().exposedSize(); }

	INLINE unsigned int		numEventStampedProperties() const;

	INLINE unsigned int		numLatestChangeOnlyMembers() const;

	bool isTempProperty( const BW::string & name ) const;

	const DataLoDLevels &	lodLevels() const { return lodLevels_; }

	const DataDescription *	pIdentifier() const;

	BWENTITY_API const EntityMethodDescriptions & cell() const { return cell_; }
	BWENTITY_API const EntityMethodDescriptions & base() const { return base_; }
	BWENTITY_API
	const EntityMethodDescriptions & client() const { return client_; }

	BWENTITY_API bool isService() const		{ return isService_; }
	BWENTITY_API bool isPersistent() const	{ return isPersistent_; }
	BWENTITY_API bool forceExplicitDBID() const { return forceExplicitDBID_; }

	void					addToMD5( MD5 & md5 ) const;
	void					addPersistentPropertiesToMD5( MD5 & md5 ) const;

	void setExposedMsgIDs(
		int maxClientMethods, const ExposedMethodMessageRange * pClientRange,
		int maxBaseMethods,   const ExposedMethodMessageRange * pBaseRange,
		int maxCellMethods,   const ExposedMethodMessageRange * pCellRange );

	void setActiveOnServerModes( BW::string & activeOnServerModes )
							{ activeOnServerModes_ = activeOnServerModes; }
	const BW::string&		activeOnServerModes() const
							{ return activeOnServerModes_; }
	// ---- Error checking ----
	bool checkMethods( const MethodDescriptionList & methods,
		ScriptObject pClass, bool warnOnMissing = true ) const;
	bool checkExposedMethods( bool isProxy ) const;
	
	bool hasComponent( const BW::string & componentName ) const;

	BWENTITY_API static int getDataDomains( StreamContentType contentType );

	// TODO remove the calls of these two methods from BWCPPComponent and make
	// them private again.
	BWENTITY_API static bool shouldSkipPass( int pass, int dataDomains );

	BWENTITY_API static bool shouldConsiderData(
		int pass, const DataDescription * pDD, int dataDomains );

#if ENABLE_WATCHERS
	static WatcherPtr pWatcher();
#endif

protected:
	bool				parseInterface( DataSectionPtr pSection,
								const char * interfaceName,
								const BW::string & componentName );

	const BW::string	getDefsDir() const;

private:
	bool				parseComponents( DataSectionPtr pComponents,
							const char * interfaceName );
	bool				parseComponent( DataSectionPtr pComponent,
							const char * interfaceName,
							const BW::string & componentName );
	bool				parseMethods( DataSectionPtr pSection,
							const char * interfaceName,
							const BW::string & componentName );
	bool				parseClientMethods( DataSectionPtr pMethods,
							const char * interfaceName,
							const BW::string & componentName );
	bool				parseCellMethods( DataSectionPtr pMethods,
							const char * interfaceName,
							const BW::string & componentName );
	bool				parseBaseMethods( DataSectionPtr pMethods,
							const char * interfaceName,
							const BW::string & componentName );
	bool				parseProperties( DataSectionPtr pProperties,
							const BW::string & componentName );
	bool				parseTempProperties( DataSectionPtr pTempProps,
							const char * interfaceName,
							const BW::string & componentName );

	void				allocateClientServerFullIndexes();

//	bool				parseMethods( DataSectionPtr parseMethods,
//											bool isForServer );
	bool	addToStream( const AddToStreamVisitor & visitor,
				BinaryOStream & stream,
				int dataTypes,
				int32 * pDataSizes = NULL,
		   		int numDataSizes = 0 ) const;

	typedef BW::vector< unsigned int >			PropertyIndices;

	EntityTypeID		index_;
	EntityTypeID		clientIndex_;
	BW::string			clientName_;
	bool				canBeOnCell_;
	bool				canBeOnBase_;
	bool				canBeOnClient_;
	bool				hasComponents_;
	bool				isService_;
	bool				isPersistent_;
	bool				forceExplicitDBID_;
	VolatileInfo 		volatileInfo_;
	BW::string			activeOnServerModes_;

	BWCompressionType	internalNetworkCompressionType_;
	BWCompressionType	externalNetworkCompressionType_;

	/// Stores indices of properties sent between the client and the server in
	/// order of their client/server index.
	PropertyIndices		clientServerProperties_;

	// TODO:PM We should probably combine the property and method maps for
	// efficiency. Only one lookup instead of two.

	/// Stores all methods associated with the cell instances of this entity.
	EntityMethodDescriptions	cell_;

	/// Stores all methods associated with the base instances of this entity.
	EntityMethodDescriptions	base_;

	/// Stores all methods associated with the client instances of this entity.
	EntityMethodDescriptions	client_;

	/// Stores the number of properties that may be time-stamped with the last
	/// time that they changed.
	unsigned int		numEventStampedProperties_;

	/// Stores the number of properties and methods that only store their
	/// latest change in EventHistory instances.
	unsigned int		numLatestChangeOnlyMembers_;

	/// Stores the radius of an entity when viewed by an AoI. Typically this is
	/// zero but may be set for big entities (e.g. large dragons).
	float appealRadius_;

	/// Stores whether volatile position updates for this entity are to be sent
	/// as detailed volatile position updates irrespective of distance.
	bool shouldSendDetailedPosition_;

	/// Whether this entity will be manually added to players' AoI.
	bool isManualAoI_;

	DataLoDLevels		lodLevels_;

	BW::set< BW::string >	tempProperties_;
	
	BW::set< BW::string >	componentNames_;

#ifdef EDITOR_ENABLED
	BW::string			editorModel_;
#endif

};

#ifdef CODE_INLINE
#include "entity_description.ipp"
#endif

BW_END_NAMESPACE

#endif // ENTITY_DESCRIPTION_HPP
