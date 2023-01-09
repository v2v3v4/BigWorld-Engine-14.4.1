#include "pch.hpp"

#include "string_data_type.hpp"

#include "simple_stream_element.hpp"

#include "cstdmf/binary_stream.hpp"
#include "cstdmf/md5.hpp"

#include "resmgr/datasection.hpp"

#include "entitydef/data_sink.hpp"
#include "entitydef/data_source.hpp"
#include "entitydef/data_types.hpp"


BW_BEGIN_NAMESPACE

typedef SimpleStreamElement< BW::string > StringStreamElement;
// -----------------------------------------------------------------------------
// Section: StringDataType
// -----------------------------------------------------------------------------

/**
 *	Overrides the DataType method.
 *
 *	@see DataType::isSameType
 */
bool StringDataType::isSameType( ScriptObject pValue )
{
	return ScriptString::check( pValue );
}


/**
 *	This method sets the default value for this type.
 *
 *	@see DataType::setDefaultValue
 */
void StringDataType::setDefaultValue( DataSectionPtr pSection )
{
	defaultValue_ = (pSection) ? pSection->as< BW::string >() : BW::string();
}


/**
 *	Overrides the DataType method.
 *
 *	@see DataType::getDefaultValue
 */
bool StringDataType::getDefaultValue( DataSink & output ) const
{
	return output.write( defaultValue_ );
}


/*
 *	Overrides the DataType method.
 *
 *	@see DataType::streamSize
 */
int StringDataType::streamSize() const
{
	return -1;
}


/*
 *	Overrides the DataType method.
 */
bool StringDataType::addToSection( DataSource & source,
		DataSectionPtr pSection ) const
{
	BW::string value;

	if (!source.read( value ))
	{
		return false;
	}

	pSection->setString( value );
	return true;
}


/**
 *	Overrides the DataType method.
 *
 *	@see DataType::createFromSection
 */
bool StringDataType::createFromSection( DataSectionPtr pSection,
		DataSink & sink ) const
{
	return sink.write( pSection->asString() );
}


bool StringDataType::fromStreamToSection( BinaryIStream & stream,
		DataSectionPtr pSection, bool /*isPersistentOnly*/ ) const
{
	BW::string value;
	stream >> value;
	if (stream.error()) return false;

	pSection->setString( value );
	return true;
}


bool StringDataType::fromSectionToStream( DataSectionPtr pSection,
			BinaryOStream & stream, bool /*isPersistentOnly*/ ) const
{
	stream << pSection->asString();
	return true;
}


void StringDataType::addToMD5( MD5 & md5 ) const
{
	md5.append( "String", sizeof( "String" ) );
}


DataType::StreamElementPtr StringDataType::getStreamElement( size_t index,
	size_t & size, bool & /* isNone */, bool /* isPersistentOnly */ ) const
{
	size = 1;
	if (index == 0)
	{
		return StreamElementPtr( new StringStreamElement( *this ) );
	}
	return StreamElementPtr();
}


bool StringDataType::operator<( const DataType & other ) const
{
	if (this->DataType::operator<( other )) return true;
	if (other.DataType::operator<( *this )) return false;

	const StringDataType& otherStr =
		static_cast< const StringDataType& >( other );

	return defaultValue_ < otherStr.defaultValue_;
}


/// Datatype for strings.
SIMPLE_DATA_TYPE( StringDataType, STRING )

BW_END_NAMESPACE

// string_data_type.cpp
