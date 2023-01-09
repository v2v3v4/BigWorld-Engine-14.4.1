#ifndef PYTHON_DATA_TYPE_HPP
#define PYTHON_DATA_TYPE_HPP

#include "entitydef/data_type.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This template class is used to represent the data type of any Python object.
 *	It is only meant to be temporary so that we can support any generic Python
 *	object/type.
 *
 *	@ingroup entity
 */
class PythonDataType : public DataType
{
public:
	PythonDataType( MetaDataType * pMeta );

protected:
	virtual bool isSameType( ScriptObject pValue );

	virtual void setDefaultValue( DataSectionPtr pSection );

	virtual bool getDefaultValue( DataSink & output ) const;

	virtual DataSectionPtr pDefaultSection() const;

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

	virtual int clientSafety() const { return CLIENT_UNSAFE; }

	virtual bool operator<( const DataType & other ) const;

public:
	static bool isExpression( const BW::string& value );

	static Pickler & pickler();

private:
	DataSectionPtr pDefaultSection_;
};

BW_END_NAMESPACE

#endif // PYTHON_DATA_TYPE_HPP
