#include "pch.hpp"
#include "cstdmf/debug.hpp"
#include "moo/effect_material.hpp"
#include "material_utility.hpp"
#include "material_proxies.hpp"
#include "resmgr/string_provider.hpp"

DECLARE_DEBUG_COMPONENT2( "Common", 0 )

BW_BEGIN_NAMESPACE

/**
 *	This method returns the number of techniques in the given material.
 */
int MaterialUtility::numTechniques( Moo::EffectMaterialPtr material )
{
	BW_GUARD;

	int retVal = 0;

	if (material.exists() && material->pEffect().exists())
	{
		retVal = static_cast<int>(material->pEffect()->techniques().size());
	}

	return retVal;
}


/**
 *	This method fills a vector of strings with the list
 *	of techniques for the given effect material.
 *
 *	@param material		The effect material.
 *	@param retVector	The returned vector of strings.
 *
 *	@return				The number of techniques.
 */
int MaterialUtility::listTechniques(	Moo::EffectMaterialPtr material,
										BW::vector<BW::string> & retVector )
{
	BW_GUARD;

	retVector.clear();

	if (material.exists() && material->pEffect().exists())
	{
		const Moo::ManagedEffect::TechniqueInfoCache& techniques = material->pEffect()->techniques();
		for (size_t i = 0; i < techniques.size(); i++)
		{
			retVector.push_back( techniques[i].name_ );
		}
	}

	MF_ASSERT( retVector.size() <= INT_MAX );
	return ( int ) retVector.size();
}


/**
 *	This method selects the given technique in the given material
 *	for viewing.  It takes a technique index, which is the same
 *	as the index given in the listTechniques method.
 *
 *	@param material		The effect material.
 *	@param index		The index of the desired technique.
 *
 *	@return				Success or Failure.
 */
bool MaterialUtility::viewTechnique(	Moo::EffectMaterialPtr material,
										int index )
{
	BW_GUARD;

	if (material.exists() && material->pEffect().exists() &&
		MaterialUtility::isTechniqueValid( material, index ))
	{
		const Moo::ManagedEffect::TechniqueInfoCache& techniques = material->pEffect()->techniques();
		material->hTechnique( techniques[index].handle_ );
		return true;
	}

	return false;
}


/**
 *	This method selects the given technique in the given material
 *	for viewing.  It takes a technique name, instead of an index.
 *
 *	@param material		The effect material.
 *	@param name			The name of the desired technique.
 *
 *	@return				Success or Failure.
 */
bool MaterialUtility::viewTechnique(	Moo::EffectMaterialPtr material,
										const BW::string & name )
{
	BW_GUARD;

	int index = techniqueByName( material, name );
	if ( index >= 0 )
	{
		return viewTechnique( material, index );
	}
	else
	{
		ERROR_MSG( "MaterialUtility::viewTechnique: technique '%s' not found for material %p.\n", name.c_str(), &*material );
		return false;
	}
}


/**
 *	This method returns the index of a technique, given the name of a
 *	technique.
 *
 *	@param	material		The effect material
 *	@param	name			The name of the desired technique
 *
 *	@return the index of the technique, or -1 to indicate the name was not
 *	found.
 */
int MaterialUtility::techniqueByName(	Moo::EffectMaterialPtr material,
										const BW::string & name )
{
	BW_GUARD;

	if (material.exists() && material->pEffect().exists())
	{
		const Moo::ManagedEffect::TechniqueInfoCache& techniques = material->pEffect()->techniques();
		for (size_t i = 0; i < techniques.size(); i++)
		{
			if (techniques[i].name_ == name)
			{
				return int(i);
			}
		}
	}

	return -1;
}


/**
 *	This method checks whether the given technique is valid.
 *
 *	@param material		The effect material.
 *	@param index		The index of the desired technique.
 *
 *	@return				If the technique is valid.
 */
bool MaterialUtility::isTechniqueValid(
	Moo::EffectMaterialPtr material,
	int index )
{
	BW_GUARD;

	if (material.exists() && material->pEffect().exists())
	{
		const Moo::ManagedEffect::TechniqueInfoCache& techniques = material->pEffect()->techniques();
		if (index >= 0 && index < int(techniques.size()))
		{
			return techniques[index].supported_;
		}
	}

	return false;
}


/**
 *	This method returns the index of the technique currently selected
 *	into a material.
 *
 *	If anything is wrong, it returns -1
 */
int MaterialUtility::currentTechnique( Moo::EffectMaterialPtr material )
{
	BW_GUARD;

	if (material.exists() && material->pEffect().exists())
	{
		const Moo::ManagedEffect::TechniqueInfoCache& techniques = material->pEffect()->techniques();
		for (size_t i = 0; i < techniques.size(); i++)
		{
			if (techniques[i].handle_ == material->hTechnique())
			{
				return int(i);
			}
		}
	}


	return -1;
}


/**
 *	This method returns the number of tweakable properties
 *	the material has.
 */
int MaterialUtility::numProperties( Moo::EffectMaterialPtr material )
{
	BW_GUARD;

	return static_cast<int>(material->properties().size());
}


/**
 *	This method fills a vector of strings with the list
 *	of editable properties for the given effect material.
 *
 *	@param material		The effect material.
 *	@param retVector	The returned vector of strings.
 *
 *	@return				The number of properties.
 */
int MaterialUtility::listProperties(
	Moo::EffectMaterialPtr material,
	BW::vector<BW::string> & retVector )
{
	BW_GUARD;

	if (material.exists())
	{
		Moo::EffectMaterial::Properties& properties = material->properties();
		Moo::EffectMaterial::Properties::iterator it = properties.begin();
		Moo::EffectMaterial::Properties::iterator end = properties.end();
		while ( it != end )
		{
			EditorEffectProperty* pProperty = dynamic_cast<EditorEffectProperty*>( it->second.get() );
			if (pProperty)
			{
				retVector.push_back( pProperty->name() );
			}
			it++;
		}
	}

	return static_cast<int>(retVector.size());
}


/**
 *	This method saves the given material's tweakable properties
 *	to the given datasection.
 *
 *	Material saving does not support recursion / inherited properties.
 *
 *	@param	m			The material to save
 *	@param	pSection	The data section to save to.
 *
 *	@return Success or Failure.
 */
void MaterialUtility::save(
	Moo::EffectMaterialPtr material,
	DataSectionPtr pSection )
{
	BW_GUARD;

    pSection->deleteSections( "property" );
    pSection->deleteSections( "fx" );

   
	if ( !material->pEffect() )
    {
		return;
    }

       pSection->writeString( "fx", material->pEffect()->resourceID() );

	Moo::EffectMaterial::Properties& properties = material->properties();
    Moo::EffectMaterial::Properties::iterator it = properties.begin();
    Moo::EffectMaterial::Properties::iterator end = properties.end();
    
    while ( it != end )
    {
        EditorEffectProperty* pProperty = dynamic_cast<EditorEffectProperty*>(it->second.get());
		MF_ASSERT( pProperty != NULL );

        if (artistEditable( pProperty ))
        {
            BW::string name(pProperty->name());
            DataSectionPtr pChild = pSection->newSection( "property" );
            pChild->setString( pProperty->name() );
            pProperty->save( pChild );
        }
		it++;
	}

	pSection->writeInt( "collisionFlags", material->collisionFlags() );
	pSection->writeInt( "materialKind", material->materialKind() );
}


bool MaterialUtility::artistEditable( EditorEffectProperty* pProperty )
{
	BW_GUARD;

	bool artistEditable = false;

	pProperty->boolAnnotation( "artistEditable", artistEditable );
	return artistEditable;
}


BW::string MaterialUtility::UIName( EditorEffectProperty* pProperty )
{
	BW_GUARD;

	BW::string uiName;
	pProperty->stringAnnotation( "UIName", uiName );
	return uiName;
}

BW::string MaterialUtility::UIDesc( EditorEffectProperty* pProperty )
{
	BW_GUARD;

	BW::string uiDesc;
	pProperty->stringAnnotation( "UIDesc", uiDesc );
	return uiDesc;
}

BW::string MaterialUtility::UIWidget( EditorEffectProperty* pProperty )
{
	BW_GUARD;

	BW::string uiWidget;
	pProperty->stringAnnotation( "UIWidget", uiWidget );
	return uiWidget;
}

void MaterialUtility::setTexture( Moo::EffectMaterialPtr material,
	int idx, const BW::string& textureName )
{
	BW_GUARD;

    Moo::EffectMaterial::Properties& properties = material->properties();
    Moo::EffectMaterial::Properties::iterator it = properties.begin();
    Moo::EffectMaterial::Properties::iterator end = properties.end();

    while ( it != end )
    {
        MF_ASSERT( it->second );
        MaterialTextureProxy* pProperty = dynamic_cast<MaterialTextureProxy*>(it->second.get());

        if ( pProperty && artistEditable( pProperty ) )
        {
			pProperty->set( textureName, false );
        }
        it++;
    }
}
BW_END_NAMESPACE

