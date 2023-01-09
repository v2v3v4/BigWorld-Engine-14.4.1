#ifndef UNICODE_STRING_DATA_TYPE_HPP
#define UNICODE_STRING_DATA_TYPE_HPP

#include "entitydef/data_type.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This template class is used to represent the Unicode string data type.
 *
 *	@ingroup entity
 */
class UnicodeStringDataType : public DataType
{
public:
	UnicodeStringDataType( MetaDataType * pMeta ) : DataType( pMeta ) {}
	virtual ~UnicodeStringDataType() {}

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

private:
	BW::wstring defaultValue_;
};

BW_END_NAMESPACE

#endif // UNICODE_STRING_DATA_TYPE_HPP
