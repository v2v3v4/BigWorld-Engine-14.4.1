#include "pch.hpp"

#include "mailbox_data_type.hpp"

#include "simple_stream_element.hpp"

#include "cstdmf/md5.hpp"

#include "entitydef/data_sink.hpp"
#include "entitydef/data_source.hpp"
#include "entitydef/data_types.hpp"
#include "entitydef/mailbox_base.hpp"

#include "resmgr/datasection.hpp"


BW_BEGIN_NAMESPACE

typedef SimpleStreamElement<EntityMailBoxRef> MailBoxStreamElement;

/**
 * Constructor
 */
MailBoxDataType::MailBoxDataType( MetaDataType * pMeta ) : DataType( pMeta )
{
}


/**
 *	Overrides the DataType method.
 *
 *	@see DataType::isSameType
 */
bool MailBoxDataType::isSameType( ScriptObject pValue )
{
	if (pValue.isNone())
	{
		return true;
	}

	// Disallow non-None mailboxes with zero-IP - these are usually mailboxes
	// pending resolution of the remote side, that have been allowed to have
	// methods buffered onto them.  An example is cell mailboxes on base
	// entities after calling createCellEntity() etc. but before the
	// onGetCell() callback.

	EntityMailBoxRef ref;
	if (!PyEntityMailBox::reduceToRef( pValue.get(), &ref ))
	{
		return false;
	}

	if (ref.addr.ip == 0)
	{
		WARNING_MSG( "MailBoxDataType::isSameType: "
				"Mailbox for entity %d has not been fully initialised yet "
				"(IP address is zero)\n",
			ref.id );
		return false;
	}

	return true;
}


/**
 *	This method sets the default value for this type.
 *
 *	@see DataType::setDefaultValue
 */
void MailBoxDataType::setDefaultValue( DataSectionPtr pSection )
{
	if (pSection)
	{
		ERROR_MSG( "MailBoxDataType::setDefaultValue: Default value for "
					"mailbox not supported\n" );
	}
}


/**
 *	Overrides the DataType method.
 *
 *	@see DataType::getDefaultValue
 */
bool MailBoxDataType::getDefaultValue( DataSink & output ) const
{
	EntityMailBoxRef defaultVal;
	MailBoxDataType::to( defaultVal, output );
	return true;
}


/*
 *	Overrides the DataType method.
 *
 *	@see DataType::streamSize
 */
int MailBoxDataType::streamSize() const
{
	// TODO: I suspect this is actually fixed size but can't identify
	// an operator<< or operator>> to confirm. How does this class compile?
	// XXX: Murph guesses: sizeof( MailBox )
	return sizeof( EntityMailBoxRef );
}


/**
 *	Overrides the DataType method.
 *
 *	@see DataType::addToSection
 */
bool MailBoxDataType::addToSection( DataSource & source,
		DataSectionPtr pSection ) const
{
	EntityMailBoxRef mbr;
	MailBoxDataType::from( source, mbr );
	MailBoxDataType::to( mbr, pSection );
	return true;
}


/**
 *	Overrides the DataType method.
 *
 *	@see DataType::createFromSection
 */
bool MailBoxDataType::createFromSection( DataSectionPtr pSection,
		DataSink & sink ) const
{
	EntityMailBoxRef mbr;
	MailBoxDataType::from( pSection, mbr );

	MailBoxDataType::to( mbr, sink );
	return true;
}


/**
 *	Overrides the DataType method.
 *
 *	@see DataType::fromStreamToSection
 */
bool MailBoxDataType::fromStreamToSection( BinaryIStream & stream,
		DataSectionPtr pSection, bool /*isPersistentOnly*/ ) const
{
	EntityMailBoxRef mbr;
	MailBoxDataType::from( stream, mbr );
	MailBoxDataType::to( mbr, pSection );
	return true;
}


/**
 *	Overrides the DataType method.
 *
 *	@see DataType::fromSectionToStream
 */
bool MailBoxDataType::fromSectionToStream( DataSectionPtr pSection,
		BinaryOStream & stream, bool /*isPersistentOnly*/ ) const
{
	if (!pSection) return false;
	EntityMailBoxRef mbr;
	MailBoxDataType::from( pSection, mbr );
	MailBoxDataType::to( mbr, stream );
	return true;	// if missing then just added empty mailbox
}


/**
 *	Overrides the DataType method.
 *
 *	@see DataType::addToMD5
 */
void MailBoxDataType::addToMD5( MD5 & md5 ) const
{
	md5.append( "MailBox", sizeof( "MailBox" ) );
}


DataType::StreamElementPtr MailBoxDataType::getStreamElement( size_t index,
	size_t & size, bool & /* isNone */, bool /* isPersistentOnly */ ) const
{
	size = 1;
	if (index == 0)
	{
		return StreamElementPtr( new MailBoxStreamElement( *this ) );
	}
	return StreamElementPtr();
}


bool MailBoxDataType::operator<( const DataType & other ) const
{
	if (this->DataType::operator<( other )) return true;
	if (other.DataType::operator<( *this )) return false;

	// All MailBoxDataTypes are equal, as we have no default value.
	return false;
}


// -----------------------------------------------------------------------------
// Section: Static converters
// -----------------------------------------------------------------------------

void MailBoxDataType::from( ScriptObject pObject, EntityMailBoxRef & mbr )
{
	PyEntityMailBox::reduceToRef( pObject.get(), &mbr );
}

/// Helper method to Python
void MailBoxDataType::to( const EntityMailBoxRef & mbr, ScriptObject & pObject )
{
	pObject = ScriptObject( PyEntityMailBox::constructFromRef( mbr ),
					ScriptObject::STEAL_REFERENCE );
}

/// Helper method from Stream
void MailBoxDataType::from( BinaryIStream & stream, EntityMailBoxRef & mbr )
{
	stream >> mbr;
}

/// Helper method from DataSource
void MailBoxDataType::from( DataSource & source, EntityMailBoxRef & mbr )
{
	MF_VERIFY( source.read( mbr ) );
}

/// Helper method to DataSink
void MailBoxDataType::to( const EntityMailBoxRef & mbr, DataSink & sink )
{
	MF_VERIFY( sink.write( mbr ) );
}

/// Helper method to Stream
void MailBoxDataType::to( const EntityMailBoxRef & mbr, BinaryOStream & stream )
{
	stream << mbr;
}

/// Helper method from Section
void MailBoxDataType::from( DataSectionPtr pSection, EntityMailBoxRef & mbr )
{
	mbr.id = pSection->readInt( "id" );
	mbr.addr.ip = pSection->readInt( "ip" );
	(uint32&)mbr.addr.port = pSection->readInt( "etc" );
}

/// Helper method to Section
void MailBoxDataType::to( const EntityMailBoxRef & mbr, DataSectionPtr pSection )
{
	pSection->writeInt( "id", mbr.id );
	pSection->writeInt( "ip", mbr.addr.ip );
	pSection->writeInt( "etc", (uint32&)mbr.addr.port );
}


/// Datatype for MailBoxes.
SIMPLE_DATA_TYPE( MailBoxDataType, MAILBOX )

BW_END_NAMESPACE

// mailbox_data_type.cpp
