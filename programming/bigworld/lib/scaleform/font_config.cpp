#include "pch.hpp"
#if SCALEFORM_SUPPORT
#include "font_config.hpp"
#include "manager.hpp"

#include <GFxFontLib.h>
#include <GFxLoader.h>


BW_BEGIN_NAMESPACE

namespace {

	GFxFontMap::MapFontFlags parseMappingFlags( const BW::string& flags )
	{
		if (flags == "NORMAL")
		{
			return GFxFontMap::MFF_Normal;
		}
		else if (flags == "BOLD")
		{
			return GFxFontMap::MFF_Bold;
		}
		else if (flags == "ITALIC")
		{
			return GFxFontMap::MFF_Italic;
		}
		else if (flags == "BOLDITALIC")
		{
			return GFxFontMap::MFF_BoldItalic;
		}
		else
		{
			return GFxFontMap::MFF_Original;
		}

	}

} // anonymous namespace

namespace Scaleform {

	FontConfig::FontConfigMap FontConfig::s_fontConfigs;

	FontConfig::FontConfig( DataSectionPtr ds, const BW::string& cfgName )
	{
		BW::string basePath = BWResource::getFilePath( cfgName );

		// Get font libs
		BW::vector<BW::string> fontLibs;
		ds->readStrings( "lib", fontLibs );
		if (!fontLibs.empty())
		{
			fontLib_ = *new GFxFontLib;
			for(size_t i = 0; i < fontLibs.size(); i++)
			{
				GPtr<GFxMovieDef> pdef = Manager::instance().loadFontMovie( basePath + fontLibs[i] );
				if (pdef)
				{
					fontLib_->AddFontsFrom( pdef, 0 );
				}
			}
		}

		// Setup font map
		BW::vector<DataSectionPtr> fontMappings;
		ds->openSections( "mapping", fontMappings );
		if (!fontMappings.empty())
		{
			fontMap_ = *new GFxFontMap;
			for(size_t i = 0; i < fontMappings.size(); i++)
			{
				BW::string mappingName = fontMappings[i]->asString();
				BW::string familyName = fontMappings[i]->readString( "family" );
				float scaleFactor = fontMappings[i]->readFloat( "scaleFactor", 1.0f );
				GFxFontMap::MapFontFlags mff = parseMappingFlags( fontMappings[i]->readString( "flags" ) );

				fontMap_->MapFont( mappingName.c_str(), familyName.c_str(), mff, scaleFactor );
			}
		}

		s_fontConfigs[cfgName] = this;
	}

	FontConfig::~FontConfig()
	{
		FontConfigMap::iterator it = findCachedConfig( this );
		if (it != s_fontConfigs.end())
		{
			s_fontConfigs.erase( it );
		}
	}

	void FontConfig::apply( GFxLoader& loader )
	{
		loader.SetFontLib( fontLib_ );
		loader.SetFontMap( fontMap_ );
	}

	//static 
	FontConfigPtr FontConfig::get( const BW::string& cfg )
	{
		FontConfigMap::iterator it = s_fontConfigs.find( cfg );
		if (it != s_fontConfigs.end())
		{
			return it->second;
		}

		DataSectionPtr ds = BWResource::openSection( cfg );
		if (!ds)
		{
			return NULL;
		}

		return new FontConfig( ds, cfg );
	}

	//static
	FontConfig::FontConfigMap::iterator 
	FontConfig::findCachedConfig( FontConfig* ptr )
	{
		for (FontConfigMap::iterator it = s_fontConfigs.begin();
			it != s_fontConfigs.end(); it++)
		{
			if (it->second == this)
			{
				return it;
			}
		}

		return s_fontConfigs.end();
	}


}	//namespace Scaleform

BW_END_NAMESPACE

#endif //#if SCALEFORM_SUPPORT
