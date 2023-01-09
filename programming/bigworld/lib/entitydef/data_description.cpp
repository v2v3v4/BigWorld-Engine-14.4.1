#include "pch.hpp"

#include "data_description.hpp"
#include "constants.hpp"

#include "cstdmf/base64.h"
#include "cstdmf/debug.hpp"
#include "cstdmf/md5.hpp"
#include "cstdmf/memory_stream.hpp"
#include "cstdmf/watcher.hpp"

#include "network/basictypes.hpp"

#include "resmgr/bwresource.hpp"
#include "resmgr/xml_section.hpp"

// TODO: DataLoDLevel shouldn't be needed by client.
#include "data_lod_level.hpp"

#include "script_data_sink.hpp"
#include "script_data_source.hpp"

DECLARE_DEBUG_COMPONENT2( "DataDescription", 0 )


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "data_description.ipp"
#endif

// -----------------------------------------------------------------------------
// Section: DataDescription
// -----------------------------------------------------------------------------

/**
 *	This is the default constructor.
 */
DataDescription::DataDescription() :
	MemberDescription( NULL ),
	pDataType_( NULL ),
	dataFlags_( 0 ),
	pInitialValue_( NULL ),
	pDefaultSection_( NULL ),
	index_( -1 ),
	localIndex_( -1 ),
	eventStampIndex_( -1 ),
	clientServerFullIndex_( -1 ),
	detailLevel_( DataLoDLevel::NO_LEVEL ),
	databaseLength_( DEFAULT_DATABASE_LENGTH ),
	databaseIndexingType_( DATABASE_INDEXING_NONE ),
	hasSetterCallback_( true ),
	hasNestedSetterCallback_( true ),
	hasSliceSetterCallback_( true )
#ifdef EDITOR_ENABLED
	, editable_( false ),
	pWidgetSection_( NULL )
#endif
{
}


// Anonymous namespace to make static to the file.
namespace
{
struct EntityDataFlagMapping
{
	const char *	name_;
	int				flags_;
};

EntityDataFlagMapping s_entityDataFlagMappings[] =
{
	{ "CELL_PRIVATE",		0 },
	{ "CELL_PUBLIC",		DATA_GHOSTED },
	{ "OTHER_CLIENTS",		DATA_GHOSTED|DATA_OTHER_CLIENT },
	{ "OWN_CLIENT",			DATA_OWN_CLIENT },
	{ "BASE",				DATA_BASE },
	{ "BASE_AND_CLIENT",	DATA_OWN_CLIENT|DATA_BASE },
	{ "CELL_PUBLIC_AND_OWN",DATA_GHOSTED|DATA_OWN_CLIENT },
	{ "ALL_CLIENTS",		DATA_GHOSTED|DATA_OTHER_CLIENT|DATA_OWN_CLIENT },
	{ "EDITOR_ONLY",		DATA_EDITOR_ONLY },
};


/*
 *	This function converts a string to the appropriate flags. It is used when
 *	parsing the properties in the def files.
 *
 *	@param name The string to convert.
 *	@param flags This value is set to the appropriate flag values.
 *	@param parentName The name of the entity type to be used if a warning needs
 *		to be displayed.
 *	@param propName The name of the property that these flags are associated
 *		with.
 *	@return True on success, otherwise false.
 */
bool setEntityDataFlags( const BW::string & name, int & flags,
		const BW::string & parentName, const BW::string & propName )
{
	const int size = sizeof( s_entityDataFlagMappings )/
		sizeof( s_entityDataFlagMappings[0] );

	for (int i = 0; i < size; ++i)
	{
		const EntityDataFlagMapping & mapping = s_entityDataFlagMappings[i];

		if (name == mapping.name_)
		{
			flags = mapping.flags_;
			return true;
		}
	}

	return false;
};


/*
 *	This helper function returns the string that is associated with the input
 *	DataDescription flags.
 */
const char * getEntityDataFlagStr( int flags )
{
	const int size = sizeof( s_entityDataFlagMappings )/
		sizeof( s_entityDataFlagMappings[0] );

	for (int i = 0; i < size; ++i)
	{
		const EntityDataFlagMapping & mapping = s_entityDataFlagMappings[i];

		if (flags == mapping.flags_)
			return mapping.name_;
	}

	return NULL;
}

} // anonymous namespace


/**
 *	This method returns the data flags as a string (hopefully looking like
 * 	the one specified in the defs file).
 */
const char* DataDescription::getDataFlagsAsStr() const
{
	return getEntityDataFlagStr( dataFlags_ & DATA_DISTRIBUTION_FLAGS );
}


/**
 *	This method parses a data description.
 *
 *	@param pSection		The data section to parse.
 *	@param parentName	The name of the parent or an empty string if no parent
 *						exists.
 *	@param componentName
 *						The name of the component to which this field belongs.
 *	@param pNumLatestEventMembers
 *						This is a pointer to the value that stores 
 *						numLatestEventMembers. If not NULL, this is incremented
 *						if this is a SendLatestOnly property.
 *	@param options		Additional parsing options. At the moment, the option
 *						PARSE_IGNORE_FLAGS is used by User Data Objects to
 *						ignore the 'Flags' section instead of failing to parse.
 *	@return				true if successful
 */
bool DataDescription::parse( DataSectionPtr pSection,
		const BW::string & parentName, const BW::string & componentName,
		unsigned int * pNumLatestEventMembers,
		PARSE_OPTIONS options /*= PARSE_DEFAULT*/ )
{
	DataSectionPtr pSubSection;

	name_ = pSection->sectionName();

	hasSetterCallback_ = true;
	hasNestedSetterCallback_ =  true;
	hasSliceSetterCallback_ = true;
	componentName_ = componentName;

	DataSectionPtr typeSection = pSection->openSection( "Type" );

	pDataType_ = DataType::buildDataType( typeSection );

	if (!pDataType_)
	{
		ERROR_MSG( "DataDescription::parse: "
					"Unable to find data type '%s' for %s.%s\n",
				pSection->readString( "Type" ).c_str(),
				parentName.c_str(),
				name_.c_str() );

		return false;
	}

#ifdef EDITOR_ENABLED
	// try to get the default widget, if it's an alias and has one that is
	widget( DataType::findAliasWidget( typeSection->asString() ) );
#endif // EDITOR_ENABLED

	if ( options & PARSE_IGNORE_FLAGS )
	{
		dataFlags_ = 0;
	}
	else
	{
		// dataFlags_ = pSection->readEnum( "Flags", "EntityDataFlags" );
		if (!setEntityDataFlags( pSection->readString( "Flags" ), dataFlags_,
					parentName, name_ ))
		{
			ERROR_MSG( "DataDescription::parse: "
						"Invalid Flags section '%s' for %s\n",
					pSection->readString( "Flags" ).c_str(), name_.c_str() );
			return false;
		}
	}

	if (pSection->readBool( "Persistent", false ))
	{
		dataFlags_ |= DATA_PERSISTENT;
	}

	bool isIndexed = false;
	bool isUnique = false;

	if (pSection->readBool( "Identifier", false ))
	{
		dataFlags_ |= DATA_ID;

		isIndexed = true;
		isUnique = true;
	}

	// We expect Identifier properties to be indexed, so default to true in
	// that case.
	if (pSection->readBool( "Indexed", isIndexed ))
	{
		if ((dataFlags_ & DATA_PERSISTENT) == 0)
		{
			ERROR_MSG( "DataDescription::parse: "
					"%s must be persistent to be indexed\n",
				name_.c_str() );

			return false;
		}

		databaseIndexingType_ = 
			pSection->readBool( "Indexed/Unique", isUnique ) ?
				DATABASE_INDEXING_UNIQUE : DATABASE_INDEXING_NON_UNIQUE;
	}

	// If the data lives on the base, it should not be on the cell.
	MF_ASSERT_DEV( !this->isBaseData() ||
			(!this->isGhostedData() && !this->isOtherClientData()) );

	pSubSection = pSection->findChild( "Default" );

	// If they include a <Default> tag, use it to create the
	// default value. Otherwise, just use the default for
	// that datatype.

#ifdef EDITOR_ENABLED
	editable_ = pSection->readBool( "Editable", false );
#endif

	if (this->isClientServerData())
	{
		// Default for other client data is that it is exposed for replay. Can
		// blacklist. Default for everything else is that it is NOT exposed.

		bool isExposedForReplay = pSection->readBool( "ExposedForReplay",
				this->isOtherClientData() );

		if (isExposedForReplay)
		{
			dataFlags_ |= DATA_REPLAY;
		}
		else
		{
			dataFlags_ &= ~DATA_REPLAY;
		}
	}

	if (pSubSection)
	{
		if (pDataType_->isConst())
		{
			ScriptDataSink sink;
			// TODO: Error handling
			pDataType_->createFromSection( pSubSection, sink );
			pInitialValue_ = sink.finalise();
		}
		else
		{
			pDefaultSection_ = pSubSection;
		}
	}
#ifdef EDITOR_ENABLED
	// The editor always pre loads the default value, so it won't try to make
	// it in the loading thread, which causes issues
	else if (editable())
	{
		ScriptDataSink sink;
		// TODO: Error handling
		pDataType_->getDefaultValue( sink );
		pInitialValue_ = sink.finalise();
	}

//	MF_ASSERT( pInitialValue_ );
#endif

	databaseLength_ = pSection->readInt( "DatabaseLength", databaseLength_ );
	// TODO: If CLASS data type, then DatabaseLength should be default for
	// the individual members of the class if it is not explicitly specified
	// for the member.

	bool isOkay = this->MemberDescription::parse( parentName,
			pSection, /* isForClient */ this->isClientServerData(),
			this->isOtherClientData() ? pNumLatestEventMembers : NULL );

	if (!this->isReliable() && !this->shouldSendLatestOnly())
	{
		WARNING_MSG( "DataDescription::parse: %s.%s is unreliable but does not "
					"have SendLatestOnly set.\n",
				parentName.c_str(), name_.c_str() );
	}

	return isOkay;
}


/**
 *	The copy constructor is needed for this class so that we can handle the
 *	reference counting of pInitialValue_.
 */
DataDescription::DataDescription(const DataDescription & description) :
	BW::MemberDescription( description ),
	pInitialValue_( NULL )
{
	(*this) = description;
}


/**
 *	The destructor for DataDescription handles the reference counting.
 */
DataDescription::~DataDescription()
{
}


/**
 *	This method returns whether or not the input value is the correct type to
 *	set as new value.
 */
bool DataDescription::isCorrectType( ScriptObject pNewValue )
{
	return pDataType_ ? pDataType_->isSameType( pNewValue ) : false;
}


/**
 *	This method adds the value of the appropriate type onto the given output
 *	stream.
 *
 *	@param pValue 				The value to stream.
 *	@param stream 				The output stream.
 *	@param isPersistentOnly 	Whether to stream only persistent data.
 *	@param clientEntityID		The client entity ID, if applicable, used for
 *								log output.
 */
bool DataDescription::addToStream( DataSource & source,
			BinaryOStream & stream, bool isPersistentOnly,
			EntityID clientEntityID /* = NULL_ENTITY_ID */ ) const
{
	BW_GUARD;

	if (this->isClientServerData() && (this->streamSize() < 0))
	{
		bool result = true;

		// Check for oversize messages.
		MemoryOStream lengthStream;
		if (!pDataType_->addToStream( source, lengthStream,
				isPersistentOnly))
		{
			result = false;
		}

		if ((clientEntityID != NULL_ENTITY_ID) &&
				!this->checkForOversizeLength( lengthStream.size(),
					clientEntityID ))
		{
			result = false;
		}

		stream.transfer( lengthStream, lengthStream.size() );
		return result;
	}

	return pDataType_->addToStream( source, stream, isPersistentOnly );
}


/**
 *	This method returns the client-server bandwidth use of this type.
 *
 *	@return  	Fixed-length sized messages are indicated by the return of a
 *				non-negative lengths. For variable-length sized messages, these
 *				are indicated by a negative number, which is the negative of
 *				the preferred number of bytes used in the nominal case.
 */
int DataDescription::streamSize() const
{
	int dataTypeStreamSize = pDataType_->streamSize();

	if (dataTypeStreamSize < 0)
	{
		return -static_cast< int >( this->getVarLenHeaderSize() );
	}

	return dataTypeStreamSize;
}


/**
 *	This method adds this object to the input MD5 object.
 */
void DataDescription::addToMD5( MD5 & md5 ) const
{
	this->MemberDescription::addToMD5( md5 );

	int md5DataFlags = dataFlags_ & DATA_DISTRIBUTION_FLAGS;
	md5.append( &md5DataFlags, sizeof(md5DataFlags) );

	pDataType_->addToMD5( md5 );
}


/**
 *	This method checks whether the defined data is unsafe for client
 */
int DataDescription::clientSafety() const
{
	return pDataType_->clientSafety();
}

/**
 * 	This method returns the initial value of this data item, as a script object.
 */
ScriptObject DataDescription::pInitialValue() const
{
	if (pInitialValue_)
	{
		return pInitialValue_;
	}
	else if (pDefaultSection_)
	{
		ScriptDataSink sink;

		if (pDataType_->createFromSection( pDefaultSection_, sink ))
		{
			return sink.finalise();
		}
	}

	ScriptDataSink sink;
	// TODO: Error handling
	MF_VERIFY( pDataType_->getDefaultValue( sink ) );
	return sink.finalise();
}

/**
 * 	This method returns the default value section of this property.
 */
DataSectionPtr DataDescription::pDefaultSection() const
{
	if (pDataType_->isConst())
	{
		// We didn't store the default section. Re-construct it from the
		// initial value.
		if (pInitialValue_)
		{
			DataSectionPtr pDefaultSection = new XMLSection( "Default" );
			ScriptDataSource source( pInitialValue_ );
			if (!pDataType_->addToSection( source, pDefaultSection ))
			{
				return NULL;
			}
			return pDefaultSection;
		}
		else
		{
			return NULL;
		}
	}
	else
	{
		return pDefaultSection_;
	}
}

#if defined( EDITOR_ENABLED )
/**
 *  This method sets the value "Widget" data section that describes
 *  specifics about how to show the property.
 */
void DataDescription::widget( DataSectionPtr pSection )
{
	pWidgetSection_ = pSection;
}

/**
 *  This method gets the value "Widget" data section that describes
 *  specifics about how to show the property.
 */
DataSectionPtr DataDescription::widget()
{
	return pWidgetSection_;
}
#endif // EDITOR_ENABLED


#if ENABLE_WATCHERS

/**
 *
 */
WatcherPtr DataDescription::pWatcher()
{
	static WatcherPtr watchMe = NULL;

	if (!watchMe)
	{
		watchMe = new DirectoryWatcher();
		DataDescription * pNull = NULL;

		watchMe->addChild( "type",
							new SmartPointerDereferenceWatcher( 
								makeWatcher( &DataType::typeName ) ),
							&pNull->pDataType_);
		watchMe->addChild( "name", 
						   makeWatcher( pNull->name_ ));
		watchMe->addChild( "interfaceName",
			makeWatcher( pNull->interfaceName_) );

		watchMe->addChild( "variableLengthHeaderSize",
			makeWatcher( &DataDescription::getVarLenHeaderSize ) );

		watchMe->addChild( "oversizeWarnLevel",
			makeNonRefWatcher( *(static_cast< MemberDescription * >(pNull)),
				&MemberDescription::oversizeWarnLevelAsString,
				&MemberDescription::watcherOversizeWarnLevelFromString ) );

		watchMe->addChild( "localIndex", 
						   makeWatcher( pNull->localIndex_ ));
		watchMe->addChild( "clientServerFullIndex", 
						   makeWatcher( pNull->clientServerFullIndex_ ));
		watchMe->addChild( "latestEventIndex", 
						   makeWatcher( pNull->latestEventIndex_ ));
		watchMe->addChild( "isReliable", 
						   makeWatcher( pNull->isReliable_ ));
		watchMe->addChild( "index",
							makeWatcher( &DataDescription::index ) );
		watchMe->addChild( "stats", EntityMemberStats::pWatcher(), 
						   &pNull->stats_ );
		watchMe->addChild( "persistent",
							makeWatcher( &DataDescription::isPersistent ) );
	}

	return watchMe;
}

#endif // ENABLE_WATCHERS


#if defined( SCRIPT_PYTHON )
/**
 *
 */
ScriptObject DataDescription::getAttrFrom( const ScriptObject & object ) const
{
	return object.getAttribute( this->name().c_str(), ScriptErrorRetain() );
}


/**
 *
 */
bool DataDescription::setAttrOn( const ScriptObject & object, const ScriptObject & attr ) const
{
	return object.setAttribute( this->name().c_str(), attr, ScriptErrorRetain() );
}
#endif // defined( SCRIPT_PYTHON )


/**
 *
 */
ScriptObject DataDescription::getItemFrom( const ScriptMapping & map ) const
{
	return map.getItem( this->fullName().c_str(), ScriptErrorRetain() );
}


/**
 *
 */
bool DataDescription::insertItemInto( const ScriptMapping & map,
	const ScriptObject & item ) const
{
	return map.setItem( this->fullName().c_str(), item, ScriptErrorRetain() );
}


/**
 *	This method calls the appropriate set_ callback for a property described by
 *	this DataDescription object on the given entity.
 *
 *	@param pEntity 		The entity Python object to find the setter callbacks
 *						on.
 *	@param pOldValue 	The previous value of the change.
 *	@param pChangePath 	The path to the nested change, if a nested change,
 *						otherwise NULL.
 *	@param isSlice	 	For nested changes, the change type, true if sliced,
 *						false for single changes.
 */
void DataDescription::callSetterCallback( ScriptObject pEntity,
		ScriptObject pOldValue,
		ScriptList pChangePath,
		bool isSlice ) const
{
#if defined( SCRIPT_PYTHON )
	const size_t callbackNameMaxSize = sizeof( "setNested_" ) + name_.size();
	char * callbackName = reinterpret_cast< char * >( 
		alloca( callbackNameMaxSize ) );

	bool needsChangePath = false;
	PyObject * pCallback = NULL;

	if (pChangePath.get() != NULL)
	{
		bool * pHasNonSimpleCallback =
			const_cast< bool * >( isSlice ?
				&hasSliceSetterCallback_ : &hasNestedSetterCallback_ );

		if (*pHasNonSimpleCallback)
		{
			strcpy( callbackName, isSlice ? "setSlice_" : "setNested_" );
			strcat( callbackName, name_.c_str() );

			pCallback = PyObject_GetAttrString( pEntity.get(), callbackName );

			if (pCallback != NULL)
			{
				needsChangePath = true;
			}
			else 
			{
				// Fallback to the simple callback if it's a nested change and
				// no nested callbacks exist.
				PyErr_Clear();
				*pHasNonSimpleCallback = false;
			}
		}
	}

	if (pCallback == NULL)
	{
		// Try to call the set_ callback.
		if (!hasSetterCallback_)
		{
			// This callback has already been found to not exist for this
			// property.
			return;
		}

		strcpy( callbackName, "set_" );
		strcat( callbackName, name_.c_str() );
		pCallback = PyObject_GetAttrString( pEntity.get(), callbackName );

		if (pCallback == NULL)
		{
			PyErr_Clear();

			// We clear out the callback name so that we won't look it up next
			// time.
			const_cast< DataDescription * >( this )->hasSetterCallback_ = false;

			return;
		}
	}

	PyObject * args = needsChangePath ? 
			PyTuple_Pack( 2, pChangePath.get(), pOldValue.get() ):
			PyTuple_Pack( 1, pOldValue.get() );

	Script::call( pCallback, args,
		"DataDescription::callSetterCallback: " );

#endif
}


/*
 *	Override from MemberDescription.
 */
BW::string DataDescription::toString() const
{
	BW::ostringstream oss;
	oss << "Property " << interfaceName_ << "." << name_;
	return oss.str();
}


// The following is a hack to make sure that data_types.cpp gets linked.
extern int DATA_TYPES_TOKEN;
int * pDataTypesToken = &DATA_TYPES_TOKEN;

BW_END_NAMESPACE

// data_description.cpp
