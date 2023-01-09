#include "pch.hpp"

#include "unicode_string_data_type.hpp"

#include "cstdmf/binary_stream.hpp"
#include "cstdmf/md5.hpp"
#include "cstdmf/string_utils.hpp"

#include "resmgr/datasection.hpp"

#include "entitydef/data_sink.hpp"
#include "entitydef/data_source.hpp"
#include "entitydef/data_types.hpp"


BW_BEGIN_NAMESPACE

// Anonymous namespace
namespace {

/**
 *	This class provides a DataType::StreamElement implementation for use by
 *	UnicodeStringDataType.
 */
class UnicodeStringStreamElement : public DataType::StreamElement
{
public:
	UnicodeStringStreamElement( const UnicodeStringDataType & type ) :
		DataType::StreamElement( type )
	{};

	bool fromSourceToStream( DataSource & source, BinaryOStream & stream ) const
	{
		BW::wstring wideString;

		if (!source.read( wideString ))
		{
			stream << "";
			return false;
		}

		BW::string utf8String;

		if (!bw_wtoutf8( wideString, utf8String ))
		{
			ERROR_MSG( "UnicodeStringStreamElement::fromSourceToStream: "
				"Could not convert to UTF8.\n" );
			stream << "";
			return false;
		}

		stream << utf8String;
		return true;
	}

	bool fromStreamToSink( BinaryIStream & stream, DataSink & sink ) const
	{
		BW::string utf8String;

		stream >> utf8String;

		BW::wstring wideString;

		if (!bw_utf8tow( utf8String, wideString ))
		{
			ERROR_MSG( "UnicodeStringStreamElement::fromStreamToSink: "
				"Could not convert from UTF8.\n" );
			return false;
		}

		return sink.write( wideString );
	}
};


} // Anonymous namespace

// -----------------------------------------------------------------------------
// Section: UnicodeStringDataType
// -----------------------------------------------------------------------------

/**
 *	Return true if the given Python object is compatible with this type.
 *
 *	@param pValue 	the Python object
 */
bool UnicodeStringDataType::isSameType( ScriptObject pValue )
{
	return PyUnicode_Check( pValue.get() );
}


/**
 *	Set the default value for this type.
 *
 *	@param pSection 	the data section containing the default value
 */
void UnicodeStringDataType::setDefaultValue( DataSectionPtr pSection )
{
	defaultValue_ = (pSection) ? pSection->as< BW::wstring >() : BW::wstring();
}


/**
 *	Output the default UNICODE_STRING value for this type.
 */
bool UnicodeStringDataType::getDefaultValue( DataSink & output ) const
{
	return output.write( defaultValue_ );
}


/*
 *	Overrides the DataType method.
 *
 *	@see DataType::streamSize
 */
int UnicodeStringDataType::streamSize() const
{
	return -1;
}


/**
 *	Add a UNICODE_STRING type instance to a data section.
 *
 *	@param source 			The DataSink to read from
 *	@param pSection 		The data section to add to.
 */
bool UnicodeStringDataType::addToSection( DataSource & source,
		DataSectionPtr pSection ) const
{
	BW::wstring wideString;

	if (!source.read( wideString ))
	{
		return false;
	}

	BW::string utf8String;

	if (!bw_wtoutf8( wideString, utf8String ))
	{
		ERROR_MSG( "UnicodeStringDataType::addToSection: "
				"Could not convert to UTF8.\n" );
		return false;
	}

	pSection->setString( utf8String );
	return true;
}


/**
 *	Create a UNICODE_STRING type instance from a data section.
 *
 *	@param pSection 	The data section.
 *	@param sink		 	The DataSink to write the string to.
 */
bool UnicodeStringDataType::createFromSection( DataSectionPtr pSection,
		DataSink & sink ) const
{
	// We don't use asWideString() as it doesn't report UTF-8 decoding errors.
	BW::string utf8String = pSection->asString();

	BW::wstring wideString;

	if (!bw_utf8tow( utf8String, wideString ))
	{
		ERROR_MSG( "UnicodeStringDataType::createFromSection: "
				"Could not convert from UTF8.\n" );
		return false;
	}

	return sink.write( wideString );
}


/**
 *	Convert a streamed UNICODE_STRING data instance to a data section
 *	representation.
 *
 *	@param stream 				The stream containing the data instance.
 *	@param pSection 			The section to add to.
 *	@param isPersistentOnly 	Whether the property is persistent or not.
 */
bool UnicodeStringDataType::fromStreamToSection( BinaryIStream & stream,
		DataSectionPtr pSection, bool isPersistentOnly ) const
{
	// Should be OK, both the network format and data section format are UTF-8
	BW::string utf8String;
	stream >> utf8String;
	if (stream.error()) return false;

	pSection->setString( utf8String );
	return true;
}


/**
 *	Convert a UNICODE_STRING data instance contained in a data section to a
 *	stream.
 *
 *	@param pSection 			The data section.
 *	@param stream 				The stream.
 *	@param isPersistentOnly		Whether the property is persistent only.
 */
bool UnicodeStringDataType::fromSectionToStream( DataSectionPtr pSection,
		BinaryOStream & stream, bool isPersistentOnly ) const
{
	// Should be OK, both the network format and data section format are UTF-8
	stream << pSection->asString();
	return true;
}


/**
 *	Add this data type description to an MD5 digest.
 */
void UnicodeStringDataType::addToMD5( MD5 & md5 ) const
{
	static const char tag[] = "UnicodeString";
	md5.append( tag, sizeof( tag ) );
}


DataType::StreamElementPtr UnicodeStringDataType::getStreamElement(
	size_t index, size_t & size, bool & /* isNone */,
	bool /* isPersistentOnly */ ) const
{
	size = 1;
	if (index == 0)
	{
		return StreamElementPtr( new UnicodeStringStreamElement( *this ) );
	}
	return StreamElementPtr();
}


/**
 *	Comparison operator for this data type.
 *
 *	@param other 		The other data type to compare with.
 */
bool UnicodeStringDataType::operator<( const DataType & other ) const
{
	if (this->DataType::operator<( other )) return true;
	if (other.DataType::operator<( *this )) return false;

	const UnicodeStringDataType & otherStr =
		static_cast< const UnicodeStringDataType & >( other );

	return defaultValue_ < otherStr.defaultValue_;
}


SIMPLE_DATA_TYPE( UnicodeStringDataType, UNICODE_STRING )

BW_END_NAMESPACE

// unicode_string_data_type.cpp
