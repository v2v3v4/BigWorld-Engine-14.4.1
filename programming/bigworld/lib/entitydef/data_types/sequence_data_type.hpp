#ifndef SEQUENCE_DATA_TYPE_HPP
#define SEQUENCE_DATA_TYPE_HPP

#include "entitydef/data_type.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This base class is used to represent sequence data type. This includes
 *	tuples and lists.
 */
class SequenceDataType : public DataType
{
public:
	SequenceDataType( MetaDataType * pMeta,
			DataTypePtr elementTypePtr,
			int size,
			int dbLen,
			bool isConst );

	int getSize() const
	{
		return size_;
	}

	int dbLen() const
	{
		return dbLen_;
	}

	DataType& getElemType() const
	{
		return elementType_;
	}

	virtual int clientSafety() const { return elementType_.clientSafety(); }

	/**
	 *	This method is overridden by implementation classes to prepare the
	 *	the DataSink for the specific sequence type they support.
	 *
	 *	@param sink				The DataSink the sequence will be written to.
	 *	@param count			The number of elements that will be written.
	 */
	virtual bool startSequence( DataSink & sink, size_t count ) const = 0;

protected:
	virtual int compareDefaultValue( const DataType & other ) const = 0;

	bool createDefaultValue( DataSink & output ) const;

	// Overrides the DataType method.
	virtual bool isSameType( ScriptObject pValue );
	virtual int streamSize() const;
	virtual bool addToSection( DataSource & source,
		DataSectionPtr pSection ) const;
	virtual bool createFromSection( DataSectionPtr pSection,
		DataSink & sink ) const;
	virtual bool fromStreamToSection( BinaryIStream & stream,
		DataSectionPtr pSection, bool isPersistentOnly ) const;
	virtual bool fromSectionToStream( DataSectionPtr pSection,
		BinaryOStream & stream, bool isPersistentOnly ) const;
	virtual void addToMD5( MD5 & md5 ) const;
	virtual StreamElementPtr getStreamElement( size_t index,
		size_t & size, bool & isNone, bool isPersistentOnly ) const;
	virtual bool operator<( const DataType & other ) const;
	virtual BW::string typeName() const;

private:
	bool fromStreamToSectionInternal( BinaryIStream & stream,
		DataSectionPtr pSection, bool isPersistentOnly ) const;
	bool fromSectionToStreamInternal( DataSectionPtr pSection,
		BinaryOStream & stream, bool isPersistentOnly ) const;

	DataTypePtr elementTypePtr_;
	DataType & elementType_;
	int size_;
	int dbLen_;
	int streamSize_;
};

BW_END_NAMESPACE

#endif // SEQUENCE_DATA_TYPE_HPP
