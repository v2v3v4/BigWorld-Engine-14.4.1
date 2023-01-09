#ifndef STRING_DATA_TYPE_HPP
#define STRING_DATA_TYPE_HPP

#include "entitydef/data_type.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This template class is used to represent the string data type.
 *
 *	@ingroup entity
 */
class StringDataType : public DataType
{
public:
	StringDataType( MetaDataType * pMeta ) : DataType( pMeta ) { }
	virtual ~StringDataType() {}

protected:
	virtual bool isSameType( ScriptObject pValue );
	virtual void setDefaultValue( DataSectionPtr pSection );
	virtual bool getDefaultValue( DataSink & output ) const;

	virtual int streamSize() const;

	virtual bool addToSection( DataSource & source, DataSectionPtr pSection )
			const;
	virtual bool createFromSection( DataSectionPtr pSection, DataSink & sink )
			const;

	virtual bool fromStreamToSection( BinaryIStream & stream,
			DataSectionPtr pSection, bool isPersistentOnly ) const;
	virtual bool fromSectionToStream( DataSectionPtr pSection,
			BinaryOStream & stream, bool isPersistentOnly ) const;

	virtual void addToMD5( MD5 & md5 ) const;

	virtual StreamElementPtr getStreamElement( size_t index,
		size_t & size, bool & isNone, bool isPersistentOnly ) const;

	virtual bool operator<( const DataType & other ) const;

protected:
	// To allow BlobDataType to output it.
	BW::string defaultValue_;
};

BW_END_NAMESPACE

#endif // STRING_DATA_TYPE_HPP
