#include "pch.hpp"

#include "user_data_object_link_data_type.hpp"

#include "cstdmf/binary_stream.hpp"
#include "cstdmf/guard.hpp"
#include "cstdmf/md5.hpp"

#include "entitydef/data_types.hpp"
#include "entitydef/data_sink.hpp"
#include "entitydef/data_source.hpp"

#ifndef EDITOR_ENABLED
#include "user_data_object.hpp"
#endif


BW_BEGIN_NAMESPACE

#ifndef EDITOR_ENABLED

// Anonymous namespace
namespace {

/**
 *	This class provides a DataType::StreamElement implementation for use by
 *	UserDataObjectLinkDataType.
 */
class UDORefStreamElement : public DataType::StreamElement
{
public:
	UDORefStreamElement( const UserDataObjectLinkDataType & type ) :
		DataType::StreamElement( type )
	{};

	bool fromSourceToStream( DataSource & source, BinaryOStream & stream ) const
	{
		bool isNone;
		if (!source.readNone( isNone ))
		{
			stream << UniqueID::zero();
			return false;
		}

		if (isNone)
		{
			stream << UniqueID::zero();
			return true;
		}

		UniqueID guid;

		if (!source.read( guid ))
		{
			stream << UniqueID::zero();
			return false;
		}

		stream << guid;

		return true;
	}

	bool fromStreamToSink( BinaryIStream & stream, DataSink & sink ) const
	{
		UniqueID guid;

		stream >> guid;

		bool isNone = (guid == UniqueID::zero());

		if (!sink.writeNone( isNone ))
		{
			return false;
		}

		if (isNone)
		{
			return true;
		}

		return sink.write( guid );
	}
};


} // Anonymous namespace

/**
 *	Constructor
 */
UserDataObjectLinkDataType::UserDataObjectLinkDataType( MetaDataType * pMeta ):
	DataType( pMeta, /*isConst:*/false )
{
}


/**
 *	Overrides the DataType method.
 *
 *	@see DataType::isSameType
 */
bool UserDataObjectLinkDataType::isSameType( ScriptObject pValue )
{
	return UserDataObject::Check( pValue.get() ) || (pValue.get() == Py_None);
}


/**
 *	This method sets the default value for this type.
 *
 *	@see DataType::setDefaultValue
 */
void UserDataObjectLinkDataType::setDefaultValue( DataSectionPtr pSection )
{
	defaultId_ = "";
	defaultChunkId_ = "";
}


/**
 *	Overrides the DataType method.
 *
 *	@see DataType::getDefaultValue
 */
bool UserDataObjectLinkDataType::getDefaultValue( DataSink & output ) const
{
	return output.writeNone( /* isNone */ true );
}


/*
 *	Overrides the DataType method.
 *
 *	@see DataType::streamSize
 */
int UserDataObjectLinkDataType::streamSize() const
{
	// XXX: Murph guesses return sizeof( UniqueID );
	return sizeof( UniqueID );
}


/**
 *	Overrides the DataType method.
 *
 *	@see DataType::addToSection
 */
bool UserDataObjectLinkDataType::addToSection( DataSource & source,
		DataSectionPtr pSection ) const
{
	BW_GUARD;

	bool isNone;
	if (!source.readNone( isNone ))
	{
		return false;
	}

	if (isNone)
	{
		return true;
	}

	UniqueID guid;

	if (!source.read( guid ))
	{
		return false;
	}

	pSection->writeString( "guid", guid.toString() );

	return true;
}


/**
 *	Overrides the DataType method.
 *
 *	@see DataType::createFromSection
 */
bool UserDataObjectLinkDataType::createFromSection( DataSectionPtr pSection,
		DataSink & sink ) const
{
	BW_GUARD;

	UniqueID guid( pSection->readString( "guid", "" ) );

	bool isNone = (guid == UniqueID::zero());

	if (!sink.writeNone( isNone ))
	{
		return false;
	}

	if (isNone)
	{
		return true;
	}

	return sink.write( guid );
}


#else // EDITOR_ENABLED


// Anonymous namespace
namespace {

/**
 *	This class provides a DataType::StreamElement implementation for use by
 *	BlobDataType.
 */
class UDORefStreamElement : public DataType::StreamElement
{
public:
	UDORefStreamElement( const DataType & type ) :
		DataType::StreamElement( type )
	{};

	bool fromSourceToStream( DataSource & source, BinaryOStream & stream ) const
	{
		BW::string id;
		BW::string chunkID;

		size_t tupleSize;

		if (!source.beginSequence( tupleSize ) ||
			tupleSize != 2 ||
			!source.enterItem( 0 ) ||
			!source.read( id ) ||
			!source.leaveItem() ||
			!source.enterItem( 1 ) ||
			!source.read( chunkID ) ||
			!source.leaveItem())
		{
			CRITICAL_MSG(
				"UDORefStreamElement::fromSourceToStream: type was not"
				" a tuple of two strings (id, chunkId).\n" );
			stream << "" << "";
			return false;
		}

		stream << id;
		stream << chunkID;

		return true;
	}

	bool fromStreamToSink( BinaryIStream & stream, DataSink & sink ) const
	{
		BW::string id;
		BW::string chunkId;
		stream >> id;
		stream >> chunkId;

		if (stream.error())
		{
			ERROR_MSG( "UDORefStreamElement::fromStreamToSink: "
				"Not enough data on stream to read value\n" );
			return false;
		}

		return sink.beginTuple( NULL, 2 ) &&
			sink.enterItem( 0 ) &&
			sink.write( id ) &&
			sink.leaveItem() &&
			sink.enterItem( 1 ) &&
			sink.write( chunkId ) &&
			sink.leaveItem();
	}
};


} // Anonymous namespace

///////////////////////////////////////////////////////////////////////////////
// Editor side UserDataObjectLinkDataType Class
///////////////////////////////////////////////////////////////////////////////

UserDataObjectLinkDataType::UserDataObjectLinkDataType( MetaDataType * pMeta ) :
	DataType( pMeta, /*isConst:*/false ),
	defaultId_( "" ),
	defaultChunkId_( "" )
{
}


/**
 *	This method returns a string representation of the data type.
 *
 *	@param pValue	Python object containing a UserDataObjectLinkDataType.
 *	@return			String representation of the data type.
 */
/*static*/ BW::string UserDataObjectLinkDataType::asString( PyObject* pValue )
{
	BW_GUARD;
	if ( !PyTuple_Check( pValue ) || PyTuple_Size( pValue ) != 2 )
		return "( , )";

	return BW::string( "( " ) +
		PyString_AsString( PyTuple_GetItem( pValue, 0 ) ) + ", " +
		PyString_AsString( PyTuple_GetItem( pValue, 1 ) ) + " )";
}


/**
 *	Overrides the DataType method.
 *
 *	@see DataType::isSameType
 */
bool UserDataObjectLinkDataType::isSameType( ScriptObject pValue )
{
	BW_GUARD;

	ScriptTuple spTuple = ScriptTuple::create( pValue );

	if (!pValue || spTuple.size() != 2)
	{
		return false;
	}

	BW::string id;
	BW::string chunkId;

	ScriptErrorPrint scriptErrorPrint( "UserDataObjectLinkDataType::isSameType" );
	if (!spTuple.getItem( 0 ).convertTo( id, "user data object ID", scriptErrorPrint ) ||
		!spTuple.getItem( 1 ).convertTo( chunkId, "chunk ID", scriptErrorPrint ))
	{
		return false;
	}

	return true;
}


/**
 *	This method sets the default value for this type.
 *
 *	@see DataType::setDefaultValue
 */
void UserDataObjectLinkDataType::setDefaultValue( DataSectionPtr pSection )
{
	BW_GUARD;
	if (pSection)
	{
		defaultId_ = pSection->readString( "guid", "" );
		defaultChunkId_ = pSection->readString( "chunkId", "" );
	}
	else
	{
		defaultId_ = "";
		defaultChunkId_ = "";
	}
}


/**
 *	Overrides the DataType method.
 *
 *	@see DataType::getDefaultValue
 */
bool UserDataObjectLinkDataType::getDefaultValue( DataSink & output ) const
{
	BW_GUARD;
	return output.beginTuple( NULL, 2 ) &&
		output.enterItem( 0 ) &&
		output.write( defaultId_ ) &&
		output.leaveItem() &&
		output.enterItem( 1 ) &&
		output.write( defaultChunkId_ ) &&
		output.leaveItem();
}


/*
 *	Overrides the DataType method.
 *
 *	@see DataType::streamSize
 */
int UserDataObjectLinkDataType::streamSize() const
{
	return -1;
}


/**
 *	Overrides the DataType method.
 *
 *	@see DataType::addToSection
 */
bool UserDataObjectLinkDataType::addToSection( DataSource & source,
		DataSectionPtr pSection ) const
{
	BW_GUARD;

	BW::string id;
	BW::string chunkId;
	size_t tupleSize;

	if (!source.beginSequence( tupleSize ) ||
		tupleSize != 2 ||
		!source.enterItem( 0 ) ||
		!source.read( id ) ||
		!source.leaveItem() ||
		!source.enterItem( 1 ) ||
		!source.read( chunkId ) ||
		!source.leaveItem())
	{
		CRITICAL_MSG(
			"UserDataObjectLinkDataType::addToSection: type was not"
			" a tuple of two strings (id, chunkId).\n" );
		return false;
	}

	pSection->writeString( "guid", id );
	pSection->writeString( "chunkId", chunkId );
	return true;
}


/**
 *	Overrides the DataType method.
 *
 *	@see DataType::createFromSection
 */
bool UserDataObjectLinkDataType::createFromSection( DataSectionPtr pSection,
		DataSink & sink ) const
{
	BW_GUARD;
	BW::string id = pSection->readString( "guid", "" );
	BW::string chunkId = pSection->readString( "chunkId", "" );

	return sink.beginTuple( NULL, 2 ) &&
		sink.enterItem( 0 ) &&
		sink.write( id ) &&
		sink.leaveItem() &&
		sink.enterItem( 1 ) &&
		sink.write( chunkId ) &&
		sink.leaveItem();
}


#endif // EDITOR_ENABLED

void UserDataObjectLinkDataType::addToMD5( MD5 & md5 ) const
{
	BW_GUARD;
	const char md5String[] = "UserDataObjectLinkDataType";
	md5.append( md5String, sizeof( md5String ) );
}


DataType::StreamElementPtr UserDataObjectLinkDataType::getStreamElement(
	size_t index, size_t & size, bool & /* isNone */,
	bool /* isPersistentOnly */ ) const
{
	size = 1;
	if (index == 0)
	{
		return StreamElementPtr( new UDORefStreamElement( *this ) );
	}
	return StreamElementPtr();
}


bool UserDataObjectLinkDataType::operator<( const DataType & other ) const
{
	BW_GUARD;
	if (this->DataType::operator<( other )) return true;
	if (other.DataType::operator<( *this )) return false;

	const UserDataObjectLinkDataType& otherUDO =
		static_cast< const UserDataObjectLinkDataType& >( other );
	return
		defaultId_ < otherUDO.defaultId_ ||
		(defaultId_ == otherUDO.defaultId_ &&
			defaultChunkId_ < otherUDO.defaultChunkId_);
}


// static class to implement the data type
class UserDataObjectDataType : public UserDataObjectLinkDataType
{
public:
	UserDataObjectDataType(MetaDataType * pMetaType ) : UserDataObjectLinkDataType( pMetaType ) {}
};

SIMPLE_DATA_TYPE( UserDataObjectDataType, UDO_REF )

BW_END_NAMESPACE

// user_data_object_link_data_type.cpp
