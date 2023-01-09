#include "pch.hpp"
#include "material_kinds.hpp"
#include "cstdmf/debug.hpp"
#include "resmgr/auto_config.hpp"
#include "resmgr/string_provider.hpp"

DECLARE_DEBUG_COMPONENT2( "Romp", 0 )

BW_BEGIN_NAMESPACE

// Implementation of the singleton static pointer.
BW_INIT_SINGLETON_STORAGE( MaterialKinds );

static AutoConfigString s_materialKinds( "environment/materialKinds" );


/**
 *	This method returns the material kind for a given texture resource name.
 *
 *	@param	textureName		texture resource name.
 *	@param	isTextureNameFormatted	Has the texture name already been formatted.
 *
 *	@return	A uint32 material kind for the resource, or 0 by default.
 */
uint32 MaterialKinds::get( const BW::StringRef & textureName,
	bool isTextureNameFormatted )
{
	uint32 kind = 0;
	StringRefUnorderedMap< uint32 >::iterator it;

	if (isTextureNameFormatted)
	{
		it = textureToID_.find( textureName );
	}
	else
	{
		char buffer[BW_MAX_PATH];
		it = textureToID_.find( MaterialKinds::format( textureName, buffer,
			ARRAY_SIZE( buffer ) ) );
	}

    if (it != textureToID_.end())
	{
		kind = it->second;
	}

	return kind;
}


/**
 *	This method returns a user string associated with a material kind.
 *
 *	@param	materialKind				id of the materialKind
 *	@param	keyName						name of the user string requested.
 *
 *	@return the requested string, or "invalid material kind"
 */
BW::string MaterialKinds::userString( uint32 materialKind,
											 const BW::StringRef & keyName )
{
	DataSectionPtr pSection = this->userData( materialKind );
	if (pSection)
	{
		return pSection->readString( keyName );
	}

	BW::string err = LocaliseUTF8(L"PHYSICS/MATERIAL_KINDS/INVALID_MATERIAL_KIND");
	return err;
}


/**
 *	This method retuns the data section associated with a material kind.
 *
 *	@param	materialKind	id of the materialKind
 *	@return DataSectionPtr	the data section associated with the material kind.
 */
DataSectionPtr MaterialKinds::userData( uint32 materialKind )
{
	Map::iterator it = materialKinds_.find( materialKind );
	if (it != materialKinds_.end())
	{
		return it->second;
	}
	return NULL;
}


/**
 *	This method retuns the data section associated with a material kind's
 *	texture map.
 *
 *	@param	materialKind	id of the materialKind
 *	@param	textureName		resource id of the texture map
 *	@return DataSectionPtr	the data section associated with the texture map.
 */
DataSectionPtr MaterialKinds::textureData( uint32 materialKind,
										const BW::StringRef & textureName )
{
	Map::iterator it = materialKinds_.find( materialKind );
	if (it != materialKinds_.end())
	{
		//the material kinds.xml file contains no terrain map extensions.
		BW::StringRef baseFilename = BWResource::removeExtension(textureName);

		DataSectionPtr pMK = it->second;
		DataSection::iterator tit = pMK->begin();
		DataSection::iterator ten = pMK->end();
		while ( tit != ten )
		{
			if ( (*tit)->asString() == baseFilename )
			{
				return *tit;
			}
			++tit;
		}
	}
	return NULL;
}


/**
 *  Indicates whether the provided material kind is valid or not.
 *
 *  @param	materialKind	id of the materialKind
 *  @return bool			validity
 */
bool MaterialKinds::isValid( uint32 materialKind )
{
	Map::iterator it = materialKinds_.find( materialKind );
	return it != materialKinds_.end();
}


/**
 *	Constructor.
 */
MaterialKinds::MaterialKinds()
{
}


/**
 *	This method initialises any objects that require initialisation in the
 *	library.
 *
 *	@return		True if init went on without errors, false otherwise.
 */
bool MaterialKinds::doInit()
{
	DataSectionPtr pSection = BWResource::openSection( s_materialKinds );
	if (pSection)
	{
		DataSectionIterator it = pSection->begin();
		while (it != pSection->end())
		{
			this->addMaterialKind( *it );
			++it;
		}

		INFO_MSG( "Material Kinds loaded from %s\n",
			s_materialKinds.value().c_str() );
	}
	else
	{
		ERROR_MSG( "Material Kinds file not found at %s\n",
			s_materialKinds.value().c_str() );
		return false;
	}
	return true;
}


/**
 *	This method finalises any objects that require finalises in the
 *	library, releasing resources.
 *
 *	@return		True if fini went on without errors, false otherwise.
 */
bool MaterialKinds::doFini()
{
	return true;
}


/**
 *	This private method adds a material kind to our map.
 */
void MaterialKinds::addMaterialKind( DataSectionPtr pSection )
{
	uint32 id = pSection->readInt( "id" );
	DataSectionIterator it = pSection->begin();
	while (it != pSection->end())
	{
		DataSectionPtr pSection = *it++;
		if (pSection->sectionName() == "terrain")
		{
			textureToID_[ pSection->asString() ] = id;
		}
	}
	materialKinds_[ id ] = pSection;
}


/**
 *	This public static method formats a texture resource name by normalising
 *	all slashes and removing the extension.
 */
BW::StringRef MaterialKinds::format( const StringRef & textureName,
	char * outPtr, size_t maxSize )
{
	MF_ASSERT( outPtr && maxSize );
	MF_ASSERT( textureName.size() < maxSize );
	size_t size = std::min( textureName.size(), maxSize - 1 );
	size_t i = 0;
	for (; i < size; ++i)
	{
		const char ch = textureName[i];
		if (ch == '\\')
		{
			// change '\' to '/'
			outPtr[i] = '/';
		}
		else if (ch != '.')
		{
			// copy character
			outPtr[i] = ch;
		}
		else
		{
			// break on first '.'
			break;
		}
	}

	// null terminate, handling case where input is empty
	outPtr[i] = 0;

	// return substring without extension
	return BW::StringRef( outPtr, i );
}



#ifdef EDITOR_ENABLED
/**
 *	This method is used by editors to populate a combo box with
 *	ID, Description pairs for material kinds.  The passed in
 *	datasection ends up containing a list of all material kinds.
 */
void MaterialKinds::populateDataSection( DataSectionPtr pSection )
{
	BW::map<uint32, DataSectionPtr>::iterator it = materialKinds_.begin();
	while (it != materialKinds_.end())
	{
		pSection->writeInt( it->second->readString("desc"), it->first );
		it++;
	}
}

/**
 *	This method creates a description map (string -> int) used by WorldEditor.
 */
void MaterialKinds::createDescriptionMap( StringHashMap<uint32>& retMap )
{
	BW::map<uint32, DataSectionPtr>::iterator it = materialKinds_.begin();
	while (it != materialKinds_.end())
	{
		retMap.insert( std::make_pair(it->second->readString("desc").c_str(),it->first) );
		it++;
	}
}

/**
 *	This method reloads material kinds, but note it does not kick on any
 *	dependent tasks, (like recreating the dominant texture maps).  That is
 *	up to the caller.
 */
void MaterialKinds::reload()
{
	materialKinds_.clear();
	textureToID_.clear();
	BWResource::instance().purge( s_materialKinds.value() );	
	this->doInit();
}

#endif //EDITOR_ENABLED

BW_END_NAMESPACE

// material_kinds.cpp

