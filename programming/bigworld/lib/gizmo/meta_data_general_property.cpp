#include "pch.hpp"
#include "meta_data.hpp"
#include "resmgr/string_provider.hpp"

BW_BEGIN_NAMESPACE

namespace
{

template<typename ProxyType>
class Proxy : public ProxyType
{
	const BW::string& name_;
	typename mutable ProxyType::Data old_;
	typename mutable ProxyType::Data transient_;
	MetaData::MetaData& metaData_;
public:
	Proxy( const BW::string& name, MetaData::MetaData& metaData )
		: name_( name ), metaData_( metaData )
	{
		BW_GUARD;

		old_ = MetaData::any_cast<Data>( metaData_[ name_ ]->get() );
		transient_ = old_;
	}
	virtual typename ProxyType::Data get() const
	{
		BW_GUARD;

		if (old_ != MetaData::any_cast<Data>( metaData_[ name_ ]->get() ))
		{
			old_ = MetaData::any_cast<Data>( metaData_[ name_ ]->get() );
			transient_ = old_;
		}

		return transient_;
	}
	virtual void set( typename ProxyType::Data value, bool transient,
							bool addBarrier )
	{
		BW_GUARD;

		transient_ = value;

		if (!transient)
		{
			UndoRedo::instance().add( new MetaData::Operation( metaData_ ) );

			metaData_[ name_ ]->set( value );
			MetaData::Environment::instance().changed( metaData_.owner() );

			if (addBarrier)
			{
				UndoRedo::instance().barrier(
					LocaliseUTF8( L"GIZMO/METADATA/MODIFY_META_DATA" ), false );
			}
		}
	}
};


template<typename PropertyType, typename ProxyType>
class Property : public MetaData::Property
{
	typename ProxyType::Data value_;
	bool readOnly_;
public:
	Property( const MetaData::PropertyDesc& desc,
		MetaData::MetaData& metaData, bool readOnly )
		: MetaData::Property( desc, metaData ), readOnly_( readOnly )
	{}
	virtual GeneralProperty* createProperty( bool readOnly, bool )
	{
		BW_GUARD;

		if (readOnly_ || readOnly)
		{
			return new StaticTextProperty( Name( desc().description() ),
				new Proxy<StringProxy>( desc().name(), metaData() ) );
		}
		return new PropertyType( Name( desc().description() ),
			new Proxy<ProxyType>( desc().name(), metaData() ) );
	}
	virtual void set( const MetaData::Any& value, bool isCreating = false )
	{
		BW_GUARD;

		value_ = MetaData::any_cast<ProxyType::Data>( value );
		if (!isCreating)
		{
			MetaData::Environment::instance().changed( metaData().owner() );
		}
	}
	virtual MetaData::Any get() const
	{
		return value_;
	}
	virtual bool load( DataSectionPtr ds )
	{
		BW_GUARD;

		value_ = ds ? ds->read<ProxyType::Data>( desc().name() ) : ProxyType::Data();

		return true;
	}
	virtual bool save( DataSectionPtr ds ) const
	{
		BW_GUARD;

		ds->deleteSections( desc().name() );

		if (value_ != Datatype::DefaultValue<typename ProxyType::Data>::val())
		{
			ds->write( desc().name(), value_ );
		}

		return true;
	}
	virtual Property* clone() const
	{
		BW_GUARD;

		return new Property( *this );
	}
};


template<typename PropType, typename ProxyType>
class PropertyType : public MetaData::PropertyType
{
public:
	PropertyType( const BW::string& type )
		: MetaData::PropertyType( type )
	{}
	virtual MetaData::PropertyPtr create( const MetaData::PropertyDesc& desc,
		MetaData::MetaData& metaData, DataSectionPtr ds ) const
	{
		BW_GUARD;

		bool readOnly = desc.descSection()->readBool( "readonly", false );
		MetaData::PropertyPtr property
			= new Property<PropType, ProxyType>(
				desc, metaData, readOnly );

		if (property->load( ds ))
		{
			return property;
		}

		return NULL;
	}
};


static PropertyType<TextProperty, StringProxy>
	s_metaDataStringPropertyType( "STRING" );
static PropertyType<GenIntProperty, IntProxy>
	s_metaDataIntPropertyType( "INT" );
static PropertyType<GenFloatProperty, FloatProxy>
	s_metaDataFloatPropertyType( "FLOAT" );
}//namespace MetaData

int metaDataGeneralPropertyTypeToken;
BW_END_NAMESPACE

