#include "pch.hpp"
#include "effect_helpers.hpp"
#include "effect_compiler.hpp"


BW_BEGIN_NAMESPACE

namespace Moo {
	namespace EffectHelpers {

	/**
	 *	
	 */
	BW::string fxoName( const BW::string& resourceID )
	{
		BW_GUARD;
		BW::StringRef fileName   = BWResource::removeExtension( resourceID );
		BW::string objectName = !EffectCompiler::fxoInfix().empty() 
			? fileName + "." + EffectCompiler::fxoInfix() + ".fxo"
			: fileName + ".fxo";

		return objectName;
	}


	/**
	 *	
	 */
	bool fxName( const StringRef& resourceID, BW::string & effectResource )
	{
		BW_GUARD;
		BW::StringRef fileName = BWResource::removeExtension( resourceID );
		BW::StringRef infix = BWResource::getExtension( fileName );
		if (!EffectCompiler::fxoInfix().empty())
		{
			if (infix != EffectCompiler::fxoInfix())
			{
				return false;
			}
			fileName = BWResource::removeExtension( fileName );
		}
		else if (!infix.empty())
		{
			return false;
		}

		effectResource = fileName + ".fx";
		return true;
	}


	/**
	 *	
	 */
	BinaryPtr loadEffectBinary( const BW::string& resourceID )
	{
		BW_GUARD;
#if EDITOR_ENABLED
		MF_ASSERT( BWResource::instance().pathIsRelative( resourceID ) );
#endif
		BW::string fxoName = EffectHelpers::fxoName( resourceID );

		DataSectionPtr pSection =
			BWResource::instance().rootSection()->openSection( fxoName );
		BWResource::instance().purge( fxoName, true );

		if (pSection)
		{
			return pSection->readBinary( "effect" );
		}
		else
		{
			ASSET_MSG("EffectHelpers::loadEffectBinary: "
				"unable to load compiled effect '%s'\n", fxoName.c_str() );
			return NULL;
		}
	}

	} // namespace EffectHelpers
} // namespace Moo

BW_END_NAMESPACE

// effect_helpers.cpp
