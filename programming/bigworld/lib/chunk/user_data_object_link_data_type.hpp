#ifndef USER_DATA_OBJECT_LINK_DATA_TYPE_HPP
#define USER_DATA_OBJECT_LINK_DATA_TYPE_HPP

#include "entitydef/data_description.hpp"

#ifdef EDITOR_ENABLED
// This is only ever defined by world editor, but everything else needs to stub it
#define DEFINE_CREATE_EDITOR_PROPERTY_STUB											\
	GeneralProperty * UserDataObjectLinkDataType::createEditorProperty(				\
		const BW::string& name, ChunkItem* item, int editorPropertyId )				\
	{																				\
		BW_GUARD;																	\
		MF_ASSERT( 0 && "Creating a UserDataObjectLinkDataType from an item that "	\
			"it's neither a EditorChunkEntity nor a EditorChunkUserDataObject." );	\
		return NULL;																\
	}
#endif // EDITOR_ENABLED

BW_BEGIN_NAMESPACE

class MetaDataType;
class DataSink;
class DataSource;


class UserDataObjectLinkDataType : public DataType
{
public:
	UserDataObjectLinkDataType( MetaDataType * pMeta );

#ifdef EDITOR_ENABLED
	static BW::string asString( PyObject * pValue );

	// This method is implemented in a different C++ file in the editor, in the
	// bigbang folder, in editor_user_data_object_link_data_type.cpp.
	GeneralProperty * createEditorProperty( const BW::string& name,
		ChunkItem* item, int editorPropertyId );
#endif //EDITOR_ENABLED

protected:
	virtual bool isSameType( ScriptObject pValue );

	virtual void setDefaultValue( DataSectionPtr pSection );

	virtual bool getDefaultValue( DataSink & output ) const;

	virtual int streamSize() const;

	virtual bool addToSection( DataSource & source,
			DataSectionPtr pSection ) const;

	virtual bool createFromSection( DataSectionPtr pSection,
			DataSink & sink ) const;

	virtual void addToMD5( MD5 & md5 ) const;

	virtual StreamElementPtr getStreamElement( size_t index,
		size_t & size, bool & isNone, bool isPersistentOnly ) const;

	virtual bool operator<( const DataType & other ) const;

private:
	BW::string defaultId_;
	BW::string defaultChunkId_;
};

BW_END_NAMESPACE

#endif // USER_DATA_OBJECT_LINK_DATA_TYPE_HPP
