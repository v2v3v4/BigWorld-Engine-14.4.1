#include "pch.hpp"

#include "sequence_data_type.hpp"

#include "cstdmf/bw_string.hpp"
#include "cstdmf/md5.hpp"
#include "cstdmf/memory_stream.hpp"

#include "entitydef/data_sink.hpp"
#include "entitydef/data_source.hpp"
#include "entitydef/script_data_source.hpp"
#include "entitydef/script_data_sink.hpp"

#include "resmgr/datasection.hpp"


BW_BEGIN_NAMESPACE

// Anonymous namespace
namespace {

/**
 *	This class is the first StreamElement of an ARRAY or TUPLE
 */
class SequenceStartStreamElement : public DataType::StreamElement
{
public:
	SequenceStartStreamElement( const SequenceDataType & type,
		size_t & varSize, bool isPersistentOnly ):
	DataType::StreamElement( type ),
	varSize_( varSize ),
	isPersistentOnly_( isPersistentOnly )
	{};

	bool containerSize( size_t & rSize ) const
	{
		const SequenceDataType & sequenceDataType =
			static_cast< const SequenceDataType & >( this->getType() );
		if (sequenceDataType.getSize() == 0)
		{
			return false;
		}

		rSize = sequenceDataType.getSize();
		return true;
	}

	bool isVariableSizedContainer() const
	{
		const SequenceDataType & sequenceDataType =
			static_cast< const SequenceDataType & >( this->getType() );
		return sequenceDataType.getSize() == 0;
	}

	bool isStreamAsStringStart() const
	{
		const SequenceDataType & sequenceDataType =
			static_cast< const SequenceDataType & >( this->getType() );
		return isPersistentOnly_ && sequenceDataType.dbLen() > 0;
	}

	bool fromSourceToStream( DataSource & source, BinaryOStream & stream ) const
	{
		const SequenceDataType & sequenceDataType =
			static_cast< const SequenceDataType & >( this->getType() );

		// Updates varSize_, but varSize_ may be "0" if unknown.
		bool result = source.beginSequence( varSize_ );

		const int & size = sequenceDataType.getSize();

		if (size == 0)
		{
			// The DataType doesn't know the size, so trust the DataSource
			// Note: 2/current has merged this as stream.writePackedInt
			stream.writePackedInt( static_cast< int >( varSize_ ) );
		}
		else if (varSize_ == 0)
		{
			// The DataSource didn't know the size, but the DataType does.
			varSize_ = size;
		}
		else if (static_cast< int >( varSize_ ) != size)
		{
			// The DataType and the DataSource both claim to know, and they
			// disagree.
			ERROR_MSG( "SequenceStartStreamElement::fromSourceToStream: "
				"Fixed-length sequence reported size %" PRIzu ", but "
				"datatype expected size %d\n", varSize_, size );

			// Ensure we return the right size for stream output
			varSize_ = size;
			result = false;
		}

		return result;
	}

	bool fromStreamToSink( BinaryIStream & stream, DataSink & sink ) const
	{
		const SequenceDataType & sequenceDataType =
			static_cast< const SequenceDataType & >( this->getType() );

		bool result = true;

		varSize_ = sequenceDataType.getSize();

		if (varSize_ == 0)
		{
			int size = stream.readPackedInt();

			if (stream.error())
			{
				ERROR_MSG( "SequenceStartStreamElement::fromStreamToSink: "
						"Missing size parameter on stream\n" );
				result = false;
			}
			varSize_ = size;
		}

		result = result && sequenceDataType.startSequence( sink, varSize_ );

		// Work out whether there's possibly enough data on the stream to
		// create a sequence of this size
		const int elementSize = sequenceDataType.getElemType().streamSize();

		if (elementSize < 0)
		{
			if (stream.remainingLength() < static_cast< int >( varSize_ ))
			{
				ERROR_MSG( "SequenceStartStreamElement::fromSourceToStream: "
					"Invalid size on stream: %" PRIzu " "
					"(%d bytes remaining)\n",
					varSize_, stream.remainingLength() );
				result = false;
			}
		}
		else
		{
			if (stream.remainingLength() <
				static_cast< int >( varSize_ * elementSize ))
			{
				ERROR_MSG( "SequenceStartStreamElement::fromSourceToStream: "
					"Expected bytes on stream: %" PRIzu " "
					"(%d bytes remaining)\n",
					varSize_ * elementSize, stream.remainingLength() );
				result = false;
			}
		}

		return result;
	}

private:
	// This will be a reference to DataType::const_iterator's size_ element when
	// used with an iterator.
	size_t & varSize_;
	const bool isPersistentOnly_;
};

/**
 *	This class is the StreamElement for each item of an ARRAY or TUPLE
 */
class SequenceEnterItemStreamElement : public DataType::StreamElement
{
public:
	SequenceEnterItemStreamElement( const SequenceDataType & type,
		size_t fieldIndex ) :
	DataType::StreamElement( type ),
	fieldIndex_( fieldIndex )
	{}

	bool childType( ConstDataTypePtr & rpChildType ) const
	{
		const SequenceDataType & sequenceDataType =
			static_cast< const SequenceDataType & >( this->getType() );
		rpChildType = &sequenceDataType.getElemType();
		return true;
	}

	bool fromSourceToStream( DataSource & source, BinaryOStream & stream ) const
	{
		return source.enterItem( fieldIndex_ );
	}

	bool fromStreamToSink( BinaryIStream & stream, DataSink & sink ) const
	{
		return sink.enterItem( fieldIndex_ );
	}

private:
	size_t fieldIndex_;
};

/**
 *	This class is the StreamElement after each item of an ARRAY or TUPLE
 */
class SequenceLeaveItemStreamElement : public DataType::StreamElement
{
public:
	SequenceLeaveItemStreamElement( const SequenceDataType & type ) :
	DataType::StreamElement( type )
	{}

	bool fromSourceToStream( DataSource & source, BinaryOStream & stream ) const
	{
		return source.leaveItem();
	}

	bool fromStreamToSink( BinaryIStream & stream, DataSink & sink ) const
	{
		return sink.leaveItem();
	}
};

/**
 *	This class is the last StreamElement of an ARRAY or TUPLE
 */
class SequenceEndStreamElement : public DataType::StreamElement
{
public:
	SequenceEndStreamElement( const SequenceDataType & type,
		bool isPersistentOnly ) :
	DataType::StreamElement( type ),
	isPersistentOnly_( isPersistentOnly )
	{}

	bool isStreamAsStringEnd() const
	{
		const SequenceDataType & sequenceDataType =
			static_cast< const SequenceDataType & >( this->getType() );
		return isPersistentOnly_ && sequenceDataType.dbLen() > 0;
	}

	bool fromSourceToStream( DataSource & source, BinaryOStream & stream ) const
	{
		return true;
	}

	bool fromStreamToSink( BinaryIStream & stream, DataSink & sink ) const
	{
		return true;
	}
private:
	const bool isPersistentOnly_;
};

} // Anonymous namespace

// -----------------------------------------------------------------------------
// Section: SequenceDataType
// -----------------------------------------------------------------------------

SequenceDataType::SequenceDataType( MetaDataType * pMeta,
		DataTypePtr elementTypePtr,
		int size,
		int dbLen,
		bool isConst ) :
	DataType( pMeta, isConst && elementTypePtr->isConst() ),
	elementTypePtr_( elementTypePtr ),
	elementType_( *elementTypePtr ),
	size_( size ),
	dbLen_( dbLen ),
	streamSize_( size_ * elementType_.streamSize() )
{
	// note: our meta data type will not always be SequenceMetaDataType,
	// it could be e.g. SimpleMetaDataType used by PatrolPath.

	// Either variable-length, or contains variable-length elements
	if (streamSize_ <= 0)
	{
		streamSize_ = -1;
	}
}


/**
 *	Overrides the DataType method.
 *
 *	@see DataType::isSameType
 */
bool SequenceDataType::isSameType( ScriptObject pValue )
{
	if (!ScriptSequence::check( pValue ))
	{
		return false;
	}

	ScriptSequence sequence( pValue );

	const int size = static_cast<int>(sequence.size());

	if ((size_ != 0) && (size != size_))
	{
		ERROR_MSG( "SequenceDataType::isSameType: "
				"Wrong size %d instead of %d.\n",
			size, size_ );
		return false;
	}

	for (int i = 0; i < size; i++)
	{
		ScriptObject pElement = sequence.getItem( i,
			ScriptErrorPrint( "SequenceDataType::isSameType" ) );

		bool isOkay = elementType_.isSameType( pElement );

		if (!isOkay)
		{
			ERROR_MSG( "SequenceDataType::isSameType: "
				"Bad element %d. Expected type %s, but element was \"%s\".\n",
				i,
				elementType_.typeName().c_str(),
				pElement.str( ScriptErrorClear() ).c_str() );
			return false;
		}
	}

	return true;
}

/**
 *	Creates a default sequence i.e. empty sequence or sequence with correct
 * 	number of default elements if &lt;size&gt; is specified.
 */
bool SequenceDataType::createDefaultValue( DataSink & output ) const
{
	if (!this->startSequence( output, size_ ))
	{
		return false;
	}

	bool isOkay = true;

	for (int i = 0; isOkay && i < size_; ++i)
	{
		isOkay &= output.enterItem( i ) &&
			elementType_.getDefaultValue( output ) &&
			output.leaveItem();
	}

	return isOkay;
}


/*
 *	Overrides the DataType method.
 *
 *	@see DataType::streamSize
 */
int SequenceDataType::streamSize() const
{
	return streamSize_;
}


/**
 *	Overrides the DataType method.
 *
 *	@see DataType::addToSection
 */
bool SequenceDataType::addToSection( DataSource & source,
	DataSectionPtr pSection ) const
{
	int size;

	size_t bigSize;

	if (!source.beginSequence( bigSize ))
	{
		return false;
	}

	if (size_ == 0)
	{
		size = (int)bigSize;
	}
	else
	{
		if (bigSize != 0 && (int)bigSize != size_)
		{
			ERROR_MSG( "SequenceDataType::addToSection: "
					"Fixed-length sequence reported size %" PRIzu ", but "
					"datatype expected size %d\n", bigSize, size_ );
			return false;
		}

		size = size_;
	}

	bool isOkay = true;

	for (int i = 0; isOkay && i < size; ++i)
	{
		DataSectionPtr itemSection = pSection->newSection( "item" );
		isOkay &= source.enterItem( i ) &&
			elementType_.addToSection( source, itemSection ) &&
			source.leaveItem();
	}

	return isOkay;
}

/**
 *	Overrides the DataType method.
 *
 *	@see DataType::createFromSection
 */
bool SequenceDataType::createFromSection( DataSectionPtr pSection,
	DataSink & sink ) const
{
	if (!pSection)
	{
		ERROR_MSG( "SequenceDataType::createFromSection: "
				"Section is NULL.\n" );
		return false;
	}

	int size = pSection->countChildren();
	int children = size;

	IF_NOT_MF_ASSERT_DEV( (size_ == 0) || (size == size_) )
	{
		size = size_;
	}

	if (!this->startSequence( sink, size ))
	{
		return false;
	}

	bool isOkay = true;

	for (int i = 0; isOkay && i < size; ++i)
	{
		isOkay &= sink.enterItem( i );

		if (i < children)
		{
			isOkay &= elementType_.createFromSection( pSection->openChild( i ),
				sink );
		}
		else
		{
			isOkay &= elementType_.getDefaultValue( sink );
		}

		isOkay &= sink.leaveItem();
	}

	return isOkay;
}

/**
 *	Overrides the DataType method.
 *
 *	@see DataType::fromStreamToSection
 */
bool SequenceDataType::fromStreamToSection( BinaryIStream & stream,
		DataSectionPtr pSection, bool isPersistentOnly ) const
{
	if (isPersistentOnly && (dbLen_ > 0))
	{
		// When receiving from DB and was stored in DB as a blob, unwrap our
		// stream from a string.
		int len = stream.readPackedInt();
		MemoryIStream stream2( stream.retrieve( len ), len );
		return this->fromStreamToSectionInternal( stream2, pSection,
													isPersistentOnly );
	}
	else
	{
		return this->fromStreamToSectionInternal( stream, pSection,
													isPersistentOnly );
	}
}

/**
 * 	@see DataType::fromStreamToSection
 */
bool SequenceDataType::fromStreamToSectionInternal( BinaryIStream & stream,
		DataSectionPtr pSection, bool isPersistentOnly ) const
{
	int size = size_;

	if (size == 0)
	{
		size = stream.readPackedInt();
		if (stream.error()) return false;
	}

	BW::vector< BW::string > dummy;
	dummy.resize( size );
	pSection->writeStrings( "item", dummy );

	BW::vector< DataSectionPtr > sections;
	pSection->openSections( "item", sections );

	bool ok = true;
	for (int i = 0; i < size && ok; i++)
	{
		ok &= elementType_.fromStreamToSection( stream, sections[ i ],
												isPersistentOnly );
	}
	return ok;
}

/**
 *	Overrides the DataType method.
 *
 *	@see DataType::fromSectionToStream
 */
bool SequenceDataType::fromSectionToStream( DataSectionPtr pSection,
		BinaryOStream & stream, bool isPersistentOnly ) const
{
	if (isPersistentOnly && (dbLen_ > 0))
	{
		// When sending to DB and storing in DB as a blob, wrap our stream
		// in a string
		MemoryOStream stream2;
		bool isOK = this->fromSectionToStreamInternal( pSection, stream2,
														isPersistentOnly );
		if (isOK)
		{
			int len = stream2.remainingLength();
			stream.appendString( (char *) stream2.retrieve(len), len );
		}
		return isOK;
	}
	else
	{
		return this->fromSectionToStreamInternal( pSection, stream,
													isPersistentOnly );
	}

}

/**
 *	@see DataType::fromSectionToStream
 */
bool SequenceDataType::fromSectionToStreamInternal( DataSectionPtr pSection,
		BinaryOStream & stream, bool isPersistentOnly ) const
{
	// TODO: Always add _something_ to the stream.
	if (!pSection)
	{
		ERROR_MSG( "SequenceDataType::fromSectionToStream: "
				"Section is NULL.\n" );
		return false;
	}

	int size = pSection->countChildren();
	int children = size;

	MF_ASSERT_DEV( (size_ == 0) || (size == size_) );
	if (size_ == 0)
	{
		stream.writePackedInt( size );
	}
	else if (size != size_)
	{
		size = size_;
	}

	for (int i = 0; i < size; i++)
	{
		if (i < children)
		{
			if ( !elementType_.fromSectionToStream(
							pSection->openChild( i ), stream, isPersistentOnly ) )
				return false;
		}
		else
		{
			// TODO: Not this.
			ScriptDataSink sink;
			if (!elementType_.getDefaultValue( sink ))
			{
				return false;
			}
			ScriptObject temp = sink.finalise();
			ScriptDataSource source( temp );
			if (!elementType_.addToStream( source, stream, isPersistentOnly ))
			{
				return false;
			}
		}
	}

	return true;
}

void SequenceDataType::addToMD5( MD5 & md5 ) const
{
	//SequenceDataType is the base class, the actual type names are added in
	//children classes like TupleDataType or ArrayDataType, not here.
	md5.append( &size_, sizeof( size_ ) );

	if (dbLen_ > 0)
	{
		md5.append( &dbLen_, sizeof( dbLen_ ) );
	}

	elementType_.addToMD5( md5 );

}


DataType::StreamElementPtr SequenceDataType::getStreamElement( size_t index,
	size_t & size, bool & /* isNone */, bool isPersistentOnly ) const
{
	if (index == 0)
	{
		size = this->getSize();
		// The size reference will be updated again by a routine that reads from
		// a DataSource or BinaryIStream.
		return StreamElementPtr(
			new SequenceStartStreamElement( *this, size, isPersistentOnly ) );
	}
	else if (index < (size * 2) + 1)
	{
		size_t fieldIndex = ( index - 1 ) / 2;
		if (index % 2 == 1)
		{
			return StreamElementPtr(
				new SequenceEnterItemStreamElement( *this, fieldIndex ) );
		}
		else
		{
			return StreamElementPtr(
				new SequenceLeaveItemStreamElement( *this ) );
		}
	}
	else if (index == (size * 2) + 1)
	{
		return StreamElementPtr(
			new SequenceEndStreamElement( *this, isPersistentOnly ) );
	}

	return StreamElementPtr();
}


bool SequenceDataType::operator<( const DataType & other ) const
{
	if (this->DataType::operator<( other )) return true;
	if (other.DataType::operator<( *this )) return false;

	// ok, equal metas, so downcast other and compare with us
	SequenceDataType & otherSeq = (SequenceDataType&)other;
	if (size_ < otherSeq.size_) return true;
	if (otherSeq.size_ < size_) return false;

	// Check dbLen
	if (dbLen_ < otherSeq.dbLen_) return true;
	if (otherSeq.dbLen_ < dbLen_) return false;

	// Now, let our element types fight it out
	if (elementType_ < otherSeq.elementType_) return true;
	if (otherSeq.elementType_ < elementType_) return false;

	return this->compareDefaultValue( other ) < 0;
}

BW::string SequenceDataType::typeName() const
{
	BW::stringstream stream;

	stream << this->DataType::typeName();

	if (size_ != 0)
	{
		stream << " size " << size_;
	}

	stream << " of (" << elementType_.typeName() << ")";

	return stream.str();
}

BW_END_NAMESPACE

// sequence_data_type.cpp
