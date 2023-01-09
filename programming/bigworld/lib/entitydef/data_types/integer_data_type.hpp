#ifndef INTEGER_DATA_TYPE_HPP
#define INTEGER_DATA_TYPE_HPP

#include "entitydef/data_type.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This template class is used to represent the different types of integer data
 *	type.
 *
 *	@ingroup entity
 */
template <class INT_TYPE>
class IntegerDataType : public DataType
{
public:
	IntegerDataType( MetaDataType * pMeta );

#ifdef _LP64
	typedef int64 NativeLong;
#else
	typedef int NativeLong;
#endif

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

	virtual void addToMD5( MD5 & md5 ) const;

	virtual StreamElementPtr getStreamElement( size_t index,
		size_t & size, bool & isNone, bool isPersistentOnly ) const;

	virtual bool operator<( const DataType & other ) const;

private:
	INT_TYPE defaultValue_;
};

BW_END_NAMESPACE

#endif // INTEGER_DATA_TYPE_HPP
