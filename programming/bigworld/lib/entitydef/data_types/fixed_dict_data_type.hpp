#ifndef FIXED_DICT_DATA_TYPE_HPP
#define FIXED_DICT_DATA_TYPE_HPP

#include "class_data_type.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class is a a combination of ClassDataType and UserDataType. It allows
 * 	the user to specify a dictionary-like property with fixed keys. It also
 * 	optionally allow the user to replace the dictionary with a custom Python
 * 	class, thereby providing functionality similar to UserDataType. If the user
 * 	chooses to replace the dictionary with their own class, they must provide
 * 	a class object which implements the following methods:
 * 		def getDictFromObj( self, obj )
 * 		def createObjFromDict( self, dict )
 * 		def	isSameType( self, obj )
 * 		def addToStream( self, test )			[Optional]
 * 		def createFromStream( self, stream )	[Optional]
 *
 * 	For example, code to implement a "real" dictionary (i.e. non-fixed keys):
 *	<code>
 *	import cPickle
 *
 *	class TestDataType:
 * 		def getDictFromObj( self, obj ):
 * 			list = []
 * 			for i in test.items():
 * 				item = { "key": i[0], "value": i[1] }
 * 				list.append( item )
 * 			return { "list": list }
 *
 * 		def createObjFromDict( self, dict ):
 * 			result = {}
 * 			list = dict["list"]
 * 			for i in list:
 * 				result[i["key"]] = i["value"]
 * 			return result
 *
 * 		def	isSameType( self, obj ):
 * 			return type(obj) is dict
 *
 *		def addToStream( self, test ):						# optional
 *			return cPickle.dumps( test )
 *
 *		def createFromStream( self, stream ):				# optional
 *			return cPickle.loads( stream )
 *
 *	instance = TestDataType()
 *	</code>
 *
 *	@ingroup entity
 */
class PyFixedDictDataInstance;

/**
 *	This class is used to describe FixedDict data types.
 */
class FixedDictDataType : public DataType
{
public:
	typedef ClassDataType::Field	Field;
	typedef ClassDataType::Fields 	Fields;

	FixedDictDataType( MetaDataType* pMeta, Fields& fields, bool allowNone );
	virtual ~FixedDictDataType();

	const BW::string & customTypeName() const { return customTypeName_; }

	void customTypeName( const BW::string & customTypeName )
	{
		customTypeName_ = customTypeName; 
	}

	int getFieldIndex( const BW::string& fieldName ) const
	{
		FieldMap::const_iterator found = fieldMap_.find( fieldName );
		return (found != fieldMap_.end()) ? found->second : -1;
	}

	DataType & getFieldDataType( int index )
	{
		return *(fields_[index].type_);
	}

	const DataType & getFieldDataType( int index ) const
	{
		return *(fields_[index].type_);
	}

	const BW::string& getFieldName( int index ) const
	{
		return fields_[index].name_;
	}

	Fields::size_type getNumFields() const { return fields_.size(); }
	const Fields& getFields() const	{ return fields_; }

	bool allowNone() const 	{ return allowNone_; }

#if defined( SCRIPT_PYTHON )
	void setCustomClassImplementor( const BW::string& moduleName,
			const BW::string& instanceName );
#endif

	// Overrides the DataType method.
	virtual bool isSameType( ScriptObject pValue );
	virtual void reloadScript();
	virtual void clearScript();
	virtual void setDefaultValue( DataSectionPtr pSection );
	virtual bool getDefaultValue( DataSink & output ) const;
	virtual DataSectionPtr pDefaultSection() const;
	virtual int streamSize() const;
	virtual bool addToSection( DataSource & source,
			DataSectionPtr pSection ) const;
	virtual bool createFromSection( DataSectionPtr pSection,
			DataSink & sink ) const;
	virtual void addToMD5( MD5 & md5 ) const;
	virtual StreamElementPtr getStreamElement( size_t index,
		size_t & size, bool & isNone, bool isPersistentOnly ) const;
	virtual int clientSafety() const { return clientSafety_; }
	virtual bool operator<( const DataType & other ) const;
	virtual BW::string typeName() const;
	virtual ScriptObject attach( ScriptObject pObject,
		PropertyOwnerBase * pOwner, int ownerRef );
	virtual void detach( ScriptObject pObject );
	virtual PropertyOwnerBase * asOwner( ScriptObject pObject ) const;

	// Functions to handle PyFixedDictDataInstance
	ScriptObject createDefaultInstance() const;
#if defined( SCRIPT_PYTHON )
	void addInstanceToStream( PyFixedDictDataInstance* pInst,
			BinaryOStream& stream, bool isPersistentOnly ) const;
	bool addMappingObjToStream( ScriptObject pValue, BinaryOStream & stream,
			bool isPersistentOnly ) const;
	ScriptObject createInstanceFromStream( BinaryIStream & stream,
			bool isPersistentOnly ) const;
	bool addInstanceToSection( PyFixedDictDataInstance* pInst,
			DataSectionPtr pSection ) const;
	ScriptObject createInstanceFromSection( DataSectionPtr pSection ) const;
	ScriptObject createInstanceFromMappingObj(
			ScriptObject pyMappingObj ) const;

	// Functions to handle custom class
	void initCustomClassImplOnDemand()
	{
		if (!isCustomClassImplInited_)
		{
			this->setCustomClassFunctions( false );
		}
	}
	void setCustomClassFunctions( bool ignoreFailure );
	bool hasCustomClass() const{ return pImplementor_ != NULL; }
	const BW::string& moduleName() const { return moduleName_; }
	const BW::string& instanceName() const { return instanceName_; }
	bool hasCustomIsSameType() const { return pIsSameTypeFn_ != NULL; }
	bool hasCustomAddToStream() const { return pAddToStreamFn_ != NULL; }
	bool hasCustomCreateFromStream() const
	{
		return pCreateFromStreamFn_ != NULL;
	}
	ScriptObject createCustomClassFromInstance( PyFixedDictDataInstance* pInst )
			const;
	bool isSameTypeCustom( ScriptObject pValue ) const;
	void addToStreamCustom( ScriptObject pValue, BinaryOStream& stream,
		bool isPersistentOnly) const;
	ScriptObject createFromStreamCustom( BinaryIStream & stream,
			bool isPersistentOnly ) const;
	ScriptObject getDictFromCustomClass( ScriptObject pCustomClass ) const;
	ScriptObject createInstanceFromCustomClass( ScriptObject pValue )
			const;
#endif

private:
	Fields::size_type getNumPersistentFields() const;
	Fields::size_type getFieldIndexForPersistentFieldIndex(
		Fields::size_type persistentIndex ) const;

	bool 		allowNone_;
	Fields		fields_;

	typedef BW::map< BW::string, int > FieldMap;
	FieldMap	fieldMap_;

	BW::string customTypeName_;
	BW::string moduleName_;
	BW::string instanceName_;

	DataSectionPtr	pDefaultSection_;

	bool			isCustomClassImplInited_;
	ScriptObject 	pImplementor_;
	ScriptObject 	pGetDictFromObjFn_;
	ScriptObject 	pCreateObjFromDictFn_;
	ScriptObject 	pIsSameTypeFn_;
	ScriptObject 	pAddToStreamFn_;
	ScriptObject 	pCreateFromStreamFn_;

	int				streamSize_;

	int				clientSafety_;
};

BW_END_NAMESPACE

#endif // FIXED_DICT_DATA_TYPE_HPP
