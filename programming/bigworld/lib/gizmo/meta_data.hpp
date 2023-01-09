#ifndef META_DATA_HPP
#define META_DATA_HPP

#include "cstdmf/bw_vector.hpp"
#include "general_properties.hpp"
#include "general_editor.hpp"
#include "meta_data_define.hpp"
#include "cstdmf/singleton.hpp"
#include "resmgr/datasection.hpp"

BW_BEGIN_NAMESPACE

namespace MetaData
{

class PropertyDesc;
class MetaData;

/**
 *	This class can hold any value that has a stream
 *	insertor and extractor
 */
class Any
{
	BW::string value_;
public:
	template <typename T>
	Any( const T& value )
	{
		BW::stringstream ss;

		ss << value;

		value_ = ss.str();
	}

	Any( const Any& that )
		: value_( that.value_ )
	{}

	const BW::string& value() const 
	{
		return value_;
	}

	template <typename T>
	operator T() const
	{
		BW::istringstream ss( value_ );
		T t;

		ss >> t;

		return t;
	}
};


template<typename T>
T any_cast( const Any& any )
{
	return (T)any;
}

template<>
inline BW::string any_cast( const Any& any )
{
	return any.value();
}

/**
 *	This class holds a meta data property
 */
class Property : public SafeReferenceCount
{
	const PropertyDesc& desc_;
	MetaData* metaData_;
public:
	Property( const PropertyDesc& desc, MetaData& metaData )
		: desc_( desc ), metaData_( &metaData )
	{}

	virtual GeneralProperty* createProperty( bool readOnly, bool useFullDateFormat ) = 0;
	virtual void set( const Any& value, bool isCreating = false ) = 0;
	virtual Any get() const = 0;
	virtual bool load( DataSectionPtr ds ) = 0;
	virtual bool save( DataSectionPtr ds ) const = 0;
	virtual Property* clone() const = 0;

	const PropertyDesc& desc() const
	{
		return desc_;
	}

	MetaData& metaData() const
	{
		return *metaData_;
	}

	void metaData( MetaData& metaData )
	{
		metaData_ = &metaData;
	}
};

typedef SmartPointer<Property> PropertyPtr;


/**
 *	This class defines the type of metadata
 */
class PropertyType
{
	BW::string name_;// type name, STRING, INT, etc
public:
	PropertyType( const BW::string& name );
	virtual ~PropertyType() {}
	virtual PropertyPtr create( const PropertyDesc& desc,
		MetaData& metaData, DataSectionPtr ds ) const = 0;

	typedef BW::map<BW::string, PropertyType*> TypeMap;

	static TypeMap& typeMap();
	void registerType();
	static const PropertyType* get( const BW::string& name );
};


/**
 *	This class defines a field of metadata, it has its
 *	type and the name of the property
 */
class PropertyDesc : public SafeReferenceCount
{
	BW::string name_;// description, created_on, etc
	BW::string description_;// description, created on, etc
	const PropertyType* type_;
	const DataSectionPtr descSection_;
public:
	PropertyDesc( const BW::string& name,
		const BW::string& description, const PropertyType* type,
		const DataSectionPtr descSection )
		: name_( name ), description_( description ),
		type_( type ), descSection_( descSection )
	{}
	const BW::string& name() const
	{
		return name_;
	}
	const PropertyType* type() const
	{
		return type_;
	}
	const BW::string& description() const
	{
		return description_;
	}
	const DataSectionPtr descSection() const
	{
		return descSection_;
	}
	PropertyPtr create( MetaData& metaData, DataSectionPtr ds ) const
	{
		BW_GUARD;

		return type()->create( *this, metaData, ds );
	}
};

typedef SmartPointer<PropertyDesc> PropertyDescPtr;


/**
 *	This class describes the whole metadata. It provides load/save
 *	functionality of individual property descriptions
 */
class Desc
{
	typedef BW::map<BW::string, PropertyDescPtr> PropDescs;
	PropDescs propDescs_;
public:
	Desc( const BW::string& configFile );
	bool load( DataSectionPtr ds );
	bool load( DataSectionPtr ds, MetaData& metaData ) const;
	void addPropDesc( PropertyDescPtr propDesc );
private:
	bool internalLoad( DataSectionPtr ds );
	void addDefaultDesc();
};


/**
 *	This class holds all metadata properties of a certain object
 */
class MetaData
{
	typedef BW::vector<PropertyPtr> Properties;
	Properties properties_;
	void* owner_;
public:
	MetaData( void* owner ) : owner_( owner )
	{}
	MetaData( const MetaData& that );
	MetaData& operator =( const MetaData& that );
	bool load( DataSectionPtr ds, const Desc& desc );
	bool save( DataSectionPtr ds ) const;
	void edit( GeneralEditor& editor, const Name& group, bool readOnly );

	void* owner() const
	{
		return owner_;
	}

	void clearProperties();
	void addProperty( PropertyPtr property );
	PropertyPtr operator[]( const BW::string& name );
};


/**
 *	This simple class can be used to restore a metadata
 *	to its saved status
 */
class Restore
{
	MetaData saved_;
	MetaData& metaData_;
public:
	Restore( MetaData& metaData );
	void restore();
	MetaData& metaData()
	{
		return metaData_;
	}
};


/**
 *	Undo redo operation for general meta data modifications
 */
class Operation : public UndoRedo::Operation
{
	Restore restore_;
public:
	Operation( MetaData& metaData )
		: UndoRedo::Operation( reinterpret_cast<size_t>( typeid( UndoRedo ).name() ) ),
		restore_( metaData )
	{}

	virtual void undo()
	{
		BW_GUARD;

		UndoRedo::instance().add(
			new Operation( restore_.metaData() ) );
		restore_.restore();
	}

	virtual bool iseq( const UndoRedo::Operation & oth ) const
	{
		return false;
	}
};


/**
 *	Application dependent meta data environment. Metadata will only call
 *	its methods to get information of the outside environment. An application
 *	have to provide a derived class of it to handle certain metadata
 *	notifications
 */
class Environment : public Singleton<Environment>
{
public:
	virtual ~Environment() {}

	virtual void changed( void* param ) = 0;
	virtual BW::string username() const;
	virtual time_t time() const;
};

}

BW_END_NAMESPACE
#endif//META_DATA_HPP
