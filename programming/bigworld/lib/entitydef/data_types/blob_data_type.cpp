#include "pch.hpp"

#include "blob_data_type.hpp"

#include "cstdmf/binary_stream.hpp"
#include "cstdmf/md5.hpp"

#include "entitydef/data_sink.hpp"
#include "entitydef/data_source.hpp"
#include "entitydef/data_types.hpp"

#include "resmgr/datasection.hpp"


BW_BEGIN_NAMESPACE

// Anonymous namespace
namespace {

/**
 *	This class provides a DataType::StreamElement implementation for use by
 *	BlobDataType.
 */
class BlobStreamElement : public DataType::StreamElement
{
public:
	BlobStreamElement( const BlobDataType & type ) :
		DataType::StreamElement( type )
	{};

	bool fromSourceToStream( DataSource & source, BinaryOStream & stream ) const
	{
		BW::string value;

		bool result = source.readBlob( value );
		stream << value;
		return result;
	}

	bool fromStreamToSink( BinaryIStream & stream, DataSink & sink ) const
	{
		BW::string value;
		stream >> value;

		if (stream.error())
		{
			ERROR_MSG( "BlobStreamElement: Not enough data on stream to read "
				"value\n");
			return false;
		}
		return sink.writeBlob( value );
	}
};


} // Anonymous namespace

/**
 *	Constructor.
 */
BlobDataType::BlobDataType( MetaDataType * pMeta ) : StringDataType( pMeta )
{
}


/**
 *	Overrides the DataType method.
 *
 *	@see DataType::getDefaultValue
 */
bool BlobDataType::getDefaultValue( DataSink & output ) const
{
	return output.writeBlob( defaultValue_ );
}


/**
 *	Overrides the DataType method.
 *
 *	@see DataType::addToSection
 */
bool BlobDataType::addToSection( DataSource & source,
		DataSectionPtr pSection ) const
{
	BW::string blobVal;

	if (!source.readBlob( blobVal ))
	{
		return false;
	}
	pSection->setBlob( blobVal );

	return true;
}


/**
 *	Overrides the DataType method.
 *
 *	@see DataType::createFromSection
 */
bool BlobDataType::createFromSection( DataSectionPtr pSection,
		DataSink & sink ) const
{
	return sink.writeBlob( pSection->asBlob() ); // ScriptBlob::create( blobStr );
}


bool BlobDataType::fromStreamToSection( BinaryIStream & stream,
		DataSectionPtr pSection, bool isPersistentOnly ) const
{
	BW::string value;
	stream >> value;
	if (stream.error()) return false;

	pSection->setBlob( value );
	return true;
}


bool BlobDataType::fromSectionToStream( DataSectionPtr pSection,
			BinaryOStream & stream, bool isPersistentOnly ) const
{
	stream << pSection->asBlob();
	return true;
}


void BlobDataType::addToMD5( MD5 & md5 ) const
{
	md5.append( "Blob", sizeof( "Blob" ) );
}


DataType::StreamElementPtr BlobDataType::getStreamElement( size_t index,
	size_t & size, bool & /* isNone */, bool /* isPersistentOnly */ ) const
{
	size = 1;
	if (index == 0)
	{
		return StreamElementPtr( new BlobStreamElement( *this ) );
	}
	return StreamElementPtr();
}


SIMPLE_DATA_TYPE( BlobDataType, BLOB )

BW_END_NAMESPACE

// blob_data_type.cpp
