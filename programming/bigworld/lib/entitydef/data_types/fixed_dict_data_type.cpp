#include "pch.hpp"

#include "fixed_dict_data_type.hpp"

#if defined( SCRIPT_PYTHON )
#include "entitydef/data_instances/fixed_dict_data_instance.hpp"
#endif

#include "cstdmf/binary_stream.hpp"
#include "cstdmf/md5.hpp"
#include "resmgr/datasection.hpp"

#include "entitydef/data_description.hpp"
#include "entitydef/property_owner.hpp"

#include "entitydef/script_data_sink.hpp"
#include "entitydef/script_data_source.hpp"


BW_BEGIN_NAMESPACE

// Anonymous namespace
namespace {

#if defined SCRIPT_PYTHON
/**
 *	This class is the sole StreamElement of a custom-streamed FIXED_DICT
 */
class FixedDictStreamedStreamElement : public DataType::StreamElement
{
public:
	FixedDictStreamedStreamElement( const FixedDictDataType & type,
		bool isPersistentOnly, bool & isNone ) :
	DataType::StreamElement( type ),
	isNone_( isNone ),
	isPersistentOnly_( isPersistentOnly )
	{
		MF_ASSERT( type.hasCustomClass() );
	};

	/* TODO: Does this makes sense? The unit tests say no...
	bool containerSize( size_t & rSize ) const
	{
		const FixedDictDataType & fixedDictType =
			static_cast< const FixedDictDataType & >( this->getType() );
		// TODO: Only count persistent fields
		rSize = fixedDictType.getNumFields();
		return true;
	}
	*/

	bool dictName( BW::string & rStructureName ) const
	{
		const FixedDictDataType & fixedDictType =
			static_cast< const FixedDictDataType & >( this->getType() );

		rStructureName = fixedDictType.customTypeName();
		return true;
	}

	bool isNoneAble() const
	{
		const FixedDictDataType & fixedDictType =
			static_cast< const FixedDictDataType & >( this->getType() );

		return fixedDictType.allowNone();
	}

	bool isCustomStreamed() const
	{
		return true;
	}

	bool fromSourceToStream( DataSource & source, BinaryOStream & stream ) const
	{
		const FixedDictDataType & fixedDictType =
			static_cast< const FixedDictDataType & >( this->getType() );

		bool result = true;
		if (fixedDictType.allowNone())
		{
			result &= source.readNone( isNone_ );
			if (!result)
			{
				isNone_ = true;
				result = false;
			}

			stream << uint8( isNone_ ? 0 : 1 );

			if (isNone_)
			{
				return result;
			}
		}

		// Let the DataSource work out how to deal with it.
		return
			source.readCustomType( fixedDictType, stream, isPersistentOnly_ );
	}

	bool fromStreamToSink( BinaryIStream & stream, DataSink & sink ) const
	{
		const FixedDictDataType & fixedDictType =
			static_cast< const FixedDictDataType & >( this->getType() );

		bool result = true;
		if (fixedDictType.allowNone())
		{
			uint8 hasValues;
			stream >> hasValues;
			isNone_ = (hasValues == 0);

			if (stream.error())
			{
				ERROR_MSG( "FixedDictStartStreamElement::fromStreamToSink "
						"Missing None/values indicator on stream\n" );
			}

			result &= !stream.error();

			result &= sink.writeNone( isNone_ );

			if (isNone_)
			{
				return result;
			}
		}

		// Let the DataSink work out how to deal with it.
		result &=
			sink.writeCustomType( fixedDictType, stream, isPersistentOnly_ );

		return result;
	}
private:
	bool & isNone_;
	bool isPersistentOnly_;
};

#endif // defined SCRIPT_PYTHON

/**
 *	This class is the first StreamElement of a FIXED_DICT
 */
class FixedDictStartStreamElement : public DataType::StreamElement
{
public:
	FixedDictStartStreamElement( const FixedDictDataType & type,
		bool & isNone ) :
	DataType::StreamElement( type ),
	isNone_( isNone )
	{
#if defined SCRIPT_PYTHON
		MF_ASSERT( !type.hasCustomClass() );
#endif // defined SCRIPT_PYTHON
	};

	bool containerSize( size_t & rSize ) const
	{
		const FixedDictDataType & fixedDictType =
			static_cast< const FixedDictDataType & >( this->getType() );
		// TODO: Only count persistent fields?
		rSize = fixedDictType.getNumFields();
		return true;
	}

	bool dictName( BW::string & rStructureName ) const
	{
		const FixedDictDataType & fixedDictType =
			static_cast< const FixedDictDataType & >( this->getType() );

		rStructureName = fixedDictType.customTypeName();
		return true;
	}

	bool isNoneAble() const
	{
		const FixedDictDataType & fixedDictType =
			static_cast< const FixedDictDataType & >( this->getType() );

		return fixedDictType.allowNone();
	}

	bool fromSourceToStream( DataSource & source, BinaryOStream & stream ) const
	{
		const FixedDictDataType & fixedDictType =
			static_cast< const FixedDictDataType & >( this->getType() );
		bool result = true;
		if (fixedDictType.allowNone())
		{
			result = result && source.readNone( isNone_ );
			if (!result)
			{
				isNone_ = true;
			}

			stream << uint8( isNone_ ? 0 : 1 );

			if (isNone_)
			{
				return result;
			}
		}

		result = result && source.beginDictionary( &fixedDictType );

		return result;
	}

	bool fromStreamToSink( BinaryIStream & stream, DataSink & sink ) const
	{
		const FixedDictDataType & fixedDictType =
			static_cast< const FixedDictDataType & >( this->getType() );

		bool result = true;
		if (fixedDictType.allowNone())
		{
			uint8 hasValues = 0;
			stream >> hasValues;
			isNone_ = (hasValues == 0);

			if (stream.error())
			{
				ERROR_MSG( "FixedDictStartStreamElement::fromStreamToSink "
						"Missing None/values indicator on stream\n" );
			}

			result &= !stream.error();

			result = result && sink.writeNone( isNone_ );

			if (isNone_)
			{
				return result;
			}
		}

		result = result && sink.beginDictionary( &fixedDictType );

		return result;
	}

private:
	bool & isNone_;
};

/**
 *	This class is the StreamElement for each field of a FIXED_DICT.
 */
class FixedDictEnterFieldStreamElement : public DataType::StreamElement
{
public:
	FixedDictEnterFieldStreamElement( const FixedDictDataType & type,
		const DataType & childType, const BW::string & fieldName ) :
	DataType::StreamElement( type ),
	childType_( childType ),
	fieldName_( fieldName )
	{
#if defined SCRIPT_PYTHON
		MF_ASSERT( !type.hasCustomClass() );
#endif // defined SCRIPT_PYTHON
	};

	bool childType( ConstDataTypePtr & rpChildType ) const
	{
		rpChildType = &childType_;
		return true;
	}

	bool fieldName( BW::string & rFieldName ) const
	{
		rFieldName = fieldName_;
		return true;
	}

	bool fromSourceToStream( DataSource & source, BinaryOStream & stream ) const
	{
		return source.enterField( fieldName_ );
	}

	bool fromStreamToSink( BinaryIStream & stream, DataSink & sink ) const
	{
		return sink.enterField( fieldName_ );
	}

private:
	const DataType & childType_;
	const BW::string & fieldName_;
};

/**
 *	This class is the StreamElement after each field of a FIXED_DICT.
 */
class FixedDictLeaveFieldStreamElement : public DataType::StreamElement
{
public:
	FixedDictLeaveFieldStreamElement( const FixedDictDataType & type ) :
	DataType::StreamElement( type )
	{
#if defined SCRIPT_PYTHON
		MF_ASSERT( !type.hasCustomClass() );
#endif // defined SCRIPT_PYTHON
	};

	bool fromSourceToStream( DataSource & source, BinaryOStream & stream ) const
	{
		return source.leaveField();
	}

	bool fromStreamToSink( BinaryIStream & stream, DataSink & sink ) const
	{
		return sink.leaveField();
	}
};

/**
 *	This class is the last StreamElement of a FIXED_DICT
 */
class FixedDictEndStreamElement : public DataType::StreamElement
{
public:
	FixedDictEndStreamElement( const FixedDictDataType & type ) :
	DataType::StreamElement( type )
	{
#if defined SCRIPT_PYTHON
		MF_ASSERT( !type.hasCustomClass() );
#endif // defined SCRIPT_PYTHON
	};

	bool fromSourceToStream( DataSource & source, BinaryOStream & stream ) const
	{
		return true;
	}

	bool fromStreamToSink( BinaryIStream & stream, DataSink & sink ) const
	{
		return true;
	}
};

} // Anonymous namespace

// -----------------------------------------------------------------------------
// Section: ScriptListPropertyOwner
// -----------------------------------------------------------------------------

class ScriptDictPropertyOwner : public PropertyOwnerBase
{
public:
	ScriptDictPropertyOwner() :
		dict_(),
		pFixedDictDataType_( NULL )
	{
	}

	void init( ScriptDict dict, const FixedDictDataType * pFixedDictDataType )
	{
		dict_ = dict;
		pFixedDictDataType_ = pFixedDictDataType;
	}

	virtual int getNumOwnedProperties() const
	{
		return static_cast<int>(pFixedDictDataType_->getNumFields());
	}

	virtual PropertyOwnerBase * getChildPropertyOwner( int childIndex ) const
	{
		BW::string name = pFixedDictDataType_->getFieldName( childIndex );

		ScriptObject childProperty = dict_.getItem( name.c_str(),
			ScriptErrorClear() );

		if (!childProperty)
		{
			return NULL;
		}

		return pFixedDictDataType_->getFieldDataType( childIndex ).asOwner(
				childProperty );
	}

	virtual ScriptObject setOwnedProperty( int childIndex,
			BinaryIStream & data )
	{
		const DataType & dataType =
			pFixedDictDataType_->getFieldDataType( childIndex );
		BW::string propertyName =
			pFixedDictDataType_->getFieldName( childIndex );

		ScriptObject oldValue = dict_.getItem( propertyName.c_str(),
			ScriptErrorClear() );

		ScriptDataSink sink;
		// TODO: Error handling
		dataType.createFromStream( data, sink, /* isPersistentOnly:*/false );

		dict_.setItem( propertyName.c_str(), sink.finalise(),
			ScriptErrorPrint( "ScriptDictPropertyOwner::setOwnedProperty:" ) );

		return oldValue;
	}

	virtual ScriptObject getPyIndex( int index ) const
	{
		return ScriptObject::createFrom(
				pFixedDictDataType_->getFieldName( index ) );
	}

private:
	ScriptDict dict_;
	const FixedDictDataType * pFixedDictDataType_;
};


// -----------------------------------------------------------------------------
// Section: FixedDictDataType
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
FixedDictDataType::FixedDictDataType( MetaDataType * pMeta, Fields & fields,
		bool allowNone ) :
	DataType( pMeta, /*isConst:*/false ),
	allowNone_( allowNone ),
	fields_(),
	fieldMap_(),
	customTypeName_(),
	moduleName_(),
	instanceName_(),
	pDefaultSection_(),
	isCustomClassImplInited_( true ),
	pImplementor_(),
	pGetDictFromObjFn_(),
	pCreateObjFromDictFn_(),
	pIsSameTypeFn_(),
	pAddToStreamFn_(),
	pCreateFromStreamFn_(),
	streamSize_( allowNone_ ? -1 : 0 ),
	clientSafety_( CLIENT_SAFE )
{
	fields_.swap( fields );

	MF_ASSERT( fields_.size() <= INT_MAX );
	int i = 0;
	for (Fields::const_iterator it = fields_.begin();
		it != fields_.end(); ++it, ++i)
	{
		fieldMap_[it->name_] = i;
		if (streamSize_ != -1)
		{
			const int fieldSize = it->type_->streamSize();
			if ( fieldSize == -1 )
			{
				streamSize_ = -1;
			}
			else
			{
				streamSize_ += fieldSize;
			}
		}

		clientSafety_ |= it->type_->clientSafety();
	}
}


/**
 *	Virtual destructor
 */
FixedDictDataType::~FixedDictDataType()
{
#if defined( SCRIPT_PYTHON )
	// If this assertion is hit, it means that this data type is being
	// destructed after the call to Script::fini. Make sure that the
	// EntityDescription is destructed or EntityDescription::clear called on it
	// before Script::fini.
	MF_ASSERT_DEV( !pImplementor_ || !Script::isFinalised() );
#endif
}


#if defined( SCRIPT_PYTHON )
/**
 *	This method sets the user-defined instance that will be responsible for
 * 	converting our dictionary to/from their custom Python class.
 */
void FixedDictDataType::setCustomClassImplementor(
		const BW::string& moduleName, const BW::string& instanceName )
{
	moduleName_ = moduleName;
	instanceName_ = instanceName;

	isCustomClassImplInited_ = false;

	this->setCustomClassFunctions( true );
}

/**
 *	This method sets our internal function pointers to the user-defined instance
 */
void FixedDictDataType::setCustomClassFunctions( bool ignoreFailure )
{
	isCustomClassImplInited_ = true;

	PyObjectPtr pModule(
			PyImport_ImportModule( const_cast<char *>( moduleName_.c_str() ) ),
			PyObjectPtr::STEAL_REFERENCE );

	if (pModule)
	{
		pImplementor_ = PyObjectPtr(
							PyObject_GetAttrString( pModule.get(),
								const_cast<char *>( instanceName_.c_str() ) ),
							PyObjectPtr::STEAL_REFERENCE );

		if (pImplementor_)
		{
			pGetDictFromObjFn_ =
				PyObjectPtr( PyObject_GetAttrString( pImplementor_.get(),
													"getDictFromObj" ),
							PyObjectPtr::STEAL_REFERENCE );
			if (!pGetDictFromObjFn_)
			{
				PyErr_Clear();
				ERROR_MSG( "FixedDictDataType::setCustomClassImplementor: "
							"%s.%s is missing method getDictFromObj\n",
							moduleName_.c_str(), instanceName_.c_str() );
			}

			pCreateObjFromDictFn_ =
				PyObjectPtr( PyObject_GetAttrString( pImplementor_.get(),
													"createObjFromDict" ),
							PyObjectPtr::STEAL_REFERENCE );
			if (!pCreateObjFromDictFn_)
			{
				PyErr_Clear();
				ERROR_MSG( "FixedDictDataType::setCustomClassImplementor: "
							"%s.%s is missing method createObjFromDict\n",
							moduleName_.c_str(), instanceName_.c_str() );
			}

			pIsSameTypeFn_ =
				PyObjectPtr( PyObject_GetAttrString( pImplementor_.get(),
													"isSameType" ),
							PyObjectPtr::STEAL_REFERENCE );
			if (!pIsSameTypeFn_)
			{
				PyErr_Clear();
				WARNING_MSG( "FixedDictDataType::setCustomClassImplementor: "
							"%s.%s is missing method isSameType\n",
							moduleName_.c_str(), instanceName_.c_str() );
			}

			pAddToStreamFn_ =
				PyObjectPtr( PyObject_GetAttrString( pImplementor_.get(),
													"addToStream" ),
							PyObjectPtr::STEAL_REFERENCE );
			if (!pAddToStreamFn_) PyErr_Clear();

			pCreateFromStreamFn_ =
				PyObjectPtr( PyObject_GetAttrString( pImplementor_.get(),
													"createFromStream" ),
							PyObjectPtr::STEAL_REFERENCE );
			if (!pCreateFromStreamFn_) PyErr_Clear();

			if ((pAddToStreamFn_ && !pCreateFromStreamFn_) ||
				(!pAddToStreamFn_ && pCreateFromStreamFn_))
			{
				ERROR_MSG( "FixedDictDataType::setCustomClassImplementor: "
							"%s.%s must implement both addToStream and "
							"createFromStream, or implement neither\n",
							moduleName_.c_str(), instanceName_.c_str() );
				pAddToStreamFn_ = NULL;
				pCreateFromStreamFn_ = NULL;
			}
		}
		else
		{
			ERROR_MSG( "FixedDictDataType::setCustomClassImplementor: "
						"Unable to import %s from %s\n",
						instanceName_.c_str(), moduleName_.c_str() );
			PyErr_Print();
		}
	}
	else
	{
		if (ignoreFailure)
		{
			isCustomClassImplInited_ = false;
			PyErr_Clear();
		}
		else
		{
			ERROR_MSG( "FixedDictDataType::setCustomClassImplementor: Unable to "
						"import %s\n", moduleName_.c_str() );
			PyErr_Print();
		}
	}
}
#endif


/**
 *	Overrides the DataType method.
 *
 *	@see DataType::isSameType
 */
bool FixedDictDataType::isSameType( ScriptObject pValue )
{
#if defined( SCRIPT_PYTHON )
	this->initCustomClassImplOnDemand();
#endif

	bool isSame = true;

	if (allowNone_ && pValue.isNone())
	{
		// isSame = true;
	}
#if defined( SCRIPT_PYTHON )
	else if (this->hasCustomClass())
	{
		if (this->hasCustomIsSameType())
		{
			isSame = this->isSameTypeCustom( pValue );
		}
		else
		{
			// Create a PyFixedDictDataInstance using the user provided
			// getDictFromObj() function. If it succeeds, we'll assume that it is
			// an acceptable type.
			ScriptObject pObj = this->createInstanceFromCustomClass( pValue );
			isSame = pObj != NULL;
		}
	}
	// check if an instance of our special Python class
	else if (PyFixedDictDataInstance::isSameType( pValue, *this ))
	{
		// isSame = true;
	}
#endif
	else if (ScriptDict::check( pValue ))
	{
		ScriptDict dict( pValue );

		// Check that it has the keys that we need.
		for (Fields::const_iterator i = fields_.begin();
				(i != fields_.end()) && isSame; ++i)
		{
			ScriptObject pKeyedValue = dict.getItem( i->name_.c_str(),
				ScriptErrorClear() );

			isSame = pKeyedValue ? 
					i->type_->isSameType( pKeyedValue ) : false;
		}
	}
	else
	{
		isSame = false;
	}

	return isSame;
}

/**
 *	Overrides the DataType method.
 *
 *	@see DataType::reloadScript
 */
void FixedDictDataType::reloadScript()
{
#if defined( SCRIPT_PYTHON )
	// __kyl__ (12/1/2007) If we haven't inited our custom class functions,
	// then we don't need to reload. this->hasCustomClass() is conveniently
	// false if we either haven't inited or if we don't have a custom class.
	// initCustomClassImplOnDemand();
	// DEBUG_MSG( "FixedDictDataType::reloadScript: this=%p\n", this );

	if (this->hasCustomClass())
	{
		pImplementor_ = NULL;
		pGetDictFromObjFn_ = NULL;
		pCreateObjFromDictFn_ = NULL;
		pIsSameTypeFn_ = NULL;
		pAddToStreamFn_ = NULL;
		pCreateFromStreamFn_ = NULL;

		this->setCustomClassFunctions( false );
	}
#endif
}

/**
 *	Overrides the DataType method.
 *
 *	@see DataType::clearScript
 */
void FixedDictDataType::clearScript()
{
	pImplementor_ = NULL;
	pGetDictFromObjFn_ = NULL;
	pCreateObjFromDictFn_ = NULL;
	pIsSameTypeFn_ = NULL;
	pAddToStreamFn_ = NULL;
	pCreateFromStreamFn_ = NULL;
}

/**
 *	This method sets the default value for this type.
 *
 *	@see DataType::setDefaultValue
 */
void FixedDictDataType::setDefaultValue( DataSectionPtr pSection )
{
	pDefaultSection_ = pSection;
}

/**
 *	Overrides the DataType method.
 *
 *	@see DataType::getDefaultValue
 */
bool FixedDictDataType::getDefaultValue( DataSink & output ) const
{
#if defined( SCRIPT_PYTHON )
	const_cast<FixedDictDataType*>(this)->initCustomClassImplOnDemand();
#endif

	if (pDefaultSection_)
	{
		return this->createFromSection( pDefaultSection_, output );
	}
	else if (allowNone_)
	{
		return output.writeNone( /* isNone */ true );
	}

	// TODO: Better than this.
	ScriptObject pDefault = this->createDefaultInstance();
#if defined( SCRIPT_PYTHON )
	if (this->hasCustomClass())
		pDefault = this->createCustomClassFromInstance(
			static_cast<PyFixedDictDataInstance*>(pDefault.get()) );
#endif

	// Eww...
	ScriptDataSink & scriptOutput = static_cast< ScriptDataSink & >( output );
	return scriptOutput.write( pDefault );
}


/**
 *
 */
DataSectionPtr FixedDictDataType::pDefaultSection() const
{
	return pDefaultSection_;
}


/*
 *	Overrides the DataType method.
 *
 *	@see DataType::streamSize
 */
int FixedDictDataType::streamSize() const
{
#if defined( SCRIPT_PYTHON )
	if (hasCustomAddToStream())
	{
		// Don't let bad Python streaming code break anything
		// except the current packet
		return -1;
	}
#endif
	return streamSize_;
}

/**
 *	Overrides the DataType method.
 *
 *	@see DataType::addToSection
 */
bool FixedDictDataType::addToSection( DataSource & source,
		DataSectionPtr pSection ) const
{
	if (allowNone_)
	{
		bool isNone;
		if (!source.readNone( isNone ))
		{
			return false;
		}

		if (isNone)
		{
			// leave as empty section
			return true;
		}
	}

#if defined( SCRIPT_PYTHON )
	const_cast<FixedDictDataType*>(this)->initCustomClassImplOnDemand();

	if (this->hasCustomClass())
	{
		// Let the DataSource work out how to deal with it.
		return source.readCustomType( *this, *pSection );
	}
#endif // defined( SCRIPT_PYTHON )

	if (!source.beginDictionary( this ))
	{
		return false;
	}

	for (Fields::const_iterator iter = fields_.begin(); iter != fields_.end();
		++iter )
	{
		if (!source.enterField( iter->name_ ) ||
			!iter->type_->addToSection( source,
				pSection->newSection( iter->name_ ) ) ||
			!source.leaveField())
		{
			return false;
		}
	}

	return true;
}

/**
 *	Overrides the DataType method.
 *
 *	@see DataType::createFromSection
 */
bool FixedDictDataType::createFromSection( DataSectionPtr pSection,
		DataSink & sink ) const
{
	if (!pSection)
	{
		ERROR_MSG( "FixedDictDataType::createFromSection: Section is NULL.\n" );
		return ScriptObject();
	}

	if (allowNone_)
	{
		bool isNone = (pSection->countChildren() == 0);

		if (!sink.writeNone( isNone ))
		{
			return false;
		}

		if (isNone)
		{
			return true;
		}
	}

#if defined( SCRIPT_PYTHON )
	const_cast<FixedDictDataType*>(this)->initCustomClassImplOnDemand();

	if (this->hasCustomClass())
	{
		// Let the DataSink work out how to deal with it.
		return sink.writeCustomType( *this, *pSection );
	}
#endif // defined( SCRIPT_PYTHON )

	if (!sink.beginDictionary( this ))
	{
		return false;
	}

	for (Fields::const_iterator it = fields_.begin();
		it != fields_.end(); ++it)
	{
		if (!sink.enterField( it->name_ ))
		{
			return false;
		}

		DataSectionPtr pChildSection = pSection->openSection( it->name_ );
		if (pChildSection)
		{
			if (!it->type_->createFromSection( pChildSection, sink ))
			{
				return false;
			}
		}
		else
		{
			if (!it->createDefaultValue( pDefaultSection_, sink ))
			{
				return false;
			}
		}

		if (!sink.leaveField())
		{
			return false;
		}
	}

	return true;
}

/**
 *	Overrides the DataType method.
 *
 *	@see DataType::addToMD5
 */
void FixedDictDataType::addToMD5( MD5 & md5 ) const
{
	md5.append( "FixedDict", sizeof( "FixedDict" ) );
#if defined( SCRIPT_PYTHON )
	if (!isCustomClassImplInited_ || this->hasCustomClass())
	{
		md5.append( moduleName_.c_str(), static_cast<int>(moduleName_.size()) );
		md5.append( instanceName_.c_str(), static_cast<int>(instanceName_.size()) );
	}
#endif // SCRIPT_PYTHON

	md5.append( &allowNone_, sizeof( allowNone_ ) );
	for (Fields::const_iterator it = fields_.begin(); it != fields_.end(); ++it)
	{
		md5.append( it->name_.data(), static_cast<int>(it->name_.size()) );
		it->type_->addToMD5( md5 );
	}
}


/**
 *	Overrides the DataType method.
 *
 *	@see DataType::operator<
 */
bool FixedDictDataType::operator<( const DataType & other ) const
{
	if (this->DataType::operator<( other )) return true;
	if (other.DataType::operator<( *this )) return false;

	// ok, equal metas, so downcast other and compare with us
	FixedDictDataType& otherCl= (FixedDictDataType&)other;

	if (customTypeName_ < otherCl.customTypeName_) return true;
	if (otherCl.customTypeName_ < customTypeName_) return false;

	if (moduleName_ < otherCl.moduleName_) return true;
	if (otherCl.moduleName_ < moduleName_) return false;

	if (instanceName_ < otherCl.instanceName_) return true;
	if (otherCl.instanceName_ < instanceName_) return false;

	if (allowNone_ != otherCl.allowNone_)
		return !allowNone_;	// we are less if our allowNone is false

	if (fields_.size() < otherCl.fields_.size()) return true;
	if (otherCl.fields_.size() < fields_.size()) return false;

	for (uint i = 0; i < fields_.size(); ++i)
	{
		int res = fields_[i].name_.compare( otherCl.fields_[i].name_ );
		if (res < 0) return true;
		if (res > 0) return false;

		if ((*fields_[i].type_) < (*otherCl.fields_[i].type_)) return true;
		if ((*otherCl.fields_[i].type_) < (*fields_[i].type_)) return false;
	}

	if (otherCl.pDefaultSection_)
		return otherCl.pDefaultSection_->compare( pDefaultSection_ ) > 0;
	// else we are equal or greater than other.
	return false;
}

/**
 *	Overrides the DataType method.
 *
 *	@see DataType::typeName
 */
BW::string FixedDictDataType::typeName() const
{
	// If you change the format, please update ProcessDefs/GenerateCPlusPlus.

	BW::string acc = this->DataType::typeName();

	if (!customTypeName_.empty())
	{
		acc += " name:" + customTypeName_;
	}

#if defined( SCRIPT_PYTHON )
	if (!isCustomClassImplInited_ || this->hasCustomClass())
		acc += " [Implemented by " + moduleName_ + "." + instanceName_ + "]";
#endif

	if (allowNone_) acc += " [None Allowed]";
	for (Fields::const_iterator it = fields_.begin(); it != fields_.end(); ++it)
	{
		acc += (it == fields_.begin()) ? " props " : ", ";
		acc += it->name_ + ":(" + it->type_->typeName() + ")";
	}
	return acc;
}

/**
 *	Overrides the DataType method.
 *
 *	@see DataType::attach
 */
ScriptObject FixedDictDataType::attach( ScriptObject pObject,
	PropertyOwnerBase * pOwner, int ownerRef )
{
#if defined( SCRIPT_PYTHON )
	// if it's None and that's ok, just return that
	if (allowNone_ && pObject.isNone())
	{
		return ScriptObject::none();
	}

	initCustomClassImplOnDemand();

	if (this->hasCustomClass())
	{
		// First part of isSameType() check
		if (this->hasCustomIsSameType() && !this->isSameTypeCustom( pObject ))
		{
			ERROR_MSG( "FixedDictDataType::attach: "
					"Trying to attach an invalid custom type\n" );
			return ScriptObject();
		}

		// See if they are referencing our PyFixedDictDataInstance
		ScriptObject pDict = this->getDictFromCustomClass( pObject );

		// Second part of isSameType() check
		if (PyFixedDictDataInstance::isSameType( pDict, *this ))
		{
			// Yay! isSameType() == true
			PyFixedDictDataInstancePtr pInst( pDict );
			if (pInst->hasOwner())
			{
				// Create copy
				pInst = PyFixedDictDataInstancePtr(
					new PyFixedDictDataInstance( this, *pInst ),
					PyFixedDictDataInstancePtr::FROM_NEW_REFERENCE );
				pInst->setOwner( pOwner, ownerRef );
				return this->createCustomClassFromInstance( pInst.get() );
			}
			else
			{
				pInst->setOwner( pOwner, ownerRef );
				return pObject;
			}
		}
		else	// not referencing PyFixedDictDataInstance
		{
			// Third part of isSameType() check, for custom class without
			// isSameType() method and doesn't reference PyFixedDictDataInstance
			if (this->hasCustomIsSameType() ||
				this->createInstanceFromMappingObj( pDict ))
			{
				return pObject;
			}
			else
			{
				return ScriptObject();
			}
		}
	}

	PyFixedDictDataInstancePtr pInst;

	// it's easy if it's the right python + entitydef type
	if (PyFixedDictDataInstance::isSameType( pObject, *this ))
	{
		pInst = pObject;
		if (pInst->hasOwner())
		{
			// Create copy
			pInst = PyFixedDictDataInstancePtr(
				new PyFixedDictDataInstance( this, *pInst ),
				PyFixedDictDataInstancePtr::FROM_NEW_REFERENCE );
			// note: up to caller to check that prop isn't being set back
		}
	}
	else
	{
		pInst = this->createInstanceFromMappingObj( pObject );
	}

	if (pInst)
	{
		pInst->setOwner( pOwner, ownerRef );
	}

	return pInst;
#else
	return pObject;
#endif
}

/**
 *	Overrides the DataType method.
 *
 *	@see DataType::detach
 */
void FixedDictDataType::detach( ScriptObject pObject )
{
#if defined( SCRIPT_PYTHON )
	initCustomClassImplOnDemand();

	if (pObject.isNone())
	{
		// do nothing
	}
	else if (this->hasCustomClass())
	{
		// TODO: Call isSameType()? A bit nasty to expect user code to handle
		// unexpected types but do we want to incur overhead?
		// See if they are referencing our PyFixedDictDataInstance
		ScriptObject pDict = this->getDictFromCustomClass( pObject );

		if (PyFixedDictDataInstancePtr::check( pDict ))
		{
			PyFixedDictDataInstancePtr( pDict )->disowned();
		}
	}
	else
	{
		// NOTE: After finalisation hasCustomClass() will always return
		// false so custom objects will end up here. We can't call
		// getDictFromCustomClass() so if the custom object is referencing our
		// PyFixedDictDataInstance its pOwner_ won't be set to NULL. This is
		// why PyFixedDictDataInstance always checks that we have not been
		// finalised before using its pOwner_.
		// NOTE: Can't use Script::isFinalised() since it is set too late. What
		// we really need is flag indicating is whether clearScript() has
		// already been called.
		MF_ASSERT_DEV( PyFixedDictDataInstancePtr::check( pObject ) ||
					!Py_IsInitialized() );
		if (PyFixedDictDataInstancePtr::check( pObject ))
		{
			PyFixedDictDataInstancePtr( pObject )->disowned();
		}
	}
#else
	// ! SCRIPT_PYTHON
	MF_ASSERT( false );
#endif
}

/*
 *	Overrides the DataType method.
 */
PropertyOwnerBase * FixedDictDataType::asOwner( ScriptObject pObject ) const
{
#if defined( SCRIPT_PYTHON )
	const_cast< FixedDictDataType * >( this )->initCustomClassImplOnDemand();

	if (this->hasCustomClass() && (!pObject.isNone()))
	{
		// TODO: Call isSameType()? A bit nasty to expect user code to handle
		// unexpected types but do we want to incur overhead?
		// See if they are referencing our PyFixedDictDataInstance
		ScriptObject pDict = this->getDictFromCustomClass( pObject );

		if (PyFixedDictDataInstance::Check( pDict ))
		{
			return static_cast<PyFixedDictDataInstance*>( pDict.get() );
		}
	}
	else
	{
		if (PyFixedDictDataInstance::Check( pObject ))
		{
			return static_cast<PyFixedDictDataInstance*>( pObject.get() );
		}
	}
#endif

	if (ScriptDict::check( pObject ))
	{
		// This is slightly dodgy to be returning a static instance. The caller
		// must not use the returned result after this method is called again.
		// This avoids having all calling code deallocate this.
		static ScriptDictPropertyOwner dictOwner;
		dictOwner.init( ScriptDict( pObject ), this );
		return &dictOwner;
	}

	return NULL;
}


DataType::StreamElementPtr FixedDictDataType::getStreamElement( size_t index,
	size_t & size, bool & isNone, bool isPersistentOnly ) const
{
	size = isNone ? 0 :
		(isPersistentOnly ?
			this->getNumPersistentFields() :
			this->getNumFields());

#if defined SCRIPT_PYTHON
	const_cast< FixedDictDataType * >( this )->initCustomClassImplOnDemand();

	if (hasCustomClass())
	{
		size = this->getNumFields();
		if (index == 0)
		{
			return StreamElementPtr(
				new FixedDictStreamedStreamElement( *this, isPersistentOnly,
					isNone ) );
		}
		return StreamElementPtr();
	}
#endif // defined SCRIPT_PYTHON

	if (index == 0)
	{
		// The isNone reference will be updated by a routine that reads
		// from a DataSource or BinaryIStream
		return StreamElementPtr(
			new FixedDictStartStreamElement( *this, isNone ) );
	}
	else if (index < (size * 2) + 1)
	{
		
		size_t fieldIndex = ( index - 1 ) / 2;
		if (isPersistentOnly)
		{
			fieldIndex = this->getFieldIndexForPersistentFieldIndex( fieldIndex );
		}

		int intFieldIndex = static_cast< int >( fieldIndex );
		if (index % 2 == 1)
		{
			return StreamElementPtr(
				new FixedDictEnterFieldStreamElement( *this,
					this->getFieldDataType( intFieldIndex ),
					this->getFieldName( intFieldIndex ) ) );
		}
		else
		{
			return StreamElementPtr(
				new FixedDictLeaveFieldStreamElement( *this ) );
		}
	}
	else if (index == (size * 2) + 1)
	{
		return StreamElementPtr( new FixedDictEndStreamElement( *this ) );
	}

	return StreamElementPtr();
}


/**
 *	Creates a PyFixedDictDataInstance with default values for each item.
 */
ScriptObject FixedDictDataType::createDefaultInstance() const
{
#if defined( SCRIPT_PYTHON )
	PyFixedDictDataInstancePtr pInst(
		new PyFixedDictDataInstance( const_cast< FixedDictDataType * >(this) ),
		PyFixedDictDataInstancePtr::FROM_NEW_REFERENCE );
#else // !SCRIPT_PYTHON
	ScriptDict pInst = ScriptDict::create();
#endif // SCRIPT_PYTHON

	MF_ASSERT( fields_.size() <= INT_MAX );
	int i = 0;
	for (Fields::const_iterator it = fields_.begin();
		it != fields_.end(); ++it, ++i)
	{
		ScriptDataSink sink;
		it->type_->getDefaultValue( sink );
		ScriptObject pValue = sink.finalise();
		MF_ASSERT_DEV( pValue );

#if defined( SCRIPT_PYTHON )
		pInst->initFieldValue( i, pValue );
#else // !SCRIPT_PYTHON
		pInst.setItem( it->name_.c_str(), pValue );
#endif // SCRIPT_PYTHON
	}

	return pInst;
}


#if defined( SCRIPT_PYTHON )
/**
 *	This method adds a PyFixedDictDataInstance into the stream.
 */
void FixedDictDataType::addInstanceToStream( PyFixedDictDataInstance* pInst,
		BinaryOStream& stream, bool isPersistentOnly ) const
{
	ScriptObject instance( pInst );
	ScriptDataSource source( instance );
	// Read PyFixedDictDataInstance as a mapping
	MF_VERIFY( source.beginDictionary( NULL ) );
	for (Fields::const_iterator it = fields_.begin(); it != fields_.end(); ++it)
	{
		if (isPersistentOnly && !it->isPersistent_)
		{
			continue;
		}

		// TODO: Error checking
		MF_VERIFY( source.enterField( it->name_ ) );
		MF_VERIFY( it->type_->addToStream( source, stream, isPersistentOnly ) );
		MF_VERIFY( source.leaveField() );
	}
}


/**
 *	This method creates a PyFixedDictDataInstance from the stream.
 */
ScriptObject FixedDictDataType::createInstanceFromStream(
		BinaryIStream& stream, bool isPersistentOnly ) const
{
	PyFixedDictDataInstancePtr pInst(
		new PyFixedDictDataInstance( const_cast<FixedDictDataType*>(this) ),
		PyFixedDictDataInstancePtr::FROM_NEW_REFERENCE );

	MF_ASSERT( fields_.size() <= INT_MAX );
	int i = 0;
	for (Fields::const_iterator it = fields_.begin();
		it != fields_.end(); ++it, ++i)
	{
		if ((!isPersistentOnly) || (it->isPersistent_))
		{
			ScriptDataSink sink;

			if (!it->type_->createFromStream( stream, sink, isPersistentOnly ))
			{
				ERROR_MSG( "FixedDictDataType::createInstanceFromStream: "
						"createFromStream failed\n" );
				return ScriptObject();	// error already printed
			}

			pInst->initFieldValue( i, sink.finalise() );
		}
		else
		{
			ScriptDataSink sink;

			if (!it->createDefaultValue( pDefaultSection_, sink ))
			{
				return ScriptObject();	// error already printed
			}

			pInst->initFieldValue( i, sink.finalise() );
		}
	}

	return pInst;
}


/**
 *	This method is adds a mapping object directly to a stream which is used
 *	when attaching children is not required (eg: method calls).
 *
 *	@param pyMappingObj	The mapping object to add to the stream.
 *	@param stream		The stream to add the mapping object to.
 *	@param isPersistentOnly  Indicates if only persistent data is being added.
 */
bool FixedDictDataType::addMappingObjToStream( ScriptObject pyMappingObj,
		BinaryOStream & stream, bool isPersistentOnly ) const
{
	ScriptMapping mappingObj = ScriptMapping::create( pyMappingObj );
	if (!mappingObj.exists())
	{
		ERROR_MSG( "FixedDictDataType::addMappingObjToStream: "
					"Cannot create from non-dictionary-like object\n" );
		// If we're fixed size, put _something_ into the stream
		if (this->streamSize() != -1 )
		{
			// TODO: Error checking, less ass
			ScriptDataSink sink;
			this->getDefaultValue( sink );
			ScriptObject temp = sink.finalise();
			ScriptDataSource source( temp );
			this->addToStream( source, stream, isPersistentOnly );
		}
		return false;
	}

	bool isOkay = true;
	for (Fields::const_iterator i = fields_.begin(); i != fields_.end(); ++i)
	{
		if ((!isPersistentOnly) || (i->isPersistent_))
		{
			ScriptObject pValue =
				mappingObj.getItem( i->name_.c_str(), ScriptErrorPrint() );

			if (!pValue.exists())
			{
				ERROR_MSG( "FixedDictDataType::addMappingObjToStream: "
							"Missing key '%s'\n", i->name_.c_str() );
				isOkay = false;
				// TODO: Error checking, less ass
				ScriptDataSink sink;
				i->type_->getDefaultValue( sink );
				ScriptObject temp = sink.finalise();
				ScriptDataSource source( temp );
				i->type_->addToStream( source, stream, isPersistentOnly );
			}
			else if (!i->type_->isSameType( pValue ))
			{
				ERROR_MSG( "FixedDictDataType::addMappingObjToStream: "
						"Value keyed by '%s' should be of type %s\n",
						i->name_.c_str(), i->type_->typeName().c_str() );
				isOkay = false;
				// TODO: Error checking, less ass
				ScriptDataSink sink;
				i->type_->getDefaultValue( sink );
				ScriptObject temp = sink.finalise();
				ScriptDataSource source( temp );
				i->type_->addToStream( source, stream, isPersistentOnly );
			}
			else
			{
				ScriptDataSource source( pValue );
				i->type_->addToStream( source, stream, isPersistentOnly );
			}
		}
	}

	return isOkay;
}


/**
 *	This method creates a PyFixedDictDataInstance from a data section.
 */
ScriptObject FixedDictDataType::createInstanceFromSection(
		DataSectionPtr pSection ) const
{
	PyFixedDictDataInstancePtr pInst(
			new PyFixedDictDataInstance( const_cast<FixedDictDataType*>(this) ),
			PyFixedDictDataInstancePtr::FROM_NEW_REFERENCE );

	MF_ASSERT( fields_.size() <= INT_MAX );
	int i = 0;
	for (Fields::const_iterator it = fields_.begin();
		it != fields_.end(); ++it, ++i)
	{
		DataSectionPtr pChildSection = pSection->openSection( it->name_ );
		if (pChildSection)
		{
			ScriptDataSink sink;
			if (!it->type_->createFromSection( pChildSection, sink ))
			{
				return ScriptObject();	// error already printed
			}

			pInst->initFieldValue( i, sink.finalise() );
		}
		else
		{
			ScriptDataSink sink;
			if (!it->createDefaultValue( pDefaultSection_, sink ))
			{
				return ScriptObject();	// error already printed
			}

			pInst->initFieldValue( i, sink.finalise() );
		}
	}

	return pInst;
}


/**
 *	This method adds data from PyFixedDictDataInstance into the section.
 */
bool FixedDictDataType::addInstanceToSection( PyFixedDictDataInstance* pInst,
		DataSectionPtr pSection ) const
{
	MF_ASSERT( fields_.size() <= INT_MAX );
	int i = 0;
	for (Fields::const_iterator it = fields_.begin();
		it != fields_.end(); ++it, ++i)
	{
		ScriptObject pValue = pInst->getFieldValue( i );
		ScriptDataSource source( pValue );
		if (!it->type_->addToSection( source,
				pSection->newSection( it->name_ ) ))
		{
			return false;
		}
	}
	return true;
}


/**
 *	This method creates a PyFixedDictDataInstance from a script object that
 *	supports the mapping protocol.
 */
ScriptObject FixedDictDataType::createInstanceFromMappingObj(
		ScriptObject pyMappingObj ) const
{
	PyFixedDictDataInstancePtr pInst;

	ScriptMapping mappingObj = ScriptMapping::create( pyMappingObj );
	if (mappingObj.exists())
	{
		pInst = PyFixedDictDataInstancePtr(
			new PyFixedDictDataInstance( const_cast<FixedDictDataType*>(this) ),
			PyObjectPtr::STEAL_REFERENCE );

		MF_ASSERT( fields_.size() <= INT_MAX );
		int i = 0;
		for (Fields::const_iterator it = fields_.begin();
			it != fields_.end(); ++it, ++i)
		{
			ScriptObject pValue =
				mappingObj.getItem( it->name_.c_str(), ScriptErrorPrint() );

			if (pValue.exists())
			{
				if (it->type_->isSameType( pValue ))
				{
					pInst->initFieldValue( i, pValue );
				}
				else
				{
					PyErr_Format( PyExc_TypeError, 
							"FixedDictDataType::createInstanceFromMappingObj: "
							"Value keyed by '%s' should be of type %s\n",
							it->name_.c_str(), it->type_->typeName().c_str() );
					return ScriptObject();
				}
			}
			else
			{
				ERROR_MSG( "FixedDictDataType::createInstanceFromMappingObj: "
							"Missing key '%s'\n", it->name_.c_str() );
				return ScriptObject();
			}
		}
	}
	else
	{
		ERROR_MSG( "FixedDictDataType::createInstanceFromMappingObj: "
					"Cannot create from non-dictionary-like object\n" );
	}

	return pInst;
}


/**
 *	This method checks whether the given Python object is the same type as
 * 	the user-defined type by calling the isSameType() function provided by
 * 	the user.
 */
bool FixedDictDataType::isSameTypeCustom( ScriptObject pValue ) const
{
	ScriptObject pResult(
		PyObject_CallFunctionObjArgs( pIsSameTypeFn_.get(), pValue.get(),
									NULL ),
						ScriptObject::STEAL_REFERENCE );

	if (!pResult)
	{
		ERROR_MSG( "FixedDictDataType::isSameTypeCustom: %s.%s.isSameType "
					"call failed\n", moduleName_.c_str(),
				instanceName_.c_str() );
		PyErr_Print();
		return false;
	}

	if (!PyBool_Check( pResult.get() ))
	{
		WARNING_MSG( "FixedDictDataType::isSameTypeCustom: "
					"%s.%s.isSameType method returned non-bool object\n",
				moduleName_.c_str(), instanceName_.c_str() );
	}

	return PyObject_IsTrue( pResult.get() ) != 0;
}


/**
 *	This method calls the getDictFromObj() script method and returns the result.
 */
ScriptObject FixedDictDataType::getDictFromCustomClass(
								ScriptObject pCustomClass ) const
{
	if (pGetDictFromObjFn_)
	{
		ScriptObject pResult(
			PyObject_CallFunctionObjArgs( pGetDictFromObjFn_.get(),
										pCustomClass.get(), NULL ),
			ScriptObject::STEAL_REFERENCE );
		if (pResult)
		{
			if (ScriptMapping::check( pResult ))
			{
				return pResult;
			}
			else
			{
				ERROR_MSG( "FixedDictDataType::getDictFromCustomClass: "
							"%s.%s.getDictFromObj returned non-dictionary "
							"object\n",
							moduleName_.c_str(), instanceName_.c_str() );
			}
		}
		else
		{
			ERROR_MSG( "FixedDictDataType::getDictFromCustomClass: "
						"%s.%s.getDictFromObj call failed\n",
						moduleName_.c_str(), instanceName_.c_str() );
			PyErr_Print();
		}
	}

	return ScriptObject();
}


/**
 *	This method calls the getDictFromObj() script method and creates a
 * 	PyFixedDictDataInstance from the result.
 */
ScriptObject FixedDictDataType::createInstanceFromCustomClass(
		ScriptObject pValue ) const
{
	ScriptObject pDict = this->getDictFromCustomClass( pValue );

	if (!pDict)
	{
		return ScriptObject();
	}

	if (PyFixedDictDataInstance::isSameType( pDict, *this ))
	{
		return pDict;
	}

	return this->createInstanceFromMappingObj( pDict );
}


/**
 *	This method calls the createObjFromDict() script method returns the
 * 	result of that call.
 */
ScriptObject FixedDictDataType::createCustomClassFromInstance(
		PyFixedDictDataInstance* pInst ) const
{
	ScriptObject pResult(
		PyObject_CallFunctionObjArgs( pCreateObjFromDictFn_.get(),
					static_cast<PyObject*>(pInst), NULL ),
		ScriptObject::FROM_NEW_REFERENCE );
	if (!pResult)
	{
		ERROR_MSG( "FixedDictDataType::createCustomClassFromInstance: "
					"%s.%s.createObjFromDict call failed\n",
					moduleName_.c_str(), instanceName_.c_str() );
		PyErr_Print();
	}

	return pResult;
}


/**
 *	This method calls the addToStream script method.
 */
void FixedDictDataType::addToStreamCustom( ScriptObject pValue,
	BinaryOStream& stream, bool isPersistentOnly) const
{
	ScriptObject pResult(
		PyObject_CallFunctionObjArgs( pAddToStreamFn_.get(), pValue.get(),
									NULL ),
		ScriptObject::FROM_NEW_REFERENCE );
	if (pResult)
	{
		ScriptString objString = ScriptString::create( pResult );
		if (objString.exists())
		{
			BW::string objStream;
			objString.getString( objStream );
			stream << objStream;
		}
		else
		{
			ERROR_MSG( "FixedDictDataType::addToStreamCustom: "
						"%s.%s.addToStream method returned non-string "
						"object\n",	moduleName_.c_str(),
						instanceName_.c_str() );
			stream << "";
		}
	}
	else
	{
		ERROR_MSG( "FixedDictDataType::addToStreamCustom: %s.%s.addToStream "
					"call failed\n", moduleName_.c_str(),
					instanceName_.c_str() );
		PyErr_Print();
		stream << "";
	}
}


/**
 *	This method calls the createFromStream() script method and returns the
 * 	result of that call.
 */
ScriptObject FixedDictDataType::createFromStreamCustom(
		BinaryIStream & stream, bool isPersistentOnly ) const
{
	BW::string data;
	stream >> data;

	if (stream.error())
	{
		ERROR_MSG( "FixedDictDataType::createFromStreamCustom: "
				   "Not enough data on stream to read value\n" );
		return ScriptObject();
	}

	ScriptObject pResult(
		PyObject_CallFunction( pCreateFromStreamFn_.get(), "s#",
								data.data(), data.length() ),
		ScriptObject::STEAL_REFERENCE );
	if (!pResult)
	{
		ERROR_MSG( "FixedDictDataType::createFromStreamCustom: "
					"%s.%s.createFromStream call failed\n", moduleName_.c_str(),
					instanceName_.c_str() );
		PyErr_Print();
	}

	return pResult;
}
#endif // SCRIPT_PYTHON


/**
 *	This method returns the number of persistent fields in this type
 */
FixedDictDataType::Fields::size_type
	FixedDictDataType::getNumPersistentFields() const
{
	FixedDictDataType::Fields::size_type count = 0;
	for (Fields::const_iterator it = fields_.begin();
		it != fields_.end();
		++it)
	{
		if (it->isPersistent_)
		{
			++count;
		}
	}
	return count;
}


/**
 *	This method translates a persistent field index into a field index
 */
FixedDictDataType::Fields::size_type
	FixedDictDataType::getFieldIndexForPersistentFieldIndex(
		FixedDictDataType::Fields::size_type persistentIndex ) const
{
	FixedDictDataType::Fields::size_type index = 0;

	for (Fields::const_iterator it = fields_.begin();
		it != fields_.end();
		++it)
	{
		if (it->isPersistent_)
		{
			if (persistentIndex-- == 0)
			{
				return index;
			}
		}
		++index;
	}

	MF_ASSERT( false && "Overran number of persistent fields" );

	return 0;
}


BW_END_NAMESPACE

// fixed_dict_data_type.cpp
