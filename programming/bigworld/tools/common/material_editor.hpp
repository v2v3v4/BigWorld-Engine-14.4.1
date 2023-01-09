#ifndef MATERIAL_EDITOR_HPP
#define MATERIAL_EDITOR_HPP

#include "gizmo/general_editor.hpp"
#include "gizmo/general_properties.hpp"
#include "dxenum.hpp"
#include "material_proxies.hpp"
#include "moo/effect_material.hpp"
#include "cstdmf/bw_map.hpp"

BW_BEGIN_NAMESPACE 

namespace Moo
{
	class EffectMaterial;
};


/**
 *	This class defines the interface for all
 *	Material Property Editors
 */
/*class MaterialProperty : public GeneralProperty
{
public:
	MaterialProperty(  const BW::string & name ):
	  GeneralProperty( name ),
	  property_( NULL )
	{};

	void property( Moo::EffectPropertyPtr& p )	{ property_ = p; }
	//Moo::EffectPropertyPtr property()			{ return property_; }

	//This method validates the given value, and sets it if ok.
	//Returns false if validation failed.
	//
	//parameter is a void pointer to handle any type of data.
	bool validateAndSetValue( void* value );
private:
	Moo::EffectPropertyPtr property_;
};*/


/**************************************************************
 *	Section - Material Properties
 **************************************************************/
class MaterialKindProxy : public IntProxy
{
public:
	MaterialKindProxy( Moo::EffectMaterialPtr m ):
      material_(m)
	{
	}

	int32 get() const
	{
		BW_GUARD;

		return material_->materialKind();
	}

	void set( int32 f, bool transient, bool addBarrier = true )
	{
		BW_GUARD;

		material_->materialKind(f);
	}

protected:
	Moo::EffectMaterialPtr material_;
};

class CollisionFlagsProxy : public IntProxy
{
public:
	CollisionFlagsProxy( Moo::EffectMaterialPtr m ):
      material_(m)
	{
	}

	int32 get() const
	{
		BW_GUARD;

		return material_->collisionFlags();
	}

	void set( int32 f, bool transient, bool addBarrier = true )
	{
		BW_GUARD;

        if ( material_->collisionFlags() != f )
        {
    		material_->collisionFlags(f);
            material_->bspModified_ = true;
        }
	}

protected:
	Moo::EffectMaterialPtr material_;
};

// this proxy is used to convert an int proxy with enum support to an index
// proxy, the set/getter of index of a vector<string>
// This is because ChoiceProperty only supports index
class MaterialEnumProxy : public IntProxy
{
public:
	MaterialEnumProxy( const BW::string& propertyEnumType, const SmartPointer<MaterialIntProxy>& proxy ):
		propertyEnumType_( propertyEnumType ),
		proxy_( proxy )
	{
		BW_GUARD;

		proxy_->get();
		size_t size = DXEnum::instance().size( propertyEnumType_ );
		MF_ASSERT( size <= INT_MAX );
		for( DXEnum::size_type i = 0; i < size;
				++i )
		{
				valueToIndexMap_[ DXEnum::instance().value( propertyEnumType_,
						DXEnum::instance().entry( propertyEnumType_, i ) ) ]
						= ( int ) i;
		}
	}

	~MaterialEnumProxy()
	{
	}

	int32 get() const
    {
		BW_GUARD;

		return proxy_->get();
    }

	void set( int32 f, bool transient, bool addBarrier = true )
	{
		BW_GUARD;

		proxy_->set( f, transient );
		return;
	}

private:
	BW::string propertyEnumType_;
	SmartPointer<MaterialIntProxy> proxy_;
	BW::map<uint32,uint32> valueToIndexMap_;
};

BW_END_NAMESPACE

#endif
