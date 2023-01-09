#include "pch.hpp"
#include "effect_includes.hpp"

#include "effect_manager.hpp"
#include "effect_compiler.hpp"

#include "resmgr/auto_config.hpp"


BW_BEGIN_NAMESPACE

namespace {
	AutoConfigString s_shaderIncludePath( "system/shaderIncludePaths" );
} // anonymous namespace

namespace Moo {

/**
*	Constructor
*/
EffectIncludes::EffectIncludes()
{
	BW_GUARD;
	this->addIncludePaths( s_shaderIncludePath );
}

/**
*	This method adds the include paths from pathString to the include 
*	search path.
*	@param pathString the paths to add, paths can be separated by ;
*/
void EffectIncludes::addIncludePaths( const BW::string& pathString )
{
	BW_GUARD;
	bw_tokenise< BW::string >( pathString, ";", includePaths_ );
	for (size_t i = 0; i < includePaths_.size(); ++i)
	{
		includePaths_[i] = BWUtil::normalisePath( includePaths_[i] );
	}
}

/**
*	Finds the given include name based first on the current path and then
*	the previously added include paths. 
*	@return		Resolved path name, if found. Empty string if not found.
*/
BW::string EffectIncludes::resolveInclude( const BW::string& name )
{
	BW_GUARD;
	if (BWResource::fileExists( name ))
	{
		return name;
	}

	BW::string pathname = currentPath_ + name;
	bool found = BWResource::fileExists( pathname );

	// Not found in the current path, go through the rest.
	if (!found)
	{
		IncludePaths::const_iterator it  = includePaths_.begin();
		IncludePaths::const_iterator end = includePaths_.end();
		while (it != end)
		{
			pathname = *(it++) + "/" + name;
			if (BWResource::fileExists( pathname ))
			{
				found = true;
				break;
			}
		}
	}

	return found ? pathname : "";
}

/**
*	Resets the dependency list as caught during the compilation process.
*/
void EffectIncludes::resetDependencies() 
{
	includesNames_.clear();
}

/**
*	Returns dependency list as caught during the compilation process.
*/
EffectIncludes::StringList& EffectIncludes::dependencies()
{
	return includesNames_;
}

/**
*	Sets the current path (usually the path containing the shader about to
*	be recompiled) used as the root path for resolving includes.
*/
void EffectIncludes::currentPath( const BW::string& currentPath ) 
{
	currentPath_ = currentPath;
}

/**
*	Returns the current path used for resolving includes.
*/
const BW::string& EffectIncludes::currentPath() const 
{
	return currentPath_;
}


/**
*	Implementation of ID3DXInclude::Open
*/
HRESULT WINAPI EffectIncludes::Open( D3DXINCLUDE_TYPE IncludeType, 
									LPCSTR pFileName,
									LPCVOID pParentData, 
									LPCVOID *ppData, 
									UINT *pBytes )
{
	BW_GUARD;

	*ppData = NULL;
	*pBytes = 0;
	BinaryPtr pData;

	BWResource::instance().purge( pFileName, true );

	DataSectionPtr pFile = BWResource::openSection( pFileName );
	if (pFile)
	{
		// DEBUG_MSG( "EffectIncludes::Open - opening %s\n", 
		//	pFileName.c_str() );
		pData = pFile->asBinary();
	}
	else
	{
		BW::StringRef name = BWResource::getFilename( pFileName );
		BW::string pathname = currentPath_ + name;
		pFile = BWResource::openSection( pathname );
		if (pFile)
		{
			pData = pFile->asBinary();
		}
		else
		{
			IncludePaths::const_iterator it = includePaths_.begin();
			IncludePaths::const_iterator end = includePaths_.end();
			while (it != end)
			{
				pathname = *(it++) + "/" + name;
				pFile = BWResource::openSection( pathname );
				if (pFile)
				{
					pData = pFile->asBinary();
					break;
				}
			}
		}
	}

	if (pData)
	{
		*ppData = pData->data();
		*pBytes = (UINT)pData->len();
		includes_.push_back( std::make_pair( *ppData, pData ) );

		includesNames_.push_back( pFileName );
	}
	else
	{
		ASSET_MSG( "EffectIncludes::Open - Failed to load %s\n", pFileName );
		return E_FAIL;
	}

	return S_OK;
}

/**
*	Implementation of ID3DXInclude::Close
*/
HRESULT WINAPI EffectIncludes::Close( LPCVOID pData )
{
	BW_GUARD;
	BW::vector<IncludeFile>::iterator it = includes_.begin();
	BW::vector<IncludeFile>::iterator end = includes_.end();
	while (it != end)
	{
		if ((*it).first == pData)
		{
			includes_.erase( it );
			break;
		}
		++it;
	}
	return S_OK;
}

} // namespace Moo

BW_END_NAMESPACE

// effect_includes.cpp
