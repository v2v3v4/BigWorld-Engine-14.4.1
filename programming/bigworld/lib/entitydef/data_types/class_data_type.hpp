#ifndef CLASS_DATA_TYPE_HPP
#define CLASS_DATA_TYPE_HPP

#include "entitydef/data_type.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This data type contains named properties like an entity class does.
 */
class ClassDataType : public DataType
{
public:

	/**
	 *	This structure is used to represent a field in a class.
	 */
	class Field
	{
	public:
		bool createDefaultValue( DataSectionPtr pDefaultSection,
			DataSink & output ) const;

		BW::string	name_;
		DataTypePtr	type_;
		int			dbLen_;
		bool		isPersistent_;
	};
	typedef BW::vector<Field>	Fields;
	typedef Fields::const_iterator Fields_iterator;


	ClassDataType( MetaDataType * pMeta, Fields & fields, bool allowNone );

	// Overrides the DataType method.
	virtual bool isSameType( ScriptObject pValue );
	virtual void setDefaultValue( DataSectionPtr pSection );
	virtual bool getDefaultValue( DataSink & output ) const;
	virtual DataSectionPtr pDefaultSection() const;
	virtual int streamSize() const;
	virtual bool addToSection( DataSource & source, DataSectionPtr pSection )
		const;
	virtual bool createFromSection( DataSectionPtr pSection, DataSink & sink )
		const;
	virtual ScriptObject attach( ScriptObject pObject,
		PropertyOwnerBase * pOwner, int ownerRef );
	virtual void detach( ScriptObject pObject );
	virtual PropertyOwnerBase * asOwner( ScriptObject pObject ) const;
	virtual void addToMD5( MD5 & md5 ) const;
	virtual StreamElementPtr getStreamElement( size_t index,
		size_t & size, bool & isNone, bool isPersistentOnly ) const;
	virtual int clientSafety() const { return clientSafety_; }
	virtual bool operator<( const DataType & other ) const;
	virtual BW::string typeName() const;

public:

	const Fields & fields()	const	{ return fields_; }
	const Fields & getFields() const	{ return fields_; }	// same as above

	int fieldIndexFromName( const char * name ) const;

	bool allowNone() const 	{ return allowNone_; }

private:
	Fields		fields_;

	typedef BW::map<BW::string,int> FieldMap;
	FieldMap	fieldMap_;

	bool		allowNone_;

	DataSectionPtr	pDefaultSection_;

	int 		clientSafety_;
};

BW_END_NAMESPACE

#endif // CLASS_DATA_TYPE_HPP
