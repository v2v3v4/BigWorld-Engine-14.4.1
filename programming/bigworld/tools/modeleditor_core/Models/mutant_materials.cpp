#include "pch.hpp"

#include "model/super_model.hpp"
#include "model/super_model_dye.hpp"
#include "model/tint.hpp"

#include "resmgr/xml_section.hpp"
#include "resmgr/string_provider.hpp"
#include "resmgr/primitive_file.hpp"

#include "undo_redo.hpp"

#include "moo/geometrics.hpp"

#include "common/material_utility.hpp"
#include "common/material_properties.hpp"
#include "common/material_editor.hpp"
#include "common/dxenum.hpp"

#include "tools/common/utilities.hpp"

#include "mutant.hpp"

DECLARE_DEBUG_COMPONENT2( "Mutant_Materials", 0 )

BW_BEGIN_NAMESPACE

MaterialInfo::MaterialInfo():
	nameData (NULL),
	colours( false ),
	dualUV( false )
{}
MaterialInfo::MaterialInfo(
	const BW::string& cName,
	DataSectionPtr cNameData,
	ComplexEffectMaterialSet cEffect,
	BW::vector< DataSectionPtr > cData,
	BW::string cFormat,
		bool cColours,
		bool cDualUV

):
	name (cName),
	nameData (cNameData),
	effect (cEffect),
	data (cData),
	format (cFormat),
	colours( cColours ),
	dualUV( cDualUV )
{}

TintInfo::TintInfo()
{}
TintInfo::TintInfo(
	Moo::ComplexEffectMaterialPtr cEffect,
	DataSectionPtr cData,
	SuperModelDyePtr cDye,
	BW::string cFormat,
	bool cColours,
	bool cDualUV

):
	effect(cEffect),
	data(cData),
	dye(cDye),
	format(cFormat),
	colours(cColours),
	dualUV(cDualUV)

{}

BW::string Mutant::materialDisplayName( const BW::string& materialName )
{
	BW_GUARD;

	BW::string test = materials_[materialName].name;
	return materials_[materialName].name;
}

void Mutant::setDye( const BW::string& matterName, const BW::string& tintName, Moo::ComplexEffectMaterialPtr & material  )
{
	BW_GUARD;

	currDyes_[matterName] = tintName;

	material = tints_[matterName][tintName].effect;

	recreateFashions();
}

void Mutant::getMaterial( const BW::string& materialName, ComplexEffectMaterialSet & material )
{
	BW_GUARD;

	material = materials_[materialName].effect;
}

BW::string Mutant::getTintName( const BW::string& matterName )
{
	BW_GUARD;

	if (currDyes_.find(matterName) != currDyes_.end())
		return currDyes_[matterName];
	else
		return "Default";
}

bool Mutant::setMaterialProperty( const BW::string& materialName, const BW::string& descName, const BW::string& uiName,  const BW::string& propType, const BW::string& val )
{
	BW_GUARD;

	BW::vector< DataSectionPtr > pMats = materials_[materialName].data;

	BW::vector< DataSectionPtr >::iterator matIt = pMats.begin();
	BW::vector< DataSectionPtr >::iterator matEnd = pMats.end();
	while (matIt != matEnd)
	{
		DataSectionPtr data = *matIt++;

		UndoRedo::instance().add( new UndoRedoOp( 0, data, currVisual_ ));

		BW::vector< DataSectionPtr > pProps;
		data->openSections( "property", pProps );

		bool done = false;
		
		BW::vector< DataSectionPtr >::iterator propsIt = pProps.begin();
		BW::vector< DataSectionPtr >::iterator propsEnd = pProps.end();
		while (propsIt != propsEnd)
		{
			DataSectionPtr prop = *propsIt++;

			if (descName == prop->asString())
			{
				DataSectionPtr textureFeed = prop->openSection("TextureFeed");
			
				if (textureFeed)
				{
					textureFeed->writeString( "default", BWResource::dissolveFilename( val ));
				}
				else
				{
					prop->writeString( propType, BWResource::dissolveFilename( val ));
				}

				done = true;
			}
		}

		if (!done)
		{
			data = data->newSection( "property" );
			data->setString( descName );
			data->writeString( propType, BWResource::dissolveFilename( val ));
		}
	}

	UndoRedo::instance().barrier( LocaliseUTF8(L"MODELEDITOR/MODELS/MUTANT_MATERIALS/CHANGE_TO", uiName ), true );

	return true;

}

void Mutant::instantiateMFM( DataSectionPtr data )
{
	BW_GUARD;

	BW::string mfmFile = data->readString( "mfm", "" );

	if (mfmFile != "")
	{
		DataSectionPtr mfmData = BWResource::openSection( mfmFile, false );

		if (!mfmData) return;

		BW::string temp = data->readString("identifier", "");
		if (temp != "")
			mfmData->writeString( "identifier", temp );
		temp = data->readString("fx", "");
		if (temp != "")
			mfmData->writeString( "fx", temp );

		mfmData->writeInt( "collisionFlags",
			data->readInt("collisionFlags", 0) );

		mfmData->writeInt( "materialKind",
			data->readInt("materialKind", 0) );
		
		BW::vector< DataSectionPtr > pSrcProps;
		data->openSections( "property", pSrcProps );

		BW::vector< DataSectionPtr > pDestProps;
		mfmData->openSections( "property", pDestProps );

		BW::vector< DataSectionPtr >::iterator destPropsIt = pDestProps.begin();
		BW::vector< DataSectionPtr >::iterator destPropsEnd = pDestProps.end();
		while (destPropsIt != destPropsEnd)
		{
			DataSectionPtr destProp = *destPropsIt++;

			BW::vector< DataSectionPtr >::iterator srcPropsIt = pSrcProps.begin();
			BW::vector< DataSectionPtr >::iterator srcPropsEnd = pSrcProps.end();
			while (srcPropsIt != srcPropsEnd)
			{
				DataSectionPtr srcProp = *srcPropsIt++;

				if (destProp->asString() == srcProp->asString())
				{
					destProp->copy( srcProp );
				}
			}
		}

		data->copy( mfmData );
		data->delChild("mfm");
	}
}

void Mutant::overloadMFM( DataSectionPtr data, DataSectionPtr mfmData )
{
	BW_GUARD;

	BW::string temp = mfmData->readString("fx", "");
	if (temp != "")
		data->writeString( "fx", temp );
	
	data->writeInt( "collisionFlags",
		mfmData->readInt("collisionFlags", 0) );

	data->writeInt( "materialKind",
		mfmData->readInt("materialKind", 0) );
		
	BW::vector< DataSectionPtr > pSrcProps;
	mfmData->openSections( "property", pSrcProps );

	BW::vector< DataSectionPtr > pDestProps;
	data->openSections( "property", pDestProps );

	BW::vector< DataSectionPtr >::iterator srcPropsIt = pSrcProps.begin();
	BW::vector< DataSectionPtr >::iterator srcPropsEnd = pSrcProps.end();
	while (srcPropsIt != srcPropsEnd)
	{
		DataSectionPtr srcProp = *srcPropsIt++;

		bool placed = false;
		
		BW::vector< DataSectionPtr >::iterator destPropsIt = pDestProps.begin();
		BW::vector< DataSectionPtr >::iterator destPropsEnd = pDestProps.end();
		while (destPropsIt != destPropsEnd)
		{
			DataSectionPtr destProp = *destPropsIt++;

			if (destProp->asString() == srcProp->asString())
			{
				destProp->copy( srcProp );
				placed = true;
			}
		}

		if (!placed)
		{
			data->newSection("property")->copy( srcProp );
		}
	}
}

void Mutant::cleanMaterials()
{
	BW_GUARD;

	BW::map< BW::string, MaterialInfo >::iterator it = materials_.begin();
	BW::map< BW::string, MaterialInfo >::iterator end = materials_.end();

	for (; it != end; ++it)
	{
		BW::string materialName = it->first;
		BW::vector< DataSectionPtr > data = it->second.data;
		Moo::ComplexEffectMaterialPtr effect = *(it->second.effect.begin());
		if (tintedMaterials_.find( materialName ) != tintedMaterials_.end())
		{
			effect = tints_[ tintedMaterials_[materialName] ]["Default"].effect;
		}

		if (data.size() == 0) continue;

		instantiateMFM( data[0] );
		
		// Make a backup of the old material data
		XMLSectionPtr materialData = new XMLSection("old_state");
		materialData->copy( data[0] );
		
		// Erase all the material data
		for (unsigned i=0; i<data.size(); i++)
		{
			data[i]->delChildren();
			
			// Copy the default fields first
			BW::string temp = materialData->readString("identifier", "");
			if (temp != "")
				data[i]->writeString( "identifier", temp );

			// Write all effect references
			BW::vector< BW::string > fxs;
			materialData->readStrings( "fx", fxs );
			for ( unsigned j=0; j<fxs.size(); j++)
			{
				data[i]->newSection( "fx" )->setString( fxs[j] );
			}

			data[i]->writeInt( "collisionFlags",
				materialData->readInt("collisionFlags", 0) );

			data[i]->writeInt( "materialKind",
				materialData->readInt("materialKind", 0) );
		}

		//Now add the material's own properties.
		effect->baseMaterial()->replaceDefaults();

		BW::vector< BW::string > existingProps;

		if ( effect->baseMaterial()->pEffect() )
		{
			Moo::EffectMaterial::Properties& properties = effect->baseMaterial()->properties();
			Moo::EffectMaterial::Properties::iterator propIt = properties.begin();
			Moo::EffectMaterial::Properties::iterator propEnd = properties.end();

			for (; propIt != propEnd; ++propIt )
			{
				EditorEffectProperty* pProperty = dynamic_cast<EditorEffectProperty*>(propIt->second.get());
				
				if ( MaterialUtility::artistEditable( pProperty ) )
				{
					BW::string descName = pProperty->name();

					// Find all the material properties
					BW::vector< DataSectionPtr > srcProps;
					materialData->openSections( "property", srcProps );

					// Copy the material properties if they exist
					BW::vector< DataSectionPtr >::iterator srcPropsIt = srcProps.begin();
					BW::vector< DataSectionPtr >::iterator srcPropsEnd = srcProps.end();
					for (; srcPropsIt != srcPropsEnd; ++srcPropsIt)
					{
						if (descName == (*srcPropsIt)->asString())
						{
							// Skip over properties that we have already added.  This can occur
							// when using multi-layer effects - there will most likely be
							// shared properties referenced by both effects.
							BW::vector< BW::string >::iterator fit =
								std::find(existingProps.begin(),existingProps.end(),descName);
							if ( fit == existingProps.end() )
							{
								// For all the material sections add the property
								for (unsigned i=0; i<data.size(); i++)
								{
									data[i]->newSection("property")->copy( (*srcPropsIt) );
								}
								existingProps.push_back( descName );
								continue;
							}
						}
					}
				}
			}
		}
	}
	dirty( currVisual_ );
}

void Mutant::cleanTints()
{
	BW_GUARD;

	BW::map< BW::string, BW::map < BW::string, TintInfo > >::iterator dyeIt = tints_.begin();
	BW::map< BW::string, BW::map < BW::string, TintInfo > >::iterator dyeEnd = tints_.end();

	for(; dyeIt != dyeEnd; ++dyeIt)
	{
		BW::string matterName = (*dyeIt).first;
		BW::map < BW::string, TintInfo >::iterator tintIt = (*dyeIt).second.begin();
		BW::map < BW::string, TintInfo >::iterator tintEnd = (*dyeIt).second.end();

		for(; tintIt != tintEnd; ++tintIt)
		{
			BW::string tintName = tintIt->first;
			DataSectionPtr data = tintIt->second.data;
			Moo::ComplexEffectMaterialPtr effect = tintIt->second.effect;

			if (data)
				data = data->openSection("material");

			if (!data) 
				continue;

			instantiateMFM( data );

			// Make a backup of the old material data
			XMLSectionPtr materialData = new XMLSection("old_state");
			materialData->copy( data );
			
			// Erase all the material data
			data->delChildren();
				
			// Copy the default fields first
			BW::string temp = materialData->readString("identifier", "");
			if (temp != "")
				data->writeString( "identifier", temp );
			
			// Write all effect references
			BW::vector< BW::string > fxs;
			materialData->readStrings( "fx", fxs );
			for ( unsigned j=0; j<fxs.size(); j++)
			{
				data->newSection( "fx" )->setString( fxs[j] );
			}

			data->writeInt( "collisionFlags",
				materialData->readInt("collisionFlags", 0) );

			data->writeInt( "materialKind",
				materialData->readInt("materialKind", 0) );

			//Now add the material's own properties.
			effect->baseMaterial()->replaceDefaults();

			if ( effect->baseMaterial()->pEffect() )
			{
				Moo::EffectMaterial::Properties& properties = effect->baseMaterial()->properties();
				Moo::EffectMaterial::Properties::iterator propIt = properties.begin();
				Moo::EffectMaterial::Properties::iterator propEnd = properties.end();

				for (; propIt != propEnd; ++propIt )
				{
					EditorEffectProperty* pProperty = dynamic_cast<EditorEffectProperty*>(propIt->second.get());
					MF_ASSERT( pProperty != NULL );

					if ( MaterialUtility::artistEditable( pProperty ) )
					{
						BW::string descName = pProperty->name();;

						// Find all the material properties
						BW::vector< DataSectionPtr > srcProps;
						materialData->openSections( "property", srcProps );

						// Copy the material properties if they exist
						BW::vector< DataSectionPtr >::iterator srcPropsIt = srcProps.begin();
						BW::vector< DataSectionPtr >::iterator srcPropsEnd = srcProps.end();
						for (; srcPropsIt != srcPropsEnd; ++srcPropsIt)
						{
							if (descName == (*srcPropsIt)->asString())
							{
								data->newSection("property")->copy( (*srcPropsIt) );
							}
						}
					}
				}
			}
		}
	}
	dirty( currModel_ );
}

void Mutant::cleanMaterialNames()
{
	BW_GUARD;

	if (materialNameDataToRemove_.empty()) return;
	
	DataSectionPtr data = currModel_->openSection( "materialNames" );

	if (data == NULL) return;
	
	BW::vector< DataSectionPtr>::iterator it = materialNameDataToRemove_.begin();
	BW::vector< DataSectionPtr>::iterator end = materialNameDataToRemove_.end();
	for (; it != end; ++it)
	{
		data->delChild( *it );
	}
	
	materialNameDataToRemove_.clear();
	
	dirty( currModel_ );
}

bool Mutant::setTintProperty( const BW::string& matterName, const BW::string& tintName, const BW::string& descName,  const BW::string& uiName,  const BW::string& propType, const BW::string& val )
{
	BW_GUARD;

	DataSectionPtr data = tints_[matterName][tintName].data;

	if (data == NULL) return false;

	DataSectionPtr materialData = data->openSection("material");

	if (materialData == NULL) return false;

	UndoRedo::instance().add( new UndoRedoOp( 0, data, currModel_ ));
	UndoRedo::instance().barrier( "Change to " + uiName, true );

	instantiateMFM( materialData );

	BW::vector<DataSectionPtr> pProps;
	materialData->openSections( "property", pProps );

	bool done = false;

	BW::vector< DataSectionPtr >::iterator propsIt = pProps.begin();
	BW::vector< DataSectionPtr >::iterator propsEnd = pProps.end();
	while (propsIt != propsEnd)
	{
		DataSectionPtr prop = *propsIt++;

		if (descName == prop->asString())
		{
			DataSectionPtr textureFeed = prop->openSection("TextureFeed");
			
			if (textureFeed)
			{
				textureFeed->writeString( "default", BWResource::dissolveFilename( val ));
			}
			else
			{
				prop->writeString( propType, BWResource::dissolveFilename( val ));
			}

			done = true;
		}
	}

	if (!done)
	{
		materialData = materialData->newSection( "property" );
		materialData->setString( descName );
		materialData->writeString( propType, BWResource::dissolveFilename( val ));
	}

	//Now do nearly the exact same thing for the parent's "property" values for exposed properties.
	//We do this to keep the python exposed value the same as the materials.

	data->openSections( "property", pProps );
	
	propsIt = pProps.begin();
	propsEnd = pProps.end();
	while (propsIt != propsEnd)
	{
		DataSectionPtr prop = *propsIt++;

		if (descName == prop->readString( "name", "" ))
		{
			prop->writeVector4( "default", getExposedVector4( matterName, tintName, descName, propType, val ) );
		}
	}

	return true;

}

bool Mutant::materialName( const BW::string& materialName, const BW::string& new_name )
{
	BW_GUARD;

	//Exit if we have already set this name
	if (new_name == materials_[materialName].name) return true;

	BW::map< BW::string, MaterialInfo >::iterator it = materials_.begin();

	//Determine whether that material name is being used and exit if it is
	for (;it != materials_.end(); ++it)
	{
		if (new_name == it->second.name) return false;
	}

	DataSectionPtr pNameData = materials_[materialName].nameData;

	if (pNameData != NULL)
	{

		UndoRedo::instance().add( new UndoRedoOp( 0, pNameData, currModel_ ));
		UndoRedo::instance().barrier( LocaliseUTF8(L"MODELEDITOR/MODELS/MUTANT_MATERIALS/CHANGING_MATERIAL_NAME"), true );

	}
	else
	{
		pNameData = currModel_->openSection( "materialNames", true );

		UndoRedo::instance().add( new UndoRedoOp( 0, pNameData, currModel_ ));
		UndoRedo::instance().barrier( LocaliseUTF8(L"MODELEDITOR/MODELS/MUTANT_MATERIALS/CHANGING_MATERIAL_NAME"), true );
		
		pNameData = pNameData->newSection( "material" );
		materials_[materialName].nameData = pNameData;
	}

	pNameData->writeString( "original", materialName );
	pNameData->writeString( "display", new_name );
	materials_[materialName].name = new_name;

	//Update the tinted material map to use the new material name
	if ( tintedMaterials_.find( materialName ) != tintedMaterials_.end())
	{
		tintedMaterials_[new_name] = tintedMaterials_[materialName];
		tintedMaterials_.erase( materialName );
	}

	triggerUpdate( "Object" );

	return true;
}

bool Mutant::matterName( const BW::string& matterName, const BW::string& new_name, bool undoRedo )
{
	BW_GUARD;

	if (matterName == new_name) return true;
	
	if (undoRedo && dyes_.find( new_name ) != dyes_.end()) return false;

	//If we are curently using this matter make sure to update the reference
	if (currDyes_.find( matterName ) != currDyes_.end())
	{
		currDyes_[new_name] = currDyes_[matterName];
		currDyes_.erase( matterName );
	}

	if (undoRedo)
	{
		UndoRedo::instance().add( new UndoRedoMatterName( new_name, matterName ) );
		UndoRedo::instance().barrier( LocaliseUTF8(L"MODELEDITOR/MODELS/MUTANT_MATERIALS/CHANGING_DYE_NAME"), true );
	}
	
	if (dyes_.find( matterName ) != dyes_.end())
	{
		dyes_[matterName]->writeString("matter", new_name);

		dyes_[new_name] = dyes_[matterName];
		dyes_.erase(matterName);
	}

	if (tints_.find( matterName ) != tints_.end())
	{
		tints_[new_name] = tints_[matterName];
		tints_.erase(matterName);
	}

	//Update the tinted material map to use the new matter name
	BW::map< BW::string , BW::string >::iterator tintIt = tintedMaterials_.begin();
	BW::map< BW::string , BW::string >::iterator tintEnd = tintedMaterials_.end();
	for (; tintIt != tintEnd; ++tintIt)
	{
		if ( tintIt->second == matterName )
		{
			tintedMaterials_[tintIt->first] = new_name;
		}
	}

	this->reloadAllLists();

	return true;
}

bool Mutant::tintName( const BW::string& matterName, const BW::string& tintName, const BW::string& new_name, bool undoRedo )
{
	BW_GUARD;

	if (tintName == new_name) return true;
	
	if (undoRedo)
	{
		TintMap::iterator tintIt = tints_.begin();
		TintMap::iterator tintEnd = tints_.end();
		for (; tintIt != tintEnd; ++tintIt)
		{
			if (tintIt->second.find( new_name ) != tintIt->second.end()) return false;
		}
	}
	//If we are curently using this matter make sure to update the reference
	if (currDyes_.find( matterName ) != currDyes_.end())
	{
		currDyes_[matterName] = new_name;
	}

	DataSectionPtr data = tints_[matterName][tintName].data;

	if (undoRedo)
	{
		UndoRedo::instance().add( new UndoRedoTintName( matterName, new_name, tintName ) );
		UndoRedo::instance().barrier( LocaliseUTF8(L"MODELEDITOR/MODELS/MUTANT_MATERIALS/CHANGING_TINT_NAME"), true );
	}

	data->writeString( "name", new_name );
	
	tints_[matterName][new_name] = tints_[matterName][tintName];
	tints_[matterName].erase(tintName);

	this->reloadAllLists();

	return true;
}

BW::string Mutant::newTint( const BW::string& materialName, const BW::string& matterName, const BW::string& oldTintName, const BW::string& newTintName, const BW::string& fxFile, const BW::string& mfmFile )
{
	BW_GUARD;

	BW::string newMatterName = matterName;
		
	DataSectionPtr data;
	DataSectionPtr pMFMSec;

	if (mfmFile != "")
	{
		pMFMSec = BWResource::openSection( mfmFile, false );
		if (!pMFMSec)
		{
			ERROR_MSG( "Cannot open MFM file: %s", mfmFile.c_str() );
			newMatterName.clear();
			return newMatterName;
		}
	}
		
	if (matterName == "")
	{
		UndoRedo::instance().add( new UndoRedoOp( 0, currModel_, currModel_ ));
		UndoRedo::instance().barrier( LocaliseUTF8(L"MODELEDITOR/MODELS/MUTANT_MATERIALS/ADDING_TINT"), true );

		data = currModel_->newSection("dye");

		newMatterName = Utilities::pythonSafeName( materialName );
		data->writeString( "matter", newMatterName );
		data->writeString( "replaces", materialName );
	}
	else
	{
		data = dyes_[matterName];

		UndoRedo::instance().add( new UndoRedoOp( 0, currModel_, currModel_ ));
		UndoRedo::instance().barrier( LocaliseUTF8(L"MODELEDITOR/MODELS/MUTANT_MATERIALS/ADDING_TINT"), true );

	}
	
	data = data->newSection("tint");
	data->writeString( "name", newTintName );
	data = data->newSection("material");
	if (fxFile != "")
	{
		if ((matterName == "") || (oldTintName == "Default")) // A material
		{
			if ((materials_.find( materialName ) != materials_.end()) &&
				( materials_[materialName].data[0]))
			{
				data->copy( materials_[materialName].data[0] );
			}
		}
		else // A tint
		{
			if ((tints_.find( matterName ) != tints_.end()) &&
				(tints_[matterName].find( oldTintName ) != tints_[matterName].end()) &&
				(tints_[matterName][oldTintName].data))
			{
				DataSectionPtr materialData( tints_[matterName][oldTintName].data->openSection("material") );
				if (materialData)
				{
					data->copy( materialData );
				}
			}
		}
		
		
		data->writeString( "fx", fxFile );
	}
	else
	{
		data->copy( pMFMSec );
	}

	reloadAllLists();

	return newMatterName;

}

bool Mutant::saveMFM( const BW::string& materialName, const BW::string& matterName, const BW::string& tintName, const BW::string& mfmFile )
{
	BW_GUARD;

	DataSectionPtr data;
	
	if ((matterName == "") || (tintName == "Default")) // We are a material
	{
		if (materials_.find( materialName ) == materials_.end())
			return false;
	
		for (unsigned i=0; i<materials_[materialName].data.size(); i++ )
		{
			data = materials_[materialName].data[i];
			instantiateMFM( data );
		}
	}
	else // We are a tint
	{
		data = tints_[matterName][tintName].data;

		if (data == NULL) return false;

		data = data->openSection("material");

		if (data == NULL) return false;

		instantiateMFM( data );
	}

	DataSectionPtr mfmData;
	mfmData = BWResource::openSection( mfmFile, true );
	if (mfmData == NULL) return false;
	mfmData->copy( data );
	mfmData->delChild("identifier");
	return mfmData->save();

	//Even though this method could have dirtied the model (through the instantiation of the mfm)
	//we do not mark it as dirty since the user did not expressly request this change.
	//If any further changes are made then this will be saved.

}

void Mutant::deleteTint( const BW::string& matterName, const BW::string& tintName )
{
	BW_GUARD;

	if (dyes_.find( matterName ) == dyes_.end())
		return;
	
	DataSectionPtr data = dyes_[matterName];

	UndoRedo::instance().add( new UndoRedoOp( 0, currModel_, currModel_ ));
	UndoRedo::instance().barrier( LocaliseUTF8(L"MODELEDITOR/MODELS/MUTANT_MATERIALS/DELETING_TINT"), true );
	
	if (tints_.find( matterName ) == tints_.end())
		return;

	if (tints_[matterName].find( tintName ) == tints_[matterName].end())
		return;
	
	data->delChild( tints_[matterName][tintName].data );

	if (data->findChild( "tint" ) == NULL) // If there are no more tints...
	{
		currModel_->delChild( data ); // Remove the dye
	}

	currDyes_.erase( matterName );

	reloadAllLists();

}

bool Mutant::ensureShaderCorrect( const BW::string& fxFile, const BW::string& format, bool dualUV )
{
	BW_GUARD;

	if (fxFile == "") return true; // No shader can apply to any format

	bool softskinned = (format == "xyznuviiiww") || (format == "xyznuviiiwwtb");
	bool hardskinned = (format == "xyznuvi") || (format == "xyznuvitb");
	
	if ( fxFile.find("hardskinned") != BW::string::npos )
	{
		if (softskinned)
		{
			ERROR_MSG("Unable to apply a hardskinned shader to a softskinned object.\n");
			::MessageBox( AfxGetApp()->m_pMainWnd->GetSafeHwnd(),
				Localise(L"MODELEDITOR/MODELS/MUTANT_MATERIALS/SOFTSKINNED_ERROR_MSG"),
				Localise(L"MODELEDITOR/MODELS/MUTANT_MATERIALS/SHADER_ERROR"), MB_OK );
			return false;
		}
	}
	else if ( fxFile.find("skinned") != BW::string::npos )
	{
		if (hardskinned)
		{
			ERROR_MSG("Unable to apply a softskinned shader to a hardskinned object.\n");
			::MessageBox( AfxGetApp()->m_pMainWnd->GetSafeHwnd(),
				Localise(L"MODELEDITOR/MODELS/MUTANT_MATERIALS/HARDSKINNED_ERROR_MSG"),
				Localise(L"MODELEDITOR/MODELS/MUTANT_MATERIALS/SHADER_ERROR"), MB_OK );
			return false;
		}
		else if (!softskinned)
		{
			ERROR_MSG("Unable to apply a softskinned shader to an unskinned object.\n");
			::MessageBox( AfxGetApp()->m_pMainWnd->GetSafeHwnd(),
				Localise(L"MODELEDITOR/MODELS/MUTANT_MATERIALS/UNSKINNED_ERROR_MSG"),
				Localise(L"MODELEDITOR/MODELS/MUTANT_MATERIALS/SHADER_ERROR"), MB_OK );
			return false;
		}
	}
	else
	{
		if (softskinned)
		{
			WARNING_MSG("Applying an unskinned shader to a softskinned object.\n");
			if (::MessageBox( AfxGetApp()->m_pMainWnd->GetSafeHwnd(),
				Localise(L"MODELEDITOR/MODELS/MUTANT_MATERIALS/SOFTSKINNED_WARNING_MSG"),
				Localise(L"MODELEDITOR/MODELS/MUTANT_MATERIALS/SOFTSKINNED_WARNING"), MB_OKCANCEL ) == IDCANCEL)
				{
					return false;
				}
		}
		else if (hardskinned)
		{
			WARNING_MSG("Applying an unskinned shader to a hardskinned object.\n");
			if (::MessageBox( AfxGetApp()->m_pMainWnd->GetSafeHwnd(),
				Localise(L"MODELEDITOR/MODELS/MUTANT_MATERIALS/HARDSKINNED_WARNING_MSG"),
				Localise(L"MODELEDITOR/MODELS/MUTANT_MATERIALS/HARDSKINNED_WARNING"), MB_OKCANCEL ) == IDCANCEL)
			{
				return false;
			}
		}
	}

	{
		// See if the # of UVs match between the shader and the primitives.
		CWaitCursor wait;
		Moo::EffectMaterialPtr material = new Moo::EffectMaterial();
		if (material && material->initFromEffect( fxFile ))
		{
			if (material->dualUV() && !dualUV)
			{
				ERROR_MSG( "Unable to apply a dual UV shader to a single UV object.\n" );
				return false;
			}
			else if (!material->dualUV() && dualUV)
			{
				WARNING_MSG( "Applying a single UV shader to a dual UV object.\n" );
				// Don't return false here, it's just a warning
			}
		}
		else
		{
			ERROR_MSG( "ensureShaderCorrect: Cannot open fx file '%s'.\n", fxFile.c_str() );
		}
	}

	return true;
}

bool Mutant::effectHasNormalMap( const BW::string& effectFile )
{
	BW_GUARD;

	if (effectFile == "") return false;
		
	static BW::map< BW::string, bool > s_binormal;

	if (s_binormal.find( effectFile ) != s_binormal.end())
	{
		return s_binormal[ effectFile ];
	}

	Moo::EffectMaterialPtr material = new Moo::EffectMaterial();

	if (!material) return false;
	if (!material->initFromEffect( effectFile )) return false;

	s_binormal[ effectFile ] = false;

	if ( material->pEffect() )
	{
		Moo::EffectMaterial::Properties& properties = material->properties();
		Moo::EffectMaterial::Properties::reverse_iterator it = properties.rbegin();
		Moo::EffectMaterial::Properties::reverse_iterator end = properties.rend();

		for (; it != end; ++it )
		{
			EditorEffectProperty* pProperty = dynamic_cast<EditorEffectProperty*>(it->second.get());
			MF_ASSERT( pProperty );
			if ( MaterialTextureProxy* pTextureProxy = dynamic_cast<MaterialTextureProxy*>(pProperty) )
			{
				BW::string UIWidget = MaterialUtility::UIWidget( pProperty );
				if ((pProperty->name() == BW::string("normalMap")) || (UIWidget == "NormalMap"))
				{
					s_binormal[ effectFile ] = true;
					break;
				}
			}
		}
	}

	return s_binormal[ effectFile ];
}

bool Mutant::doAnyEffectsHaveNormalMap()
{
	BW_GUARD;

	bool isNormalMap = false;

	BW::map< BW::string, MaterialInfo >::iterator it = materials_.begin();
	BW::map< BW::string, MaterialInfo >::iterator end = materials_.end();

	for (; it != end && !isNormalMap; ++it)
	{
		// This should catch all cases
		for (BW::set< Moo::ComplexEffectMaterialPtr >::iterator effectIt = (*it).second.effect.begin();
			effectIt != (*it).second.effect.end() && !isNormalMap; ++effectIt)
		{
			Moo::ComplexEffectMaterialPtr effect = (*effectIt);
			if (effect && effect->baseMaterial()->pEffect() &&
				effect->baseMaterial()->bumpMapped())
			{
				isNormalMap = true;
				break;
			}
		}

		// If there are some effects not yet loaded, do the hacky search for
		// normal maps inside the shaders.
		// TODO: investigate further if this is still needed, if it's possible
		// that an effect is not loaded in ME.
		BW::string materialName = it->first;
		DataSectionPtr data;
		for (unsigned i=0; i < it->second.data.size() && !isNormalMap; i++ )
		{
			data = it->second.data[i];

			BW::vector<BW::string> fxs;
			data->readStrings( "fx", fxs );

			for ( uint32 j = 0; j < fxs.size(); j++ )
			{
				if (effectHasNormalMap( fxs[j] ))
				{
					isNormalMap = true;
					break;
				}
			}
		}
	}
	return isNormalMap;
}


/**
 *	This method checks if a FX file is a sky box shader (doing the xyww
 *	transform) by checking the bool "isBWSkyBox" in the shader.
 */
bool Mutant::effectIsSkybox( const BW::string & effectFile ) const
{
	BW_GUARD;

	if (effectFile.empty())
	{
		return false;
	}
		
	Moo::EffectMaterialPtr material = new Moo::EffectMaterial();

	if (!material->initFromEffect( effectFile ))
	{
		return false;
	}

	if (material->pEffect() &&
		material->pEffect()->pEffect())
	{
		ID3DXEffect* pEffect = material->pEffect()->pEffect();

		D3DXHANDLE hParameter = pEffect->GetParameterByName( 0, "isBWSkyBox" );

		if (hParameter)
		{
			D3DXPARAMETER_DESC desc;
			if (SUCCEEDED( pEffect->GetParameterDesc( hParameter, &desc ) ))
			{
				if (desc.Class == D3DXPC_SCALAR && desc.Type == D3DXPT_BOOL)
				{
					BOOL isSkyBox = FALSE;
					if (SUCCEEDED( pEffect->GetBool( hParameter, &isSkyBox )))
					{
						if (isSkyBox)
						{
							return true;
						}
					}

				}
			}
		}
	}

	return false;
}


/**
 *	This method checks all materials in the model and sets up internal flags
 *	and/or states where needed.
 */
void Mutant::checkMaterials()
{
	BW_GUARD;

	isSkyBox_ = false;

	BW::map< BW::string, MaterialInfo >::iterator it = materials_.begin();
	BW::map< BW::string, MaterialInfo >::iterator end = materials_.end();

	for (; it != end; ++it)
	{
		BW::string materialName = it->first;
		DataSectionPtr data;

		for (uint32 i = 0; i < it->second.data.size(); ++i)
		{
			data = it->second.data[i];

			BW::vector< BW::string > fxs;
			data->readStrings( "fx", fxs );

			for (uint32 j = 0; j < fxs.size(); ++j)
			{
				if (effectIsSkybox( fxs[j] ))
				{
					isSkyBox_ = true;
					return;
				}
			}
		}
	}
}


bool Mutant::materialShader( const BW::string& materialName, const BW::string& matterName, const BW::string& tintName, const BW::string& fxFile, bool undoable /* = true */ )
{
	BW_GUARD;

	if ((matterName == "") || (tintName == "Default"))
	{
		if (materials_.find( materialName ) == materials_.end())
			return false;

		if ( ! ensureShaderCorrect( fxFile, materials_[materialName].format, materials_[materialName].dualUV ))
			return false;

		DataSectionPtr data;
		
		for (unsigned i=0; i<materials_[materialName].data.size(); i++ )
		{
			data = materials_[materialName].data[i];

			if (undoable)
			{
				UndoRedo::instance().add( new UndoRedoOp( 0, data, currVisual_ ));
			}

			instantiateMFM( data );

			// Delete all fx entries initially
			BW::vector<DataSectionPtr> fxs;
			data->openSections( "fx", fxs );
			for ( unsigned i=0; i<fxs.size(); i++ ) 
			{
				data->delChild( fxs[i] );
			}

			if (fxFile != "")
			{
				data->writeString( "fx", fxFile );
			}
			else
			{
				data->delChild( "fx" );
			}
		}
	}
	else
	{
		if (tints_.find( matterName ) == tints_.end())
			return false;
		
		if (tints_[matterName].find( tintName ) == tints_[matterName].end())
			return false;

		if ( ! ensureShaderCorrect( fxFile, tints_[matterName][tintName].format, tints_[matterName][tintName].dualUV ))
			return false;
		
		DataSectionPtr data = tints_[matterName][tintName].data;

		if (data == NULL) return false;

		data = data->openSection("material");

		if (undoable)
		{
			UndoRedo::instance().add( new UndoRedoOp( 0, currModel_, currModel_ ));
		}

		instantiateMFM( data );

		// Delete all fx entries initially
		BW::vector<DataSectionPtr> fxs;
		data->openSections( "fx", fxs );
		for ( unsigned i=0; i<fxs.size(); i++ ) 
		{
			data->delChild( fxs[i] );
		}

		if (fxFile != "")
		{
			data->writeString( "fx", fxFile );
		}
		else
		{
			data->delChild( "fx" );
		}
	}

	if (undoable)
	{
		UndoRedo::instance().barrier( LocaliseUTF8(L"MODELEDITOR/MODELS/MUTANT_MATERIALS/CHANGING_MATERIAL_EFFECT"), true );
	}

	reloadAllLists();

	return true;
}

BW::string Mutant::materialShader( const BW::string& materialName, const BW::string& matterName, const BW::string& tintName )
{
	BW_GUARD;

	DataSectionPtr data;
	
	if ((matterName == "") || (tintName == "Default"))
	{
		if (materials_.find( materialName ) == materials_.end())
			return "";

		data = materials_[materialName].data[0];
	}
	else
	{
		if (tints_.find( matterName ) == tints_.end())
			return "";
		
		if (tints_[matterName].find( tintName ) == tints_[matterName].end())
			return "";

		data = tints_[matterName][tintName].data;

		if (data == NULL) return "";

		data = data->openSection("material");
	}

	if (data == NULL) return "";

	instantiateMFM( data );
	
	return data->readString( "fx", "" );
}

bool Mutant::materialMFM( const BW::string& materialName, const BW::string& matterName, const BW::string& tintName, const BW::string& mfmFile, BW::string* fxFile )
{
	BW_GUARD;

	DataSectionPtr mfmData = BWResource::openSection( mfmFile, false );
	
	if (mfmData == NULL) return false;

	BW::string mfmFx = mfmData->readString( "fx", "" );

	if (mfmFx == "") return false;

	if ((matterName == "") || (tintName == "Default"))
	{
		if (materials_.find( materialName ) == materials_.end())
			return false;

		if ( ! ensureShaderCorrect( mfmFx, materials_[materialName].format, materials_[materialName].dualUV ))
			return false;
	
		DataSectionPtr data;
		
		for (unsigned i=0; i<materials_[materialName].data.size(); i++ )
		{
			data = materials_[materialName].data[i];

			UndoRedo::instance().add( new UndoRedoOp( 0, data, currVisual_ ));

			overloadMFM( data, mfmData );
		}

		//Hmm... Special case if we don't have a tint, we have to reload the material by hand
		if (tintName == "")
		{
			BW::set< Moo::ComplexEffectMaterialPtr >::iterator matIt = materials_[materialName].effect.begin();
			BW::set< Moo::ComplexEffectMaterialPtr >::iterator matEnd = materials_[materialName].effect.end();

			while (matIt != matEnd)
			{
				Moo::ComplexEffectMaterialPtr effect = *matIt++;
				effect->load( data );
			}
		}
	}
	else
	{
		if (tints_.find( matterName ) == tints_.end())
			return false;
		
		if (tints_[matterName].find( tintName ) == tints_[matterName].end())
			return false;

		if ( ! ensureShaderCorrect( mfmFx, tints_[matterName][tintName].format, tints_[matterName][tintName].dualUV ))
			return false;
		
		DataSectionPtr data = tints_[matterName][tintName].data;

		if (data == NULL) return false;

		data = data->openSection("material");

		UndoRedo::instance().add( new UndoRedoOp( 0, data, currModel_ ));

		overloadMFM( data, mfmData );
	}

	UndoRedo::instance().barrier( LocaliseUTF8(L"MODELEDITOR/MODELS/MUTANT_MATERIALS/CHANGING_MATERIAL_MFM"), true );

	reloadAllLists();

	if (fxFile)
		*fxFile = mfmFx;

	return true;
}

void Mutant::tintFlag( const BW::string& matterName, const BW::string& tintName, const BW::string& flagName, uint32 val )
{
	BW_GUARD;

	if (tints_.find( matterName ) == tints_.end())
		return;

	if (tints_[matterName].find( tintName ) == tints_[matterName].end())
		return;
	
	DataSectionPtr data = tints_[matterName][tintName].data->openSection("material");

	UndoRedo::instance().add( new UndoRedoOp( 0, currModel_, currModel_ ));

	instantiateMFM( data );
	data->writeInt( flagName, val );

	UndoRedo::instance().barrier( LocaliseUTF8(L"MODELEDITOR/MODELS/MUTANT_MATERIALS/CHANGING_TINT_FLAG"), true );

	reloadModel();
	reloadBSP();
	triggerUpdate("Materials");
	triggerUpdate("Object");
}

uint32 Mutant::tintFlag( const BW::string& matterName, const BW::string& tintName, const BW::string& flagName )
{
	BW_GUARD;

	if (tints_.find( matterName ) == tints_.end())
		return 0xB0B15BAD;
	
	if (tints_[matterName].find( tintName ) == tints_[matterName].end())
		return 0xB0B15BAD;

	DataSectionPtr data = tints_[matterName][tintName].data;

	if (data == NULL) return 0xB0B15BAD;

	uint32 flag = data->readInt( "material/"+flagName, 0xB0B15BAD );

	if ( flag != 0xB0B15BAD ) return flag;

	//Now handle the case where the material is using an MFM which hasn't been instanciated (legacy)

	BW::string mfmName = data->readString( "material/mfm", "" );

	data = BWResource::openSection( mfmName, false );

	if (data == NULL) return 0xB0B15BAD;

	return data->readInt( flagName, 0xB0B15BAD );
}

void Mutant::materialFlag( const BW::string& materialName, const BW::string& flagName, uint32 val )
{
	BW_GUARD;

	if (materials_.find( materialName ) == materials_.end())
		return;

	for (unsigned i=0; i<materials_[materialName].data.size(); i++ )
	{
		DataSectionPtr data = materials_[materialName].data[i];

		UndoRedo::instance().add( new UndoRedoOp( 0, data, currVisual_, true ));

		instantiateMFM( data );
		data->writeInt( flagName, val );

		// apply change to EffectMaterial as well
		if (flagName == "materialKind")
		{
			ComplexEffectMaterialSet &effMats = materials_[materialName].effect;
			for (ComplexEffectMaterialSet::iterator it = effMats.begin(); it != effMats.end(); it++)
				(*it)->baseMaterial()->materialKind(val);
		}
	}

	UndoRedo::instance().barrier( LocaliseUTF8(L"MODELEDITOR/MODELS/MUTANT_MATERIALS/CHANGING_MATERIAL_FLAG"), true );

	reloadModel();
	reloadBSP();
	triggerUpdate("Materials");
	triggerUpdate("Object");
}

uint32 Mutant::materialFlag( const BW::string& materialName, const BW::string& flagName )
{
	BW_GUARD;

	if (materials_.find( materialName ) == materials_.end())
		return 0;

	DataSectionPtr data = materials_[materialName].data[0];
		
	if (data->findChild( flagName ))
		return data->readInt( flagName, 0 );

	//Now handle the case where the material is using an MFM which hasn't been instanciated (legacy)

	BW::string mfmName = data->readString( "mfm", "" );

	if (mfmName == "") return 0;

	data = BWResource::openSection( mfmName, false );

	if (data == NULL) return 0;

	return data->readInt( flagName, 0 );
}

void Mutant::tintNames( const BW::string& matterName, BW::vector< BW::string >& names )
{
	BW_GUARD;

	if (tints_.find(matterName) == tints_.end()) return;
	
	BW::map < BW::string, TintInfo >::iterator tintIt = tints_[matterName].begin();
	BW::map < BW::string, TintInfo >::iterator tintEnd = tints_[matterName].end();
	while ( tintIt != tintEnd )
	{
		names.push_back( (*tintIt++).first );
	}
}

int Mutant::modelMaterial () const
{
	BW_GUARD;

	if (currVisual_ == NULL) return 0;
	return currVisual_->readInt( "materialKind" , 0 );
}

void Mutant::modelMaterial ( int id )
{
	BW_GUARD;

	if (currVisual_ == NULL) return;

	DataSectionPtr data = currVisual_->openSection( "materialKind", true );

	UndoRedo::instance().add( new UndoRedoOp( 0, data, currVisual_ ));
	
	data->setInt( id );

	UndoRedo::instance().barrier( LocaliseUTF8(L"MODELEDITOR/MODELS/MUTANT_MATERIALS/CHANGING_MATERIAL_KIND"), false );

	triggerUpdate("Object");
}

BW::string Mutant::materialTextureFeedName( const BW::string& materialName, const BW::string& matterName, const BW::string& tintName, const BW::string& propName )
{
	BW_GUARD;

	DataSectionPtr data;
	
	if ((matterName == "") || (tintName == "Default"))
	{
		if (materials_.find( materialName ) == materials_.end())
			return "";

		data = materials_[materialName].data[0];
	}
	else
	{
		if (tints_.find( matterName ) == tints_.end())
			return "";
		
		if (tints_[matterName].find( tintName ) == tints_[matterName].end())
			return "";
		
		data = tints_[matterName][tintName].data;

		if (data == NULL) return "";

		data = data->openSection("material");
	}

	if (data == NULL) return "";

	instantiateMFM( data );

	BW::string feedName = "";

	BW::vector<DataSectionPtr> pProps;
	data->openSections( "property", pProps );

	BW::vector< DataSectionPtr >::iterator propsIt = pProps.begin();
	BW::vector< DataSectionPtr >::iterator propsEnd = pProps.end();
	while (propsIt != propsEnd)
	{
		DataSectionPtr prop = *propsIt++;

		if (propName == prop->asString())
		{
			feedName = prop->readString( "TextureFeed", feedName);
		}
	}

	return feedName;
}

BW::string Mutant::materialPropertyVal( const BW::string& materialName, const BW::string& matterName, const BW::string& tintName, const BW::string& propName, const BW::string& dataType )
{
	BW_GUARD;

	DataSectionPtr data;
	
	if ((matterName == "") || (tintName == "Default"))
	{
		if (materials_.find( materialName ) == materials_.end())
			return "";

		data = materials_[materialName].data[0];
	}
	else
	{
		if (tints_.find( matterName ) == tints_.end())
			return "";
		
		if (tints_[matterName].find( tintName ) == tints_[matterName].end())
			return "";
		
		data = tints_[matterName][tintName].data;

		if (data == NULL) return "";

		data = data->openSection("material");
	}

	if (data == NULL) return "";

	instantiateMFM( data );

	BW::string val = "";

	BW::vector<DataSectionPtr> pProps;
	data->openSections( "property", pProps );

	BW::vector< DataSectionPtr >::iterator propsIt = pProps.begin();
	BW::vector< DataSectionPtr >::iterator propsEnd = pProps.end();
	while (propsIt != propsEnd)
	{
		DataSectionPtr prop = *propsIt++;

		if (propName == prop->asString())
		{
			val = prop->readString( dataType, val);
		}
	}

	return val;
}

void Mutant::changeMaterialFeed( DataSectionPtr data, const BW::string& propName, const BW::string& feedName )
{
	BW_GUARD;

	if (data == NULL) return;
	
	UndoRedo::instance().add( new UndoRedoOp( 0, data ));

	instantiateMFM( data );

	BW::vector<DataSectionPtr> pProps;
	data->openSections( "property", pProps );

	BW::vector< DataSectionPtr >::iterator propsIt = pProps.begin();
	BW::vector< DataSectionPtr >::iterator propsEnd = pProps.end();
	while (propsIt != propsEnd)
	{
		DataSectionPtr prop = *propsIt++;

		if (propName == prop->asString())
		{
			if (prop->readString( "TextureFeed", "" ) != "")
			{
				if ( feedName != "")
				{
					prop->writeString( "TextureFeed", feedName );
				}
				else
				{
					BW::string textureName = prop->readString( "TextureFeed/default", "" );
					prop->delChild( "TextureFeed" );
					prop->writeString( "Texture", textureName );
				}
			}
			else
			{
				BW::string textureName = prop->readString( "Texture", "" );
				prop->delChild( "Texture" );
				prop->writeString( "TextureFeed", feedName );
				data = prop->openSection( "TextureFeed" );
				data->writeString( "default", textureName );
			}
		}
	}
}

void Mutant::materialTextureFeedName( const BW::string& materialName, const BW::string& matterName, const BW::string& tintName, const BW::string& propName, const BW::string& feedName )
{
	BW_GUARD;

	DataSectionPtr data;
	
	if ((matterName == "") || (tintName == "Default"))
	{
		if (materials_.find( materialName ) == materials_.end())
			return;
		
		for (unsigned i=0; i<materials_[materialName].data.size(); i++ )
		{
			data = materials_[materialName].data[i];

			changeMaterialFeed( data, propName, feedName );
		}

		dirty( currVisual_ );
	}
	else
	{
		if (tints_.find( matterName ) == tints_.end())
			return;
		
		if (tints_[matterName].find( tintName ) == tints_[matterName].end())
			return;
		
		data = tints_[matterName][tintName].data;

		if (data == NULL) return;

		data = data->openSection("material");

		changeMaterialFeed( data, propName, feedName );

		dirty( currModel_ );
	}

	UndoRedo::instance().barrier( LocaliseUTF8(L"MODELEDITOR/MODELS/MUTANT_MATERIALS/CHANGING_TEXTURE_FEED_NAME"), false );

	reloadModel();
	triggerUpdate("Materials");
}

BW::string Mutant::exposedToScriptName( const BW::string& matterName, const BW::string& tintName, const BW::string& propName )
{
	BW_GUARD;

	if ((matterName == "") || (tintName == "Default"))
		return "";
		
	if (tints_.find( matterName ) == tints_.end())
		return "";
	
	if (tints_[matterName].find( tintName ) == tints_[matterName].end())
		return "";
	
	DataSectionPtr data = tints_[matterName][tintName].data;

	if (data == NULL) return "";
	
	BW::vector<DataSectionPtr> pProps;
	data->openSections( "property", pProps );

	BW::vector< DataSectionPtr >::iterator propsIt = pProps.begin();
	BW::vector< DataSectionPtr >::iterator propsEnd = pProps.end();
	while (propsIt != propsEnd)
	{
		DataSectionPtr prop = *propsIt++;

		if (propName == prop->readString( "name", "" ))
		{
			return propName;
		}
	}

	return "";
}

void Mutant::toggleExposed( const BW::string& matterName, const BW::string& tintName, const BW::string& propName )
{
	BW_GUARD;

	if ((matterName == "") || (tintName == "Default"))
		return;
	
	if (tints_.find( matterName ) == tints_.end())
		return;
	
	if (tints_[matterName].find( tintName ) == tints_[matterName].end())
		return;
	
	DataSectionPtr data = tints_[matterName][tintName].data;

	if (data == NULL) return;

	UndoRedo::instance().add( new UndoRedoOp( 0, data, currModel_ ));

	DataSectionPtr materialData = data->openSection("material");

	if (materialData == NULL) return;
	
	instantiateMFM( materialData );

	BW::vector<DataSectionPtr> pProps;
	data->openSections( "property", pProps );
	BW::vector< DataSectionPtr >::iterator propsIt = pProps.begin();
	BW::vector< DataSectionPtr >::iterator propsEnd = pProps.end();
	while (propsIt != propsEnd)
	{
		DataSectionPtr prop = *propsIt++;

		if (propName == prop->readString( "name", "" ))
		{
			//Remove the "property" section
			data->delChild( prop );

			UndoRedo::instance().barrier( LocaliseUTF8(L"MODELEDITOR/MODELS/MUTANT_MATERIALS/DISABLING_PYTHON"), false );

			return;
		}
	}

	//Add a new "property" section
	data = data->newSection( "property" );
	data->writeString( "name", propName );
	data->writeVector4( "default", getExposedVector4( matterName, tintName, propName, "", "" ) );

	UndoRedo::instance().barrier( LocaliseUTF8(L"MODELEDITOR/MODELS/MUTANT_MATERIALS/ENABLING_PYTHON"), false );
}

Vector4 Mutant::getExposedVector4( const BW::string& matterName, const BW::string& tintName, const BW::string& descName,  const BW::string& propType, const BW::string& val )
{
	BW_GUARD;

	// if propType and val are empty then that means we want the default value
	Vector4 exposedVector4( 0.f, 0.f, 0.f, 0.f );

	if ((matterName == "") || (tintName == "Default"))
		return exposedVector4;
	
	if (tints_.find( matterName ) == tints_.end())
		return exposedVector4;
	
	if (tints_[matterName].find( tintName ) == tints_[matterName].end())
		return exposedVector4;

	if ( propType.empty() && val.empty() )
	{
		Moo::ComplexEffectMaterialPtr material = tints_[matterName][tintName].effect;
		if ( material->baseMaterial()->pEffect() )
		{

			Moo::EffectMaterial::Properties& properties = material->baseMaterial()->properties();
			Moo::EffectMaterial::Properties::iterator it = properties.begin();
			Moo::EffectMaterial::Properties::iterator end = properties.end();

			while (it != end)
			{
				EditorEffectProperty* pProperty = dynamic_cast<EditorEffectProperty*>(it->second.get());

				if (pProperty != NULL &&
					pProperty->name() == descName)
				{
					if (MaterialBoolProxy* pBoolProxy = dynamic_cast<MaterialBoolProxy*>( pProperty ))
					{
						exposedVector4.x = pBoolProxy->get() ? 1.f : 0.f;
					}
					else if (MaterialIntProxy* pIntProxy = dynamic_cast<MaterialIntProxy*>( pProperty ))
					{
						exposedVector4.x = float( pIntProxy->get() );
					}
					else if (MaterialFloatProxy* pFloatProxy = dynamic_cast<MaterialFloatProxy*>( pProperty ))
					{
						exposedVector4.x = pFloatProxy->get();
					}
					else if (MaterialVector4Proxy* pVector4Proxy = dynamic_cast<MaterialVector4Proxy*>( pProperty ))
					{
						pVector4Proxy->getVector( exposedVector4 );
					}

				}
				++it;
			}
		}
	}
	else
	{
		DataSectionPtr tempSection = new XMLSection("temp");
		tempSection->writeString( propType, val );
		if ( propType == "Bool" )
			exposedVector4[0] = tempSection->readBool( propType, false ) ? 1.f : 0.f;
		else if ( propType == "Int" )
			exposedVector4[0] = (float)tempSection->readInt( propType, 0 );
		else if ( propType == "Float" )
			exposedVector4[0] = tempSection->readFloat( propType, 0.f );
		else
			exposedVector4 = tempSection->readVector4( propType, exposedVector4 );
	}
	return exposedVector4;
}

uint32 Mutant::materialSectionTextureMemUsage( DataSectionPtr data, BW::set< BW::string >& texturesDone )
{
	BW_GUARD;

	uint32 size = 0;

	BW::string name;

	instantiateMFM( data );

	BW::vector< DataSectionPtr > props;

	data->openSections( "property", props );

	for (unsigned i=0; i<props.size(); i++)
	{
		name = props[i]->readString( "Texture", "" );
		if ( name != "" )
		{
			if ( texturesDone.find( name ) != texturesDone.end() )
				continue;
			texturesDone.insert( name );
			Moo::BaseTexturePtr baseTexture = Moo::TextureManager::instance()->get( name, true, false, false );
			if (baseTexture)
			{
				size += baseTexture->textureMemoryUsed();
			}
		}
		else
		{
			DataSectionPtr textureFeed = props[i]->openSection( "TextureFeed" );

			if (textureFeed != NULL) 
			{
				name = textureFeed->readString( "default", "" );
				if (( name == "" ) || ( texturesDone.find( name ) != texturesDone.end() ))
					continue;
				texturesDone.insert( name );
				Moo::BaseTexturePtr baseTexture = Moo::TextureManager::instance()->get( name, true, false, false );
				if (baseTexture)
				{
					size += baseTexture->textureMemoryUsed();
				}
			}
		}
	}

	return size;
}

uint32 Mutant::recalcTextureMemUsage()
{
	BW_GUARD;

	texMem_ = 0;
	DataSectionPtr data;
	BW::set< BW::string > texturesDone;

	BW::map< BW::string, MaterialInfo >::iterator matIt = materials_.begin();
	BW::map< BW::string, MaterialInfo >::iterator matEnd = materials_.end();

	for ( ;matIt != matEnd; ++matIt )
	{
		data = matIt->second.data[0];

		if (data == NULL) continue;

		texMem_ += materialSectionTextureMemUsage( data, texturesDone );
	}

	TintMap::iterator dyeIt = tints_.begin();
	TintMap::iterator dyeEnd = tints_.end();

	for (; dyeIt != dyeEnd; ++dyeIt)
	{
		BW::map< BW::string, TintInfo >::iterator tintIt = dyeIt->second.begin();
		BW::map< BW::string, TintInfo >::iterator tintEnd = dyeIt->second.end();
		for (; tintIt != tintEnd; ++tintIt)
		{
			data = tintIt->second.data;

			if (data == NULL) continue;

			data = data->openSection( "material" );

			if (data == NULL) continue;

			texMem_ += materialSectionTextureMemUsage( data, texturesDone );
		}
	}

	texMemDirty_ = true;

	return texMem_;
}


Moo::EffectMaterialPtr Mutant::getEffectForTint(
	const BW::string& matterName, const BW::string& tintName, const BW::string& materialName )
{
	BW_GUARD;

	if ((matterName == "") || (tintName == ""))
	{
		if ( materialName != "" )
		{
			return (*materials_[materialName].effect.begin())->baseMaterial();
		}
		else
			return NULL;
	}
	
	TintMap::iterator matterIt;
	matterIt = tints_.find( matterName );
	if ( matterIt == tints_.end() )
		return NULL;

	BW::map < BW::string, TintInfo >::iterator tintIt;
	tintIt = tints_[matterName].find( tintName );
	if ( tintIt == tints_[matterName].end() )
		return NULL;
	
	return tintIt->second.effect->baseMaterial();
}


void Mutant::updateTintFromEffect( const BW::string& matterName,
									const BW::string& tintName )
{
	BW_GUARD;

	// update tints
	Tint * pTint = NULL;
	superModel_->topModel(0)->getDye( matterName, tintName, &pTint );
	if (pTint)
	{
		pTint->updateFromEffect();

		// Fix in the fashions vector
		if (tints_.find( matterName ) != tints_.end())
		{
			if (tints_[matterName].find( tintName ) != tints_[matterName].end())
			{
				MaterialFashionVector::iterator fit = std::find(
					materialFashions_.begin(),
					materialFashions_.end(),
					tints_[matterName][tintName].dye.get() );

				tints_[matterName][tintName].dye = superModel_->getDye(
					matterName, tintName );

				if (fit != materialFashions_.end())
				{
					(*fit) = tints_[matterName][tintName].dye;
				}
			}
		}
	}
}
BW_END_NAMESPACE

