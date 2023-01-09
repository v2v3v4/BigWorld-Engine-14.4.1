#include "pch.hpp"

#include "data_type.hpp"

#include "constants.hpp"
#include "meta_data_type.hpp"
#include "script_data_sink.hpp"
#include "script_data_source.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/xml_section.hpp"

DECLARE_DEBUG_COMPONENT2( "DataDescription", 0 )


BW_BEGIN_NAMESPACE

extern int SingleTypeDataSinks_Token;
extern int SingleTypeDataSources_Token;
int EntityDef_Token = SingleTypeDataSinks_Token |
	SingleTypeDataSources_Token;

// -----------------------------------------------------------------------------
// Section: DataType static methods
// -----------------------------------------------------------------------------

DataType::SingletonMap * DataType::s_singletonMap_;
DataType::Aliases DataType::s_aliases_;
#ifdef EDITOR_ENABLED
DataType::AliasWidgets DataType::s_aliasWidgets_;
#endif // EDITOR_ENABLED
static bool s_aliasesDone = false;

namespace
{
class Cleanup
{
public:
	~Cleanup()
	{
		// Delete s_singletonMap_
		DataType::clearStaticsForReload();
	}
};

Cleanup s_cleanup;
}


/**
 *	Default implementation for pDefaultSection().
 */
DataSectionPtr DataType::pDefaultSection() const
{
	DataSectionPtr pDefaultSection = new XMLSection( "Default" );

	ScriptDataSink sink;

	if (!this->getDefaultValue( sink ))
	{
		return NULL;
	}

	ScriptObject temp = sink.finalise();
	ScriptDataSource source( temp );

	if (!this->addToSection( source, pDefaultSection ))
	{
		return NULL;
	}
	/* TODO: XMLDataSink
	XMLDataSink sink( pDefaultSection );
	if (!this->getDefaultValue( sink ))
	{
		return NULL;
	}
	*/
	return pDefaultSection;
}


/**
 *	This method adds the DataSource of the appropriate type onto the input
 *	stream.  If isPersistentOnly is true, then the object should stream
 *	only the persistent parts of itself.
 *
 *	@param source	The source to read the value from.
 *	@param stream	The stream to add the value to.
 *	@param isPersistentOnly	Indicates if only persistent data is being added.
 *
 *	@return true if succeeded, false otherwise. In the failure case, the
 *	stream is generally clobbered and should be discarded.
 */
bool DataType::addToStream( DataSource & source, BinaryOStream & stream,
	bool isPersistentOnly ) const
{
	bool result = true;
	std::vector< MemoryOStream * > subStreams;

	for (DataType::const_iterator iter = 
			(isPersistentOnly ? this->beginPersistent() : this->begin());
		iter != (isPersistentOnly ? this->endPersistent() : this->end());
		++iter)
	{
		if (iter->isSubstreamStart())
		{
			subStreams.push_back( new MemoryOStream() );
		}

		BinaryOStream & curStream =
			subStreams.empty() ? stream : *subStreams.back();

		// Note: Taking advantage of addToStream automatically
		// taking care of "isVariableSizedType" and "isNoneAbleType"
		// StreamElements.
		result &= iter->addToStream( source, curStream );

		if (iter->isSubstreamEnd())
		{
			MF_ASSERT( !subStreams.empty() );
			std::auto_ptr<MemoryOStream> pLastStream( subStreams.back() );
			subStreams.pop_back();
			// Second-last stream
			BinaryOStream & prevStream =
				subStreams.empty() ? stream : *subStreams.back();

			int len = pLastStream->remainingLength();

			prevStream.appendString(
				static_cast< const char * >( pLastStream->retrieve( len ) ),
				len );
		}
	}

	if (!subStreams.empty())
	{
		result = false;
		ERROR_MSG( "DataType::addToStream: Failed to pair substream start "
			"and substream end correctly.\n" );
		while (!subStreams.empty())
		{
			std::auto_ptr< MemoryOStream > pExtraStream( subStreams.back() );
			subStreams.pop_back();
		}
	}


	return result;
}


/**
 *	This method outputs an object created from the input stream into
 *	the given DataSink.
 *
 *	@param stream		The stream to read from.
 *	@param sink			The DataSink to output to.
 *  @param isPersistentOnly	If true, then the stream only contains
 * 						persistent properties of the object.
 *
 *	@return				true if succeeded, false otherwise.
 */
bool DataType::createFromStream( BinaryIStream & stream, DataSink & sink,
	bool isPersistentOnly ) const
{
	bool result = true;
	std::vector< MemoryIStream * > subStreams;

	for (DataType::const_iterator iter = 
			(isPersistentOnly ? this->beginPersistent() : this->begin());
		iter != (isPersistentOnly ? this->endPersistent() : this->end());
		++iter)
	{
		if (iter->isSubstreamStart())
		{
			BinaryIStream & outerStream =
				subStreams.empty() ? stream : *subStreams.back();

			int len = outerStream.readPackedInt();
			subStreams.push_back(
				new MemoryIStream( outerStream.retrieve( len ), len ) );
		}

		BinaryIStream & curStream =
			subStreams.empty() ? stream : *subStreams.back();

		// Note: Taking advantage of createFromStream automatically
		// taking care of "isVariableSizedType" and "isNoneAbleType"
		// StreamElements.
		result &= iter->createFromStream( curStream, sink );

		if (iter->isSubstreamEnd())
		{
			MF_ASSERT( !subStreams.empty() );
			std::auto_ptr< MemoryIStream > pInnerStream( subStreams.back() );
			subStreams.pop_back();
			if (pInnerStream->error() || pInnerStream->remainingLength() != 0)
			{
				result = false;
				ERROR_MSG( "DataType::createFromStream: "
						"Iterating substream failed to correctly destream\n" );
			}
		}
	}

	result &= !stream.error();

	if (!subStreams.empty())
	{
		result = false;
		ERROR_MSG( "DataType::createFromStream: Failed to pair substream start "
				"and substream end correctly.\n" );
		while (!subStreams.empty())
		{
			std::auto_ptr< MemoryIStream > pExtraStream( subStreams.back() );
			subStreams.pop_back();
		}
	}

	return result;
}


/**
 *	This factory method returns the DataType derived object associated with the
 *	input data section.
 *
 *	@param pSection	The data section describing the data type.
 *
 *	@return		A pointer to the data type. NULL if an invalid id is entered.
 *
 *	@ingroup entity
 */
DataTypePtr DataType::buildDataType( DataSectionPtr pSection )
{
	if (!pSection)
	{
		WARNING_MSG( "DataType::buildDataType: No <Type> section\n" );
		return NULL;
	}

	if (!s_aliasesDone)
	{
	   	s_aliasesDone = true;
		DataType::initAliases();
	}

	BW::string typeName = pSection->asString();

	// See if it is an alias
	Aliases::iterator found = s_aliases_.find( typeName );
	if (found != s_aliases_.end())
	{
		if ( pSection->findChild( "Default" ) )
		{
			WARNING_MSG( "DataType::buildDataType: New default value for "
					"aliased data type '%s' is ignored. The default value of an"
					" aliased data type can only be overridden by the "
					"default value of an entity property.\n",
					typeName.c_str() );
		}
		return found->second;
	}

	// OK look for the MetaDataType then
	MetaDataType * pMetaType = MetaDataType::find( typeName );
	if (pMetaType == NULL)
	{
		ERROR_MSG( "DataType::buildDataType: "
			"Could not find MetaDataType '%s'\n", typeName.c_str() );
		return NULL;
	}

	// Build a DataType from the contents of the <Type> section
	DataTypePtr pDT = pMetaType->getType( pSection );

	if (!pDT)
	{
		ERROR_MSG( "DataType::buildDataType: "
			"Could not build %s from spec given\n", typeName.c_str() );
		return NULL;
	}

	pDT->setDefaultValue( pSection->findChild( "Default" ) );

	// And return either it or an existing one if this is a dupe
	return DataType::findOrAddType( pDT.get() );
}


/**
 *	This factory method returns the DataType derived object associated with the
 *	input data section.
 *
 *	@param typeXML	An XML string describing the data type.
 *
 *	@return		A pointer to the data type. NULL if an invalid id is entered.
 *
 *	@ingroup entity
 */
/* static */ DataTypePtr DataType::buildDataType( const BW::string & typeXML )
{
	BW::stringstream typeDataStrStream;
	typeDataStrStream << typeXML;
	XMLSectionPtr pXML = XMLSection::createFromStream( "", typeDataStrStream );
	return DataType::buildDataType( pXML );
}


/**
 *	Static method to find an equivalent data type in our set and delete
 *	the given one, or if there is no such data type then add this one.
 *	Note that MethodArgs comparison operator relies on the fact that arguments
 *	of the same time refer to the same instance of DataType.
 */
DataTypePtr DataType::findOrAddType( DataTypePtr pDT )
{
	if (s_singletonMap_ == NULL) s_singletonMap_ = new SingletonMap();

	SingletonMap::iterator found = s_singletonMap_->find(
										SingletonPtr( pDT.get() ) );

	if (found != s_singletonMap_->end())
		return found->pInst_;

	s_singletonMap_->insert( SingletonPtr( pDT.get() ) );
	return pDT;
}


/**
 *	This static method initialises the type aliases from alias.xml.
 *
 *	Note that these are full instances of DataType here, not just alternative
 *	labels for MetaDataTypes.
 */
bool DataType::initAliases()
{
	// Add internal aliases
	MetaDataType::addAlias( "FLOAT32", "FLOAT" );

	DataSectionPtr pAliases =
		BWResource::openSection( EntityDef::Constants::aliasesFile() );

	if (pAliases)
	{
		DataSection::iterator iter;
		for (iter = pAliases->begin(); iter != pAliases->end(); ++iter)
		{
			DataTypePtr pAliasedType = DataType::buildDataType( *iter );

			if (pAliasedType)
			{
				s_aliases_.insert( std::make_pair(
					(*iter)->sectionName().c_str(), pAliasedType ) );

#ifdef EDITOR_ENABLED
				s_aliasWidgets_.insert( std::make_pair(
					(*iter)->sectionName().c_str(),
					(*iter)->findChild( "Widget" ) ) );
#endif // EDITOR_ENABLED
			}
			else
			{
				ERROR_MSG( "DataType::initAliases: Failed to add %s\n",
					(*iter)->sectionName().c_str() );
			}
		}
	}
	else
	{
		WARNING_MSG( "Couldn't open aliases file '%s'\n",
			EntityDef::Constants::aliasesFile() );
	}

	return true;
}


/**
 *	This static method clears our internal statics in preparation for a full
 *	reload of all entitydef stuff.
 */
void DataType::clearStaticsForReload()
{
	// only need to clear the singleton map for UserDataTypes
	DataType::SingletonMap * oldMap = s_singletonMap_;
	s_singletonMap_ = NULL;	// set to NULL first for safety
	bw_safe_delete(oldMap);

	s_aliases_.clear();
	s_aliasesDone = false;

	IF_NOT_MF_ASSERT_DEV( s_singletonMap_ == NULL )
	{
		MF_EXIT( "something is really wrong (NULL is no longer NULL)" );
	}
}


/**
 *	This method returns whether it is safe to ignore assigning a new value.
 */
bool DataType::canIgnoreAssignment( ScriptObject pOldValue,
		ScriptObject pNewValue ) const
{
	return (pOldValue == pNewValue) && this->canIgnoreSelfAssignment();
}


/**
 *	This method returns whether a value has actually changed or not.
 */
bool DataType::hasChanged( ScriptObject pOldValue, ScriptObject pNewValue ) const
{
	return !pOldValue ||
		!this->canIgnoreSelfAssignment() ||
		(pOldValue.compareTo( pNewValue, 
			ScriptErrorPrint( "DataType::hasChanged" ) ) != 0);
}


BW::string DataType::typeName() const
{
	return pMetaDataType_->name();
}


void DataType::callOnEach( void (DataType::*fn)() )
{
	if (s_singletonMap_)
	{
		DataType::SingletonMap::iterator iter = s_singletonMap_->begin();
		const DataType::SingletonMap::iterator endIter = s_singletonMap_->end();

		while (iter != endIter)
		{
			(iter->pInst_->*fn)();

			++iter;
		}
	}
}


// -----------------------------------------------------------------------------
// Section: DataType base class methods
// -----------------------------------------------------------------------------

/**
 *	Destructor
 */
DataType::~DataType()
{
	//SingletonPtr us( this );
	//SingletonMap::iterator found = s_singletonMap_->find( us );
	//if (found->pInst_ == this)
	//	s_singletonMap_->erase( found );

	// TODO: Make this more like code above than code below.
	// Unfortunately, code above doesn't work, because by the time we get here
	// in Windows, our virtual fn table has already been changed back to the
	// base class DataType one, and we can no longer call operator< on ourself.

	if (s_singletonMap_ != NULL)
	{
		for (SingletonMap::iterator it = s_singletonMap_->begin();
			it != s_singletonMap_->end();
			++it)
		{
			if (it->pInst_ == this)
			{
				s_singletonMap_->erase( it );
				break;
			}
		}
	}
}


/**
 *	This method reads this data type from a stream and adds it to a data
 *	section. The default implementation uses createFromStream then
 *	addToSection. DEPRECATED
 *
 *	@param stream		The stream to read from.
 *	@param pSection		The section to add to.
 *	@param isPersistentOnly Indicates whether only persistent data should be
 *		considered.
 *	@return true for success
 */
bool DataType::fromStreamToSection( BinaryIStream & stream,
	DataSectionPtr pSection, bool isPersistentOnly ) const
{
	ScriptDataSink sink;

	if (!this->createFromStream( stream, sink, isPersistentOnly ))
	{
		return false;
	}

	ScriptObject pValue = sink.finalise();
	ScriptDataSource source( pValue );
	return this->addToSection( source, pSection );
}


/**
 *	This method reads this data type from a data section and adds it to
 *	a stream. The default implementation uses createFromSection then
 *	addToStream. DEPRECATED
 *
 *	@param pSection		The section to read from.
 *	@param stream		The stream to write to.
 *	@param isPersistentOnly Indicates whether only persistent data should be
 *		considered.
 *	@return true for success
 */
bool DataType::fromSectionToStream( DataSectionPtr pSection,
	BinaryOStream & stream, bool isPersistentOnly ) const
{
	if (!pSection)
		return false;

	ScriptDataSink sink;

	if (!this->createFromSection( pSection, sink ))
	{
		return false;
	}

	ScriptObject pValue = sink.finalise();
	ScriptDataSource source( pValue );
	return this->addToStream( source, stream, isPersistentOnly );
}


/**
 *	This method first checks the type of the given object. If it fails, then
 *	it returns NULL. If it succeeds then it tells the object who its owner is,
 *	if it needs to know (i.e. if it is mutable and needs to tell its owner when
 *	it is modified). This base class implementation assumes that the object
 *	is const. Finally it returns a pointer to the same object passed in. Some
 *	implementations will need to copy the object if it already has another
 *	owner. In this case the newly-created object is returned instead.
 */
ScriptObject DataType::attach( ScriptObject pObject,
	PropertyOwnerBase * pOwner, int ownerRef )
{
	if (this->isSameType( pObject ))
		return pObject;

	return ScriptObject();
}


/**
 *	This method detaches the given object from its present owner.
 *	This base class implementation does nothing.
 */
void DataType::detach( ScriptObject pObject )
{
}


/**
 *	This method returns the given object, which was created by us, in the
 *	form of a PropertyOwnerBase, i.e. an object which can own other objects.
 *	If the object cannot own other objects, then NULL should be returned.
 *
 *	This base class implementation always returns NULL.
 */
PropertyOwnerBase * DataType::asOwner( ScriptObject pObject ) const
{
	return NULL;
}


// -----------------------------------------------------------------------------
// Section: DataType::StreamElement
// -----------------------------------------------------------------------------
/**
 * Protected constructor
 */
DataType::StreamElement::StreamElement( const DataType & type ) :
	type_( type )
{}


/**
 *	This method provides the number of elements in the container we just started
 *	iterating.
 */
bool DataType::StreamElement::getSize( size_t & rSize ) const
{
	return this->containerSize( rSize );
}


/**
 *	This method provides the DataType of the child element of this element, if
 *	one exists.
 */
bool DataType::StreamElement::getChildType(
	ConstDataTypePtr & rpChildType ) const
{
	return this->childType( rpChildType );
}


/**
 *	This method provides the type-name of the dictionary we just started
 *	iterating.
 */
bool DataType::StreamElement::getStructureName(
	BW::string & rStructureName ) const
{
	return this->dictName( rStructureName );
}


/**
 *	This method provides the name of the current field in the dictionary we are
 *	iterating.
 */
bool DataType::StreamElement::getFieldName( BW::string & rFieldName ) const
{
	return this->fieldName( rFieldName );
}


/**
 *	This method returns true if we have just started iterating a container and
 *	need to tell the iterator how many elements the container contains.
 */
bool DataType::StreamElement::isVariableSizedType() const
{
	return this->isVariableSizedContainer();
}


/**
 *	This method returns true if we have just started iterating a dictionary
 *	and we need to tell the iterator if we have a 'None' value for it.
 */
bool DataType::StreamElement::isNoneAbleType() const
{
	return this->isNoneAble();
}


/**
 *	This method returns true if we need to be supplied with a blob of data
 *	rather than individual elements of this data type
 */
bool DataType::StreamElement::isCustomStreamingType() const
{
	return this->isCustomStreamed();
}


/**
 *	This method returns true if we need to stream this element onwards in a
 *	string, and stream that in the containing stream
 */
bool DataType::StreamElement::isSubstreamStart() const
{
	return this->isStreamAsStringStart();
}


/**
 *	This method returns true if we have reached the end of a series of elements
 *	started by DataType::StreamElement::isSubstreamStart.
 */
bool DataType::StreamElement::isSubstreamEnd() const
{
	return this->isStreamAsStringEnd();
}


/**
 *	This method attempts to add data from the supplied DataSource to the
 *	supplied BinaryOStream in the format specified by this StreamElement.
 */
bool DataType::StreamElement::addToStream( DataSource & source,
	BinaryOStream & stream ) const
{
	return this->fromSourceToStream( source, stream );
}


/**
 *	This method attempts to write data to the supplied DataSink from the
 *	supplied BinaryIStream in the format specified by this StreamElement.
 */
bool DataType::StreamElement::createFromStream( BinaryIStream & stream,
	DataSink & sink ) const
{
	return this->fromStreamToSink( stream, sink );
}

// -----------------------------------------------------------------------------
// Section: DataType::const_iterator
// -----------------------------------------------------------------------------

/**
 *	Private constructor, used by DataType
 */
DataType::const_iterator::const_iterator( const DataType & root,
	bool isPersistentOnly, bool isEnd ) :
root_( root ),
useChild_( false ),
pChildIt_(),
index_( 0 ),
size_( 0 ),
isNone_( false ),
isPersistentOnly_( isPersistentOnly ),
pCurrent_( isEnd ? StreamElementPtr() :
	root_.getStreamElement( index_, size_, isNone_, isPersistentOnly_ )
)
{
	if (pCurrent_.get() == NULL)
	{
		size_ = 0;
		isNone_ = false;
	}

	// Invariant: pCurrent_ is NULL iff *this == root_.end()
	MF_ASSERT( (pCurrent_.get() != NULL) ||
		(!useChild_ && pChildIt_.get() == NULL && size_ == 0 && index_ == 0
			&& !isNone_ ) );
}


/**
 *	Copy constructor. Note that this is a deep copy of the child iterators, and
 *	recreates its pCurrent_
 */
DataType::const_iterator::const_iterator( const const_iterator & other) :
root_( other.root_ ),
useChild_( other.useChild_ ),
pChildIt_( other.pChildIt_.get() ? new const_iterator(*other.pChildIt_) : NULL),
index_( other.index_ ),
size_( other.index_ ),
isNone_( other.isNone_ ),
isPersistentOnly_( other.isPersistentOnly_ ),
pCurrent_( root_.getStreamElement( index_, size_, isNone_, isPersistentOnly_ ) )
{
	// Invariant: pCurrent_ is NULL iff *this == root_.end()
	MF_ASSERT( (pCurrent_.get() != NULL) ||
		(!useChild_ && pChildIt_.get() == NULL && size_ == 0 && index_ == 0
			&& !isNone_ ) );
}


/**
 *	Assignment operator.
 *	This uses the exception-safe copy-and-swap idiom, so it's a deep-copy of the
 *	child iterators via our copy-constructor.
 */
DataType::const_iterator & DataType::const_iterator::operator=(
	 const_iterator other )
{
	swap( *this, other );
	return *this;
}


/**
 *	Equality operator
 */
bool operator==( const DataType::const_iterator & lhs,
	const DataType::const_iterator & rhs )
{
	MF_ASSERT( &lhs.root_ == &rhs.root_ );
	MF_ASSERT( lhs.isPersistentOnly_ == rhs.isPersistentOnly_ );

	// NOTE: pCurrent_ is only checked for NULL-ness, it's value is a
	// consequence of the other member variables. (This makes the
	// copy-constructor easier since we're holding an auto_ptr to it)

	// Check the easy values
	if (lhs.useChild_ != rhs.useChild_ ||
		lhs.index_ != rhs.index_ ||
		lhs.size_ != rhs.size_ ||
		lhs.isNone_ != rhs.isNone_ ||
		(lhs.pCurrent_.get() == NULL) != (rhs.pCurrent_.get() == NULL))
	{
		return false;
	}

	// Does one have a child and one not have a child?
	if ((lhs.pChildIt_.get() == NULL) != (rhs.pChildIt_.get() == NULL))
	{
		return false;
	}

	// If everything else matches, compare the children
	return lhs.pChildIt_.get() == rhs.pChildIt_.get() ||
		*lhs.pChildIt_ == *rhs.pChildIt_;
}


/**
 *	Reference operator
 */
DataType::const_iterator::pointer
	DataType::const_iterator::operator->() const
{
	// You can't dereference DataType::end()
	MF_ASSERT( pCurrent_.get() != NULL );

	if (useChild_ && pChildIt_.get())
	{
		return pChildIt_->operator->();
	}

	return pCurrent_.get();
}


/**
 *	Pre-increment operator
 */
DataType::const_iterator & DataType::const_iterator::operator++()
{
	MF_ASSERT( pCurrent_.get() != NULL );

	if (!useChild_ && pChildIt_.get())
	{
		// This child was provided in the last increment, so start using it
		useChild_ = true;
		return *this;
	}

	if (pChildIt_.get())
	{
		// If we have a child iterator, increment it.
		DataType::const_iterator & childIt = *pChildIt_;
		++childIt;

		if (childIt != ( isPersistentOnly_ ?
			childIt.root_.endPersistent() :
			childIt.root_.end() ) )
		{
			// childIt is still going
			return *this;
		}

		// If it's done, forget it, and continue
		pChildIt_.reset();
		useChild_ = false;
	}

	MF_ASSERT( !useChild_ );
	MF_ASSERT( !pChildIt_.get() );

	// Fetch a new token pair.
	++index_;
	pCurrent_ =
		root_.getStreamElement( index_, size_, isNone_, isPersistentOnly_ );

	ConstDataTypePtr pChildType;

	if (pCurrent_.get() == NULL)
	{
		// If we ended, reset our state
		index_ = 0;
		size_ = 0;
		isNone_ = false;
	}
	else if (pCurrent_->getChildType( pChildType ))
	{
		// We have a child now. Next increment will start using it.
		pChildIt_.reset( new DataType::const_iterator(
			isPersistentOnly_ ?
			pChildType->beginPersistent() :
			pChildType->begin() ) );
	}

	// Invariant: pCurrent_ is NULL iff *this == root_.end()
	MF_ASSERT( (pCurrent_.get() != NULL) ||
		(!useChild_ && pChildIt_.get() == NULL && size_ == 0 && index_ == 0
			&& !isNone_ ) );

	return *this;
}


/**
 *	Post-increment operator
 */
DataType::const_iterator DataType::const_iterator::operator++( int )
{
	const_iterator retVal( *this );
	++*this;
	return retVal;
}


/**
 *	Swap method for two DataType::const_iterators.
 */
void swap( DataType::const_iterator & lhs, DataType::const_iterator & rhs )
{
	MF_ASSERT( &lhs.root_ == &rhs.root_ );
	std::swap( lhs.pCurrent_, rhs.pCurrent_ );
	std::swap( lhs.pChildIt_, rhs.pChildIt_ );
	std::swap( lhs.index_, rhs.index_ );
	std::swap( lhs.size_, rhs.size_ );
}


/**
 *	Set the size of the current variable-size container
 */
void DataType::const_iterator::setSize( size_t size )
{
	if (useChild_ && pChildIt_.get())
	{
		pChildIt_->setSize( size );
		return;
	}

	MF_ASSERT( pCurrent_->isVariableSizedType() );

	size_ = size;
}


/**
 *	Set the noneness of the current none-able container
 */
void DataType::const_iterator::setNone( bool isNone )
{
	if (useChild_ && pChildIt_.get())
	{
		pChildIt_->setNone( isNone );
		return;
	}

	MF_ASSERT( pCurrent_->isNoneAbleType() );

	isNone_ = isNone;
}


BW_END_NAMESPACE

// data_type.cpp
