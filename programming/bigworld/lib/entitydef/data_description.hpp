#ifndef DATA_DESCRIPTION_HPP
#define DATA_DESCRIPTION_HPP

#include "script/script_object.hpp"

#include "data_type.hpp"
#include "member_description.hpp"
#include "meta_data_type.hpp"
#include "script_data_source.hpp" // Used in the .ipp

#include "bwentity/bwentity_api.hpp"
#include "resmgr/datasection.hpp"


BW_BEGIN_NAMESPACE

class BinaryOStream;
class BinaryIStream;
class MD5;
class Watcher;
typedef SmartPointer<Watcher> WatcherPtr;


/**
 *	This enumeration is used for flags to indicate properties of data associated
 *	with an entity type.
 *
 *	@ingroup entity
 */
enum EntityDataFlags
{
	DATA_GHOSTED		= 0x01,		///< Synchronised to ghost entities.
	DATA_OTHER_CLIENT	= 0x02,		///< Sent to other clients.
	DATA_OWN_CLIENT		= 0x04,		///< Sent to own client.
	DATA_BASE			= 0x08,		///< Sent to the base.
	DATA_CLIENT_ONLY	= 0x10,		///< Static client-side data only.
	DATA_PERSISTENT		= 0x20,		///< Saved to the database.
	DATA_EDITOR_ONLY	= 0x40,		///< Only read and written by editor.
	DATA_ID				= 0X80,		///< Is an indexed column in the database.
	DATA_REPLAY			= 0x100		///< Is exposed for replay.
};

#define DATA_DISTRIBUTION_FLAGS (DATA_GHOSTED | DATA_OTHER_CLIENT | \
								DATA_OWN_CLIENT | DATA_BASE | 		\
								DATA_CLIENT_ONLY | DATA_EDITOR_ONLY)

#define DEFAULT_DATABASE_LENGTH 65535

enum DatabaseIndexingType
{
	DATABASE_INDEXING_NONE,
	DATABASE_INDEXING_UNIQUE,
	DATABASE_INDEXING_NON_UNIQUE
};


class MetaDataType;

#ifdef EDITOR_ENABLED
class GeneralProperty;
class EditorChunkEntity;
class ChunkItem;
#endif



class DataType;
typedef SmartPointer<DataType> DataTypePtr;


/**
 *	This class is used to describe a type of data associated with an entity
 *	class.
 *
 *	@ingroup entity
 */
class DataDescription : public MemberDescription
{
public:
	DataDescription();
	DataDescription( const DataDescription& description );
	virtual ~DataDescription();

	enum PARSE_OPTIONS
	{
		PARSE_DEFAULT,			// Parses all known sections.
		PARSE_IGNORE_FLAGS = 1	// Ignores the 'Flags' section.
	};

	bool parse( DataSectionPtr pSection, const BW::string & parentName,
		const BW::string & componentName,
		unsigned int * pNumLatestEventMembers,
		PARSE_OPTIONS options = PARSE_DEFAULT );

//	DataDescription & operator=( const DataDescription & description );

	bool isCorrectType( ScriptObject pNewValue );

	BWENTITY_API bool addToStream( DataSource & source, BinaryOStream & stream,
		bool isPersistentOnly, EntityID clientEntityID = NULL_ENTITY_ID ) const;
	BWENTITY_API bool createFromStream( BinaryIStream & stream, DataSink & sink,
		bool isPersistentOnly ) const;
	BWENTITY_API int streamSize() const;

	bool addToSection( DataSource & source, DataSectionPtr pSection );
	bool createFromSection( DataSectionPtr pSection, DataSink & sink ) const;

	bool fromStreamToSection( BinaryIStream & stream, DataSectionPtr pSection,
			bool isPersistentOnly ) const;
	bool fromSectionToStream( DataSectionPtr pSection,
			BinaryOStream & stream, bool isPersistentOnly ) const;

	void addToMD5( MD5 & md5 ) const;

	int clientSafety() const;

	/// @name Accessors
	//@{
	ScriptObject pInitialValue() const;

	INLINE bool isGhostedData() const;
	INLINE bool isOtherClientData() const;
	INLINE bool isOwnClientData() const;
	INLINE bool isCellData() const;
	INLINE bool isBaseData() const;
	INLINE bool isClientServerData() const;
	INLINE bool isExposedForReplay() const;
	INLINE bool isPersistent() const;
	INLINE bool isIdentifier() const;
	INLINE bool isIndexed() const;
	INLINE bool isUnique() const;

	DatabaseIndexingType databaseIndexingType() const
		{ return databaseIndexingType_; }

	DataSectionPtr pDefaultSection() const;

	bool isEditorOnly() const;

	BWENTITY_API bool isOfType( EntityDataFlags flags );
	const char* getDataFlagsAsStr() const;

	int index() const;
	void index( int index );

	int localIndex() const					{ return localIndex_; }
	void localIndex( int i )				{ localIndex_ = i; }

	int eventStampIndex() const;
	void eventStampIndex( int index );

	BWENTITY_API
	int clientServerFullIndex() const		{ return clientServerFullIndex_; }
	void clientServerFullIndex( int i )		{ clientServerFullIndex_ = i; }

	int detailLevel() const;
	void detailLevel( int level );

	int databaseLength() const;

	void callSetterCallback( ScriptObject pEntity, 
		ScriptObject pOldValue, ScriptList pChangePath, bool isSlice ) const;

#ifdef EDITOR_ENABLED
	bool editable() const;
	void editable( bool v );

	void widget( DataSectionPtr pSection );
	DataSectionPtr widget();
#endif

	DataType *		dataType() { return pDataType_.get(); }
	const DataType* dataType() const { return pDataType_.get(); }
	//@}

#if ENABLE_WATCHERS
	static WatcherPtr pWatcher();
#endif

	EntityMemberStats & stats() const
	{ return stats_; }

	ScriptObject getAttrFrom( const ScriptObject & object ) const;
	bool setAttrOn( const ScriptObject & object, const ScriptObject & attr ) const;

	// Errors retain from this method
	ScriptObject getItemFrom( const ScriptMapping & map ) const;
	// Errors retain from this method
	bool insertItemInto( const ScriptMapping & map, const ScriptObject & newItem ) const;

	BWENTITY_API
	bool isComponentised() const { return !componentName_.empty(); }

	BWENTITY_API
	const BW::string & componentName() const { return componentName_; }

	BW::string fullName() const;

private:
	virtual BW::string toString() const;

	/* Override from MemberDescription. */
	virtual int getServerToClientStreamSize() const
	{
		return this->streamSize();
	}

	DataTypePtr	pDataType_;
	int			dataFlags_;
	ScriptObject	pInitialValue_;
	DataSectionPtr	pDefaultSection_;

	int			index_;
	int			localIndex_;		// Index into local prop value vector.
	int			eventStampIndex_;	// Index into time-stamp vector.
	int			clientServerFullIndex_;

	int			detailLevel_;

	int			databaseLength_;

	DatabaseIndexingType databaseIndexingType_;

	bool		hasSetterCallback_;
	bool		hasNestedSetterCallback_;
	bool		hasSliceSetterCallback_;

	BW::string	componentName_;

#ifdef EDITOR_ENABLED
	bool		editable_;
	DataSectionPtr pWidgetSection_;
#endif

	// NOTE: If adding data, check the assignment operator.
};


#ifdef CODE_INLINE
#include "data_description.ipp"
#endif

BW_END_NAMESPACE

#endif // DATA_DESCRIPTION_HPP

// data_description.hpp
