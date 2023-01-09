#ifndef BLOB_DATA_TYPE_HPP
#define BLOB_DATA_TYPE_HPP

#include "string_data_type.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class is used to represent a blob data type. It is similar to the
 *	string data type but writes itself differently to sections.
 */
class BlobDataType : public StringDataType
{
public:
	BlobDataType( MetaDataType * pMeta );

protected:
	virtual bool getDefaultValue( DataSink & output ) const;

	virtual bool addToSection( DataSource & source, DataSectionPtr pSection )
			const;
	virtual bool createFromSection( DataSectionPtr pSection,
			DataSink & sink ) const;

	virtual bool fromStreamToSection( BinaryIStream & stream,
			DataSectionPtr pSection, bool isPersistentOnly ) const;
	virtual bool fromSectionToStream( DataSectionPtr pSection,
				BinaryOStream & stream, bool isPersistentOnly ) const;

	virtual void addToMD5( MD5 & md5 ) const;

	virtual StreamElementPtr getStreamElement( size_t index,
		size_t & size, bool & isNone, bool isPersistentOnly ) const;
};

BW_END_NAMESPACE

#endif // BLOB_DATA_TYPE_HPP
