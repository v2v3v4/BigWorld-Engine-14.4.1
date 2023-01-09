#include "pch.hpp"

#include "user_data_type.hpp"

#include "cstdmf/binary_stream.hpp"
#include "cstdmf/md5.hpp"
#include "pyscript/py_data_section.hpp"
#include "resmgr/datasection.hpp"

#include "entitydef/script_data_sink.hpp"
#include "entitydef/script_data_source.hpp"

BW_BEGIN_NAMESPACE

// Anonymous namespace
namespace {

/**
 *	This class provides a DataType::StreamElement implementation for use by
 *	UserDataType.
 */
class UserStreamElement : public DataType::StreamElement
{
public:
	UserStreamElement( const UserDataType & type, bool isPersistentOnly ) :
		DataType::StreamElement( type ),
		isPersistentOnly_( isPersistentOnly )
	{};

	bool fromSourceToStream( DataSource & source, BinaryOStream & stream ) const
	{
		const UserDataType & userType =
			static_cast< const UserDataType & >( this->getType() );

		return source.readCustomType( userType, stream, isPersistentOnly_ );
	}

	bool fromStreamToSink( BinaryIStream & stream, DataSink & sink ) const
	{
		const UserDataType & userType =
			static_cast< const UserDataType & >( this->getType() );
		return sink.writeCustomType( userType, stream, isPersistentOnly_ );
	}
private:
	bool isPersistentOnly_;
};


} // Anonymous namespace

/**
 *	Destructor.
 */
UserDataType::~UserDataType()
{
	// If this assertion is hit, it means that this data type is being
	// destructed after the call to Script::fini. Make sure that the
	// EntityDescription is destructed or EntityDescription::clear called on it
	// before Script::fini.
	MF_ASSERT_DEV( !pObject_ || !Script::isFinalised() );
}


/**
 *	This method is used to ask our object to convert the given object
 *	into its stream format.
 *
 *	@param	object				The ScriptObject to convert from
 *	@param	stream				The BinaryOStream to write the data to
 *	@param	isPersistentOnly	true if only persistent data needs to be written
 *								to the stream
 *	@return						true unless conversion failed
 */
bool UserDataType::addObjectToStream( const ScriptObject & object,
	BinaryOStream & stream, bool /* isPersistentOnly */ ) const
{
	ScriptObject converter = this->pObject();

	if (!converter.exists())
	{
		ERROR_MSG( "UserDataType::addObjectToStream: "
				"Do not have %s.%s\n",
			moduleName_.c_str(), instanceName_.c_str() );
		stream << "";
		return false;
	}

	ScriptObject result = converter.callMethod( "addToStream",
		ScriptArgs::create( object ), ScriptErrorPrint() );

	if (!result.exists())
	{
		ERROR_MSG( "UserDataType::addObjectToStream: (%s,%s) "
				"Script call failed.\n",
			moduleName_.c_str(), instanceName_.c_str() );
		stream << "";
		return false;
	}

	ScriptString resultStr = ScriptString::create( result );
	if (!resultStr.exists())
	{
		ERROR_MSG( "UserDataType::addObjectToStream: (%s,%s) "
				"Script call did not return a string.\n",
			moduleName_.c_str(), instanceName_.c_str() );
		stream << "";
		return false;
	}

	BW::string newString;
	resultStr.getString( newString );

	stream << newString;

	return true;
}


/**
 *	This method is used to ask our object to convert the given stream
 *	into an instance of its object type.
 *
 *	@param	object				The ScriptObject reference to populate
 *	@param	stream				The BinaryIStream to read the data from
 *	@param	isPersistentOnly	true if only persistent data will be found
 *								in the stream
 *	@return						true unless conversion failed
 */
bool UserDataType::createObjectFromStream( ScriptObject & object,
	BinaryIStream & stream, bool /* isPersistentOnly */ ) const
{
	BW::string data;
	stream >> data;

	if (stream.error())
	{
		ERROR_MSG( "UserDataType::createObjectFromStream: "
				"Not enough data on stream to read value\n" );
		return false;
	}

	ScriptObject converter = this->pObject();

	if (!converter.exists())
	{
		ERROR_MSG( "UserDataType::createObjectFromStream: "
				"Do not have %s.%s\n",
			moduleName_.c_str(), instanceName_.c_str() );
		return false;
	}

	object = converter.callMethod( "createFromStream",
		ScriptArgs::create( data ), ScriptErrorPrint() );

	if (!object.exists())
	{
		ERROR_MSG( "UserDataType::createObjectFromStream: (%s,%s) "
				"Script call failed.\n",
			moduleName_.c_str(), instanceName_.c_str() );
		return false;
	}

	return true;
}


/**
 *	This method is used to ask our object to convert the given object
 *	into its DataSection format.
 *
 *	@param	object		The ScriptObject to convert from
 *	@param	pSection	The DataSection to write the data to
 *	@return				true unless conversion failed
 */
bool UserDataType::addObjectToSection( const ScriptObject & object,
	DataSectionPtr pSection ) const
{
	ScriptObject converter = this->pObject();

	if (!converter.exists())
	{
		ERROR_MSG( "UserDataType::addObjectToSection: "
				"Do not have %s.%s\n",
			moduleName_.c_str(), instanceName_.c_str() );
		return false;
	}

	ScriptObject section = ScriptObject( new PyDataSection( pSection ),
		ScriptObject::FROM_NEW_REFERENCE );

	ScriptObject result = converter.callMethod( "addToSection",
		ScriptArgs::create( object, section ), ScriptErrorPrint() );

	if (!result.exists())
	{
		ERROR_MSG( "UserDataType::addObjectToSection: (%s,%s) "
				"Script call failed.\n",
			moduleName_.c_str(), instanceName_.c_str() );
		return false;
	}

	return true;
}


/**
 *	This method is used to ask our object to convert the given DataSection
 *	into an instance of its object type.
 *
 *	@param	object				The ScriptObject reference to populate
 *	@param	pSection			The DataSection to read the data from
 *	@return						true unless conversion failed
 */
bool UserDataType::createObjectFromSection( ScriptObject & object,
	DataSectionPtr pSection ) const
{
	ScriptObject converter = this->pObject();

	if (!converter.exists())
	{
		ERROR_MSG( "UserDataType::createObjectFromSection: "
				"Do not have %s.%s\n",
			moduleName_.c_str(), instanceName_.c_str() );
		return false;
	}

	ScriptObject section = ScriptObject( new PyDataSection( pSection ),
		ScriptObject::FROM_NEW_REFERENCE );

	object = converter.callMethod( "createFromSection",
		ScriptArgs::create( section ), ScriptErrorPrint() );

	if (!object.exists())
	{
		ERROR_MSG( "UserDataType::createObjectFromSection: (%s,%s) "
				"Script call failed.\n",
			moduleName_.c_str(), instanceName_.c_str() );
		return false;
	}

	return true;
}


/**
 *	Overrides the DataType method.
 *
 *	@see DataType::isSameType
 */
bool UserDataType::isSameType( ScriptObject /*pValue*/ )
{
	// TODO: We should allow script check if this type is okay.
	return this->pObject() != NULL;
}

void UserDataType::reloadScript()
{
	this->init();
}

void UserDataType::clearScript()
{
	pObject_ = NULL;
	isInited_ = false;
}

/**
 *	This method sets the default value for this type.
 *
 *	@see DataType::setDefaultValue
 */
void UserDataType::setDefaultValue( DataSectionPtr pSection )
{
	pDefaultSection_ = pSection;
}

/**
 *	Overrides the DataType method.
 *
 *	@see DataType::getDefaultValue
 */
bool UserDataType::getDefaultValue( DataSink & output ) const
{
	if (pDefaultSection_)
		return this->createFromSection( pDefaultSection_, output );

	// TODO: Eww...
	ScriptDataSink & scriptOutput =
		static_cast< ScriptDataSink & >( output );

	// TODO: Use this->method + script::ask
	ScriptObject pObject = this->pObject();

	if (!pObject)
	{
		ERROR_MSG( "UserDataType::pDefaultValue: "
				"Do not have %s.%s\n",
			moduleName_.c_str(), instanceName_.c_str() );
		return scriptOutput.write( ScriptObject::none() );
	}

	ScriptObject pResult(
		PyObject_CallMethod( pObject.get(), "defaultValue", "" ),
		ScriptObject::STEAL_REFERENCE );

	if (!pResult)
	{
		ERROR_MSG( "UserDataType::pDefaultValue: (%s.%s)"
						"Script call failed.\n",
			moduleName_.c_str(), instanceName_.c_str() );
		PyErr_Print();
		return scriptOutput.write( ScriptObject::none() );
	}

	return scriptOutput.write( pResult );
}


/*
 *	Overrides the DataType method.
 *
 *	@see DataType::streamSize
 */
int UserDataType::streamSize() const
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
bool UserDataType::addToSection( DataSource & source,
	DataSectionPtr pSection ) const
{
	// The DataSource will bounce back to
	// UserDataType::addObjectToSection
	return source.readCustomType( *this, *pSection );
}

/**
 *	Overrides the DataType method.
 *
 *	@see DataType::createFromSection
 */
bool UserDataType::createFromSection( DataSectionPtr pSection,
	DataSink & sink ) const
{
	// The DataSink will bounce back to
	// UserDataType::createObjectFromSection
	return sink.writeCustomType( *this, *pSection );
}


bool UserDataType::fromStreamToSection( BinaryIStream & stream,
		DataSectionPtr pSection, bool isPersistentOnly ) const
{
	ScriptObject pMethod = this->method( "fromStreamToSection" );
	if (!pMethod)
	{
		return this->DataType::fromStreamToSection( stream, pSection,
													isPersistentOnly );
	}

	BW::string data;
	stream >> data;
	if (stream.error())
	{
		ERROR_MSG( "UserDataType::fromStreamToSection: "
		   "Bad string on stream\n" );
		return false;
	}

	ScriptObject pPySection( new PyDataSection( pSection ),
			ScriptObject::STEAL_REFERENCE );

	return Script::call( pMethod.newRef(),
		Py_BuildValue( "s#O", data.data(), data.length(), pPySection.get() ),
		"UserDataType::fromStreamToSection: " );
}

bool UserDataType::fromSectionToStream( DataSectionPtr pSection,
		BinaryOStream & stream, bool isPersistentOnly ) const
{
	ScriptObject pMethod = this->method( "fromSectionToStream" );
	if (!pMethod)
	{
		return this->DataType::fromSectionToStream( pSection, stream,
													isPersistentOnly );
	}

	if (!pSection)
	{
		return false;
	}

	PyObject * pPySection = new PyDataSection( pSection );

	PyObject * pResult = Script::ask(
		pMethod.newRef(),
		Py_BuildValue( "(O)", pPySection ),
		"UserDataType::fromSectionToStream: " );

	Py_DECREF( pPySection );

	if (!pResult)
	{
		stream << "";	// hmmm
		return false;
	}

	if (!PyString_Check( pResult ))
	{
		ERROR_MSG( "UserDataType::fromSectionToStream: (%s.%s) "
					"Method did not return a string.\n",
				moduleName_.c_str(), instanceName_.c_str() );
		Py_DECREF( pResult );
		stream << "";	// hmmm
		return false;
	}

	BW::string s;
	Script::setData( pResult, s );
	Py_DECREF( pResult );

	stream << s;
	return true;
}


void UserDataType::addToMD5( MD5 & md5 ) const
{
	md5.append( "User", sizeof( "User" ) );
	md5.append( moduleName_.c_str(), static_cast<int>(moduleName_.size()) );
	md5.append( instanceName_.c_str(), static_cast<int>(instanceName_.size()) );
}


DataType::StreamElementPtr UserDataType::getStreamElement(
	size_t index, size_t & size, bool & /* isNone */,
	bool isPersistentOnly ) const
{
	size = 1;
	if (index == 0)
	{
		return StreamElementPtr(
			new UserStreamElement( *this, isPersistentOnly ) );
	}
	return StreamElementPtr();
}


bool UserDataType::operator<( const DataType & other ) const
{
	if (this->DataType::operator<( other )) return true;
	if (other.DataType::operator<( *this )) return false;

	// ok, equal metas, so downcast other and compare with us
	UserDataType & otherUser = (UserDataType&)other;
	int diff = (moduleName_ + instanceName_).compare(
					otherUser.moduleName_ + otherUser.instanceName_ );
	if (diff < 0) return true;
	if (diff > 0) return false;
	// much more stable to compare strings than PyObjectPtrs

	if (otherUser.pDefaultSection_)
		return otherUser.pDefaultSection_->compare( pDefaultSection_ ) > 0;
	// else we are equal or greater than other.
	return false;
}

BW::string UserDataType::typeName() const
{
	return this->DataType::typeName() + " by " + moduleName_ + "." +
		instanceName_;
}

void UserDataType::init()
{
	isInited_ = true;
	PyObjectPtr pModule(
			PyImport_ImportModule(
				const_cast< char * >( moduleName_.c_str() ) ),
			PyObjectPtr::STEAL_REFERENCE );

	if (pModule)
	{
		pObject_ = PyObject_GetAttrString( pModule.get(),
						const_cast< char * >( instanceName_.c_str() ) );
		if (!pObject_)
		{
			ERROR_MSG( "UserDataType::pObject: "
									"Unable to import %s from %s\n",
				instanceName_.c_str(), moduleName_.c_str() );
			PyErr_Print();
		}
		else
		{
			Py_DECREF( pObject_.get() );
		}
	}
	else
	{
		ERROR_MSG( "UserDataType::pObject: Unable to import %s\n",
			moduleName_.c_str() );
		PyErr_Print();
	}
}

inline ScriptObject UserDataType::method( const char * name ) const
{
	ScriptObject pObject = this->pObject();

	if (!pObject)
	{
		ERROR_MSG( "UserDataType: Do not have %s.%s\n",
			moduleName_.c_str(), instanceName_.c_str() );
		return ScriptObject();
	}

	ScriptObject pMethod( PyObject_GetAttrString( pObject.get(),
				const_cast<char*>( name ) ),
			ScriptObject::STEAL_REFERENCE );

	if (!pMethod)
	{
		PyErr_Clear();
	}

	return pMethod;
}

BW_END_NAMESPACE

// user_data_type.cpp
