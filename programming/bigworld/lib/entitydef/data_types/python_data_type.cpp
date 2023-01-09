#include "pch.hpp"

#include "python_data_type.hpp"

#include "cstdmf/binary_stream.hpp"
#include "cstdmf/md5.hpp"

#include "resmgr/datasection.hpp"

#include "script/pickler.hpp"

#include "entitydef/data_sink.hpp"
#include "entitydef/data_source.hpp"
#include "entitydef/data_types.hpp"
#include "entitydef/script_data_sink.hpp"
#include "entitydef/script_data_source.hpp"


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: PythonDataType
// -----------------------------------------------------------------------------
class PythonDataType;

// Anonymous namespace
namespace {

/**
 *	This class provides a DataType::StreamElement implementation for use by
 *	PythonDataType.
 */
class PythonStreamElement : public DataType::StreamElement
{
public:
	PythonStreamElement( const PythonDataType & type ) :
		DataType::StreamElement( type )
	{};

	bool fromSourceToStream( DataSource & source, BinaryOStream & stream ) const
	{
		ScriptObject pNewValue;

		// TODO: Eww...
		ScriptDataSource & scriptSource =
			static_cast< ScriptDataSource & >( source );

		if (!scriptSource.read( pNewValue ))
		{
			return false;
		}

		BW::string pickled = PythonDataType::pickler().pickle( pNewValue );
		stream << pickled;
		return !pickled.empty();
	}

	bool fromStreamToSink( BinaryIStream & stream, DataSink & sink ) const
	{
		// TODO: Eww...
		ScriptDataSink & scriptSink =
			static_cast< ScriptDataSink & >( sink );

		BW::string value;
		stream >> value;

		// Check for zero length as unpickle doesn't work correctly and it
		// will cause a crash.
		if (stream.error() || (value.length() <= 0))
		{
			ERROR_MSG( "PythonStreamElement::fromStreamToSink: "
				"Not enough data on stream to read value\n" );
			return false;
		}

		return scriptSink.write( PythonDataType::pickler().unpickle( value ) );
	}
};


} // Anonymous namespace

/**
 *	Constructor.
 */
PythonDataType::PythonDataType( MetaDataType * pMeta ) :
	DataType( pMeta, /*isConst:*/false )
{
}


/**
 *	Overrides the DataType method.
 *
 *	@see DataType::isSameType
 */
bool PythonDataType::isSameType( ScriptObject pValue )
{
	// TODO: We shouldn't have to pickle the object twice.
	return !pickler().pickle( pValue ).empty();
}


/**
 *	This method sets the default value for this type.
 *
 *	@see DataType::setDefaultValue
 */
void PythonDataType::setDefaultValue( DataSectionPtr pSection )
{
	pDefaultSection_ = pSection;
}


/**
 *	Overrides the DataType method.
 *
 *	@see DataType::pDefaultValue
 */
bool PythonDataType::getDefaultValue( DataSink & output ) const
{
	// TODO: Eww...
	ScriptDataSink & scriptOutput =
		static_cast< ScriptDataSink & >( output );
	return (pDefaultSection_) ?
		this->createFromSection( pDefaultSection_, output ) :
		scriptOutput.write( ScriptObject::none() );
}


/**
 *	Overrides the DataType method.
 *
 *	@see DataType::pDefaultSection
 */
DataSectionPtr PythonDataType::pDefaultSection() const
{
	return pDefaultSection_;
}


/*
 *	Overrides the DataType method.
 *
 *	@see DataType::streamSize
 */
int PythonDataType::streamSize() const
{
	return -1;
}


/**
 *	Overrides the DataType method.
 *
 *	@see DataType::addToSection
 */
bool PythonDataType::addToSection( DataSource & source,
		DataSectionPtr pSection ) const
{
	ScriptObject pNewValue;

	// TODO: Eww...
	ScriptDataSource & scriptSource =
		static_cast< ScriptDataSource & >( source );

	if (!scriptSource.read( pNewValue ))
	{
		ERROR_MSG( "PythonDataType::addToSection: "
				"pickle failed to get object\n" );
		return false;
	}

	BW::string pickled = this->pickler().pickle( pNewValue );

	if (pickled.empty())
	{
		ERROR_MSG( "PythonDataType::addToSection: "
				"pickle failed for object of type %s\n",
			pNewValue.typeNameOfObject() );
		return false;
	}

	pSection->setBlob( pickled );
	return true;
}


/**
 *	Overrides the DataType method.
 *
 *	@see DataType::createFromSection
 */
bool PythonDataType::createFromSection( DataSectionPtr pSection,
		DataSink & sink ) const
{
	// TODO: Eww...
	ScriptDataSink & scriptSink =
		static_cast< ScriptDataSink & >( sink );

	if (!pSection)
	{
		ERROR_MSG( "PythonDataType::createFromSection: "
			"pSection = NULL\n" );
		return false;
	}

	BW::string value = pSection->asString();

	if (PythonDataType::isExpression( value ))
	{
		ScriptObject pResult( Script::runString( value.c_str(), false ),
			ScriptObject::STEAL_REFERENCE );

		if (!pResult)
		{
			ERROR_MSG( "PythonDataType::createFromSection: "
				"Failed to evaluate '%s'\n", value.c_str() );
			PyErr_Print();
			return false;
		}

		return scriptSink.write( pResult );
	}
	else
	{
		return scriptSink.write( pickler().unpickle( pSection->asBlob() ) );
	}
}


bool PythonDataType::fromStreamToSection( BinaryIStream & stream,
		DataSectionPtr pSection, bool isPersistentOnly ) const
{
	BW::string value;
	stream >> value;
	if (stream.error()) return false;

	pSection->setBlob( value );
	return true;
}


bool PythonDataType::fromSectionToStream( DataSectionPtr pSection,
					BinaryOStream & stream, bool isPersistentOnly ) const
{
	if (!pSection)
	{
		return false;
	}

	BW::string value = pSection->asBlob();

	stream.appendString( value.data(), static_cast<int>(value.length()) );

	return true;
}


void PythonDataType::addToMD5( MD5 & md5 ) const
{
	md5.append( "Python", sizeof( "Python" ) );
}


DataType::StreamElementPtr PythonDataType::getStreamElement( size_t index,
	size_t & size, bool & /* isNone */, bool /* isPersistentOnly */ ) const
{
	size = 1;
	if (index == 0)
	{
		return StreamElementPtr( new PythonStreamElement( *this ) );
	}
	return StreamElementPtr();
}


bool PythonDataType::operator<( const DataType & other ) const
{
	if (this->DataType::operator<( other )) return true;
	if (other.DataType::operator<( *this )) return false;

	const PythonDataType& otherPy =
		static_cast< const PythonDataType& >( other );
	if (otherPy.pDefaultSection_)
		return otherPy.pDefaultSection_->compare( pDefaultSection_ ) > 0;
	// else we are equal or greater than other.
	return false;
}


Pickler & PythonDataType::pickler()
{
	static Pickler pickler;

	return pickler;
}


/**
 *	This function has a good guess as to whether the given value is
 * 	a Python expression.
 */
bool PythonDataType::isExpression( const BW::string& value )
{
	// Since PYTHON values in DataSections can either be a Python
	// expression or a Base64 encoded pickled object, we return
	// true if it doesn't look like a Base64 encoded blob.

	return (!value.empty() && (*(value.rbegin()) != '='));
}


/// Datatype for strings.
SIMPLE_DATA_TYPE( PythonDataType, PYTHON )

BW_END_NAMESPACE

// python_data_type.cpp
