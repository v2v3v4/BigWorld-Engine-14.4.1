#include "pch.hpp"
#pragma warning ( disable: 4503 )
#pragma warning ( disable: 4786 )


#include "expsets.hpp"
#include <CommDlg.h>

#include <string>
#include "cstdmf/string_utils.hpp"

#ifndef CODE_INLINE
#include "expsets.ipp"
#endif

BW_BEGIN_NAMESPACE

//the instance
extern HINSTANCE hInstance;


ExportSettings::ExportSettings()
: includeMeshes_( true ),
  includeEnvelopesAndBones_( false ),
  includeNodes_( true ),
  includeMaterials_( true ),
  includeAnimations_( false ),
  useCharacterMode_( false ),
  includePortals_( false ),
  worldSpaceOrigin_( false ),
  resolvePaths_( true ),
  animationName_( "anim" ),
  staticFrame_( 0 ),
  frameFirst_( 0 ),
  frameLast_( 100 ),
  unitScale_( 0.1f ),
  localHierarchy_( false ),
  exportMode_( NORMAL ),
  nodeFilter_( ALL ),
  transformToOrigin_( false ),
  allowScale_( false ),
  bumpMapped_( false ),
  fixCylindrical_( false ),
  keepExistingMaterials_( true ),
  snapVertices_( false ),
  stripRefPrefix_( false ),
  maxBones_( 17 ),
  exportAnim_( false ),
  referenceNode_( false ),
  disableVisualChecker_( false ),
  useLegacyScaling_( false ),
  useLegacyOrientation_( false ),
  sceneRootAdded_( false ),
  copyExternalTextures_( true ),
  copyTexturesTo_( "" )
{
}

ExportSettings::~ExportSettings()
{
}

BW::string ExportSettings::getReferenceFile( HWND hWnd )
{
	OPENFILENAME	ofn;
	memset( &ofn, 0, sizeof(ofn) );

	wchar_t * filters = L"MFX Files\0*.MFX\0\0";

	wchar_t	filename[512] = L"";

	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hWnd;
	ofn.lpstrFilter = filters;
	ofn.lpstrFile = filename;
	ofn.nMaxFile = sizeof(filename);
	ofn.lpstrTitle = L"Select reference hierarchy file";
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST |
		OFN_NOCHANGEDIR | OFN_READONLY | OFN_HIDEREADONLY;
	ofn.lpstrDefExt = L"MFX";


	if (!GetOpenFileName( &ofn )) return "";

	return bw_wtoutf8( filename );
}

ExportSettings& ExportSettings::instance()
{
	static ExportSettings expsets;
	return expsets;
}


std::ostream& operator<<(std::ostream& o, const ExportSettings& t)
{
	o << "ExportSettings\n";
	return o;
}

BW_END_NAMESPACE

/*expsets.cpp*/