#ifndef FLOAT_DATA_TYPES_HPP
#define FLOAT_DATA_TYPES_HPP

#include "entitydef/data_type.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class is used to represent a float data type.
 *
 *	@ingroup entity
 */
template <class FLOAT_TYPE>
class FloatDataType : public DataType
{
public:
	FloatDataType( MetaDataType * pMeta );

protected:
	virtual bool isSameType( ScriptObject pValue );
	virtual void setDefaultValue( DataSectionPtr pSection );

	virtual bool getDefaultValue( DataSink & output ) const;

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

private:
	FLOAT_TYPE defaultValue_;
};

BW_END_NAMESPACE

#endif // FLOAT_DATA_TYPES_HPP
