#if not defined( SCRIPT_OBJECTIVE_C )
#error Only supported with Objective C Script Objects. \
	See chunk/user_data_object_link_data_type.cpp for real implementation
#endif // SCRIPT_OBJECTIVE_C

#include "pch.hpp"

#include "udo_ref_data_type.hpp"

#include "cstdmf/binary_stream.hpp"
#include "cstdmf/md5.hpp"
#include "cstdmf/unique_id.hpp"

#include "entitydef/data_types.hpp"


BW_BEGIN_NAMESPACE

// Anonymous namespace
namespace {

/**
 *	This class provides a DataType::StreamElement implementation for use by
 *	UDORefDataType.
 */
class UDORefStreamElement : public DataType::StreamElement
{
public:
	UDORefStreamElement( const UDORefDataType & type ) :
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
UDORefDataType::UDORefDataType( MetaDataType * pMeta ):
	DataType( pMeta, /*isConst:*/false )
{
}


/*
 *	Overrides the DataType method.
 *
 *	@see DataType::streamSize
 */
int UDORefDataType::streamSize() const
{
	// XXX: Murph guesses return sizeof( UniqueID );
	return sizeof( UniqueID );
}


void UDORefDataType::addToMD5( MD5 & md5 ) const
{
	const char md5String[] = "UserDataObjectLinkDataType";
	md5.append( md5String, sizeof( md5String ) );
}


DataType::StreamElementPtr UDORefDataType::getStreamElement( size_t index,
	size_t & size, bool & /* isNone */, bool /* isPersistentOnly */ ) const
{
	size = 1;
	if (index == 0)
	{
		return StreamElementPtr( new UDORefStreamElement( *this ) );
	}
	return StreamElementPtr();
}


bool UDORefDataType::operator<( const DataType & other ) const
{
	if (this->DataType::operator<( other )) return true;
	if (other.DataType::operator<( *this )) return false;

#if 1
	return false;
#else
	const UDORefDataType& otherStr =
		static_cast< const UDORefDataType& >( other );
	return pDefaultValue_ < otherStr.pDefaultValue_;
#endif
}


SIMPLE_DATA_TYPE( UDORefDataType, UDO_REF )

BW_END_NAMESPACE

// udo_ref_data_type.cpp
