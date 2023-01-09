/******************************************************************************
BigWorld Technology 
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

#ifndef SCALEFORM_FONT_CONFIG_HPP
#define SCALEFORM_FONT_CONFIG_HPP
#if SCALEFORM_SUPPORT

#include <string>
#include "cstdmf/smartpointer.hpp"
#include "resmgr/bwresource.hpp"

class GFxLoader;
class GFxFontLib;
class GFxFontMap;

namespace Scaleform {

	class FontConfig;

	typedef SmartPointer<FontConfig> FontConfigPtr;

	class FontConfig : public ReferenceCount
	{
	public:
		FontConfig( DataSectionPtr ds, const BW::string& cfgName );
		~FontConfig();

		void apply( GFxLoader& loader );

		static FontConfigPtr get( const BW::string& cfg );

	private:
		GPtr<GFxFontLib> fontLib_;
		GPtr<GFxFontMap> fontMap_;

	private:
		typedef BW::map<BW::string, FontConfig*> FontConfigMap;
		static FontConfigMap s_fontConfigs;

		FontConfigMap::iterator findCachedConfig( FontConfig* ptr );
	};

	

} //namespace Scaleform

#endif // #if SCALEFORM_SUPPORT
#endif // #define SCALEFORM_FONT_CONFIG_HPP
