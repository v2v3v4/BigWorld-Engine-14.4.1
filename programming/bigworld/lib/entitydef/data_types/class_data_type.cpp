#include "pch.hpp"

#include "class_data_type.hpp"

#include "entitydef/data_source.hpp"

#include "entitydef/data_instances/class_data_instance.hpp"

#include "cstdmf/binary_stream.hpp"
#include "cstdmf/md5.hpp"
#include "resmgr/datasection.hpp"

#include "entitydef/script_data_sink.hpp"

BW_BEGIN_NAMESPACE

// Anonymous namespace
namespace {

/**
 *	This class is the first StreamElement of a CLASS
 */
class ClassStartStreamElement : public DataType::StreamElement
{
public:
	ClassStartStreamElement( const ClassDataType & type,
		bool & isNone ) :
	DataType::StreamElement( type ),
	isNone_( isNone )
	{};

	bool containerSize( size_t & rSize ) const
	{
		const ClassDataType & classType =
			static_cast< const ClassDataType & >( this->getType() );
		// TODO: Only count persistent fields?
		rSize = classType.getFields().size();
		return true;
	}

	bool isNoneAble() const
	{
		const ClassDataType & classType =
			static_cast< const ClassDataType & >( this->getType() );

		return classType.allowNone();
	}

	bool fromSourceToStream( DataSource & source, BinaryOStream & stream ) const
	{
		const ClassDataType & classType =
			static_cast< const ClassDataType & >( this->getType() );
		bool result = true;
		if (classType.allowNone())
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

		result = result && source.beginClass();

		return result;
	}

	bool fromStreamToSink( BinaryIStream & stream, DataSink & sink ) const
	{
		const ClassDataType & classType =
			static_cast< const ClassDataType & >( this->getType() );

		bool result = true;
		if (classType.allowNone())
		{
			uint8 hasValues = 0;
			stream >> hasValues;
			isNone_ = (hasValues == 0);

			if (stream.error())
			{
				ERROR_MSG( "ClassStartStreamElement::fromStreamToSink "
						"Missing None/values indicator on stream\n" );
			}

			result &= !stream.error();

			result = result && sink.writeNone( isNone_ );

			if (isNone_)
			{
				return result;
			}
		}

		result = result && sink.beginClass( &classType );

		return result;
	}

private:
	bool & isNone_;
};

/**
 *	This class is the StreamElement for each field of a CLASS.
 */
class ClassEnterFieldStreamElement : public DataType::StreamElement
{
public:
	ClassEnterFieldStreamElement( const ClassDataType & type,
		const DataType & childType, const BW::string & fieldName ) :
	DataType::StreamElement( type ),
	childType_( childType ),
	fieldName_( fieldName )
	{};

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
 *	This class is the StreamElement after each field of a CLASS.
 */
class ClassLeaveFieldStreamElement : public DataType::StreamElement
{
public:
	ClassLeaveFieldStreamElement( const ClassDataType & type ) :
	DataType::StreamElement( type )
	{};

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
 *	This class is the last StreamElement of a CLASS
 */
class ClassEndStreamElement : public DataType::StreamElement
{
public:
	ClassEndStreamElement( const ClassDataType & type ) :
	DataType::StreamElement( type )
	{};

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
// Section: ClassDataType
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
ClassDataType::ClassDataType( MetaDataType * pMeta,
		Fields & fields, bool allowNone ) :
	DataType( pMeta, /*isConst:*/false ),
	allowNone_( allowNone ),
	clientSafety_( CLIENT_SAFE )
{
	fields_.swap( fields );

	MF_ASSERT( fields.size() <= INT_MAX );
	int i = 0;
	for (Fields::iterator it = fields_.begin(); it != fields_.end(); ++it, ++i )
	{
		fieldMap_[it->name_] = i;

		clientSafety_ |= it->type_->clientSafety();
	}
}


/**
 *	Overrides the DataType method.
 *
 *	@see DataType::isSameType
 */
bool ClassDataType::isSameType( ScriptObject pValue )
{
	if (pValue.get() == Py_None && allowNone_) return true;

	PyClassDataInstancePtr pInst;

	// check if an instance of our special Python class
	if (PyClassDataInstance::Check( pValue.get() ) &&
		((PyClassDataInstance*)pValue.get())->dataType() == this)
	{
		return true;
	}
	else
	{
		// ok try to import it then
		pInst = PyClassDataInstancePtr( new PyClassDataInstance( this ),
			pInst.STEAL_REFERENCE );
		bool ok = pInst->setToForeign( pValue.get(), PyMapping_Check( pValue.get() ) ?
			pInst->FOREIGN_MAPPING : pInst->FOREIGN_ATTRS );

		if (!ok)
		{
			 PyErr_Print();
		}
		// throw it away and return (what a waste! maybe add a cache?...)
		return ok;
	}
}

/**
 *	This method sets the default value for this type.
 *
 *	@see DataType::setDefaultValue
 */
void ClassDataType::setDefaultValue( DataSectionPtr pSection )
{
	pDefaultSection_ = pSection;
}

/**
 *	Overrides the DataType method.
 *
 *	@see DataType::getDefaultValue
 */
bool ClassDataType::getDefaultValue( DataSink & output ) const
{
	if (pDefaultSection_)
	{
		return this->createFromSection( pDefaultSection_, output );
	}

	if (allowNone_)
	{
		return output.writeNone( true );
	}

	// TODO: How can I hand this off? Otherwise I can just call
	// return this->createFromSection( DataSection(), output );
	// Or open-code it.
	PyClassDataInstance * pNewInst = new PyClassDataInstance( this );
	pNewInst->setToDefault();
	// TODO: Eww...
	ScriptDataSink & scriptOutput =
		static_cast< ScriptDataSink & >( output );
	return scriptOutput.write(
		ScriptObject( pNewInst, ScriptObject::STEAL_REFERENCE ) );
}


/**
 *
 */
DataSectionPtr ClassDataType::pDefaultSection() const
{
	return pDefaultSection_;
}


/*
 *	Overrides the DataType method.
 *
 *	@see DataType::streamSize
 */
int ClassDataType::streamSize() const
{
	// Don't let bad Python streaming code break anything
	// except the current packet
	return -1;
}


/**
 *	Overrides the DataType method.
 *
 *	@see DataType::addToSection
 */
bool ClassDataType::addToSection( DataSource & source,
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

	if (!source.beginClass())
	{
		return false;
	}

	for (Fields_iterator it = fields_.begin(); it != fields_.end(); ++it )
	{
		if (!source.enterField( it->name_ ) ||
			!it->type_->addToSection( source,
				pSection->newSection( it->name_ ) ) ||
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
bool ClassDataType::createFromSection( DataSectionPtr pSection,
		DataSink & sink ) const
{
	if (!pSection)
	{
		ERROR_MSG( "ClassDataType::createFromSection: Section is NULL.\n" );
		return false;
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

	if (!sink.beginClass( this ))
	{
		return false;
	}

	for (Fields_iterator it = fields_.begin(); it != fields_.end(); ++it )
	{
		DataSectionPtr pChildSection = pSection->openSection( it->name_ );
		if (pChildSection)
		{
			if (!sink.enterField( it->name_ ) ||
				!it->type_->createFromSection( pChildSection, sink ) ||
				!sink.leaveField())
			{
				return false;
			}
		}
		else
		{
			if (!sink.enterField( it->name_ ) ||
				!it->createDefaultValue( pDefaultSection_, sink ) ||
				!sink.leaveField())
			{
				return false;
			}
		}
	}

	return true;
}


/**
 *	Attach to the given owner; or copy the object if we already have one
 *	(or it is foreign and should be copied anyway).
 */
ScriptObject ClassDataType::attach( ScriptObject script,
	PropertyOwnerBase * pOwner, int ownerRef )
{
	//if (!this->DataType::attach( script, pOwner, ownerRef )) return NULL;
	// don't call the base class method as it's inefficient for foreign
	// types - it would convert them all twice!

	// if it's None and that's ok, just return that
	if (script.isNone() && allowNone_)
	{
		return ScriptObject::none();
	}


	PyClassDataInstancePtr pInst = PyClassDataInstancePtr::create( script );

	// it's easy if it's the right python + entitydef type
	if (pInst.exists() && pInst->dataType() == this)
	{
		if (pInst->hasOwner())
		{	// note: up to caller to check that prop isn't being set back
			PyClassDataInstancePtr pOldInst = pInst;	// into itself
			pInst = PyClassDataInstancePtr( new PyClassDataInstance( this ),
				PyClassDataInstancePtr::FROM_NEW_REFERENCE );
			pInst->setToCopy( *pOldInst.get() );
		}
	}
	// otherwise just see if it has the same keys/attrs
	else
	{
		PyClassDataInstancePtr pOldInst = pInst;
		pInst = PyClassDataInstancePtr( new PyClassDataInstance( this ),
			PyClassDataInstancePtr::FROM_NEW_REFERENCE );
		if (!pInst->setToForeign( pOldInst.get(),
			PyMapping_Check( pOldInst.get() ) ?
				PyClassDataInstance::FOREIGN_MAPPING :
				PyClassDataInstance::FOREIGN_ATTRS ))
		{
			PyErr_Print();
			return ScriptObject();
		}
	}

	pInst->setOwner( pOwner, ownerRef );
	return ScriptObject( pInst.get() );
}

void ClassDataType::detach( ScriptObject script )
{
	PyObject * pObject = script.get();

	if (pObject == Py_None) return;

	IF_NOT_MF_ASSERT_DEV( PyClassDataInstance::Check( pObject ) )
	{
		return;
	}

	((PyClassDataInstance*)pObject)->disowned();
}

PropertyOwnerBase * ClassDataType::asOwner( ScriptObject pObject ) const
{
	if (!PyClassDataInstance::Check( pObject.get() )) return NULL; // for None

	// __kyl__ (17/2/2006) The following assert is actually not true after
	// reloadScript. May be can restore it after updater is fully operational.
	// MF_ASSERT( ((PyClassDataInstance*)pObject)->dataType() == this );
	return (PyClassDataInstance*)pObject.get();
}


void ClassDataType::addToMD5( MD5 & md5 ) const
{
	md5.append( "Class", sizeof( "Class" ) );
	md5.append( &allowNone_, sizeof(allowNone_) );
	for (Fields_iterator it = fields_.begin(); it != fields_.end(); ++it)
	{
		md5.append( it->name_.data(), static_cast<int>(it->name_.size()) );
		it->type_->addToMD5( md5 );
	}
}


DataType::StreamElementPtr ClassDataType::getStreamElement( size_t index,
	size_t & size, bool & isNone, bool isPersistentOnly ) const
{
	// TODO: Only send persistent fields.
	size = isNone ? 0 : this->getFields().size();
	if (index == 0)
	{
		// The isNone reference will be updated by a routine that reads
		// from a DataSource or BinaryIStream
		return StreamElementPtr(
			new ClassStartStreamElement( *this, isNone ) );
	}
	else if (index < (size * 2) + 1)
	{
		int fieldIndex = static_cast<int>( index - 1 ) / 2;
		if (index % 2 == 1)
		{
			return StreamElementPtr(
				new ClassEnterFieldStreamElement( *this,
					*this->getFields()[ fieldIndex ].type_,
					this->getFields()[ fieldIndex ].name_ ) );
		}
		else
		{
			return StreamElementPtr(
				new ClassLeaveFieldStreamElement( *this ) );
		}
	}
	else if (index == (size * 2) + 1)
	{
		return StreamElementPtr( new ClassEndStreamElement( *this ) );
	}

	return StreamElementPtr();
}


bool ClassDataType::operator<( const DataType & other ) const
{
	if (this->DataType::operator<( other )) return true;
	if (other.DataType::operator<( *this )) return false;

	// ok, equal metas, so downcast other and compare with us
	ClassDataType & otherCl= (ClassDataType&)other;
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

BW::string ClassDataType::typeName() const
{
	BW::string acc = this->DataType::typeName();
	if (allowNone_) acc += " [None Allowed]";
	for (Fields_iterator it = fields_.begin(); it != fields_.end(); ++it)
	{
		acc += (it == fields_.begin()) ? " props " : ", ";
		acc += it->name_ + ":(" + it->type_->typeName() + ")";
	}
	return acc;
}


bool ClassDataType::Field::createDefaultValue( DataSectionPtr pDefaultSection,
	DataSink & sink ) const
{
	DataSectionPtr pChildSection = pDefaultSection ?
		pDefaultSection->openSection( name_ ) : NULL;

	if (!pChildSection)
	{
		return type_->getDefaultValue( sink );
	}

	bool result = type_->createFromSection( pChildSection, sink );

	if (!result)
	{

	ERROR_MSG( "ClassDataType::Field::createDefaultValue: "
			"Failed to create %s from default section\n",
		name_.c_str() );
	}

	return result;
}


/**
 *
 */
int ClassDataType::fieldIndexFromName( const char * name ) const
{
	FieldMap::const_iterator found = fieldMap_.find( name );

	if (found != fieldMap_.end())
	{
		return found->second;
	}

	return -1;
}

BW_END_NAMESPACE

// class_data_type.cpp
