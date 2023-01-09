#ifndef EXPSETS_HPP
#define EXPSETS_HPP

#pragma warning ( disable : 4530 )


#include <iostream>

#include <windows.h>

#include <maya/MDistance.h>

#include "cstdmf/bw_string.hpp"
#include "cstdmf/smartpointer.hpp"

BW_BEGIN_NAMESPACE

class DataSection;

typedef SmartPointer<DataSection> DataSectionPtr;

class ExportSettings
{
public:
	~ExportSettings();

	void setSettingFileName( const BW::string& settingFilename )
	{
		settingFilename_ = settingFilename;
	}
	bool readSettings();
	bool readSettings( const BW::string & fileName, bool root = true );
	bool readSettings( const DataSectionPtr & pRoot );
	bool writeSettings();
	bool writeSettings( const BW::string & fileName, bool root = true );
	bool writeSettings( DataSectionPtr & pRoot );

	void setExportMeshes( bool b );
	void setExportEnvelopesAndBones( bool b );
	void setExportNodes( bool b );
	void setExportMaterials( bool b );
	void setExportAnimations( bool b );
	void setUseCharacterMode( bool b );

	void setIncludePortals( bool b );
	void setWorldSpaceOrigin( bool b );
	void setResolvePaths( bool b );
	void setUnitScale( float f );
	void setLocalHierarchy( bool b );

	void setAnimationName( const BW::string &s );

	void setStaticFrame( unsigned int i );
	void setFrameRange( unsigned int first, unsigned int last );

	bool includeMeshes( void ) const;
	bool includeEnvelopesAndBones( void ) const;
	bool includeNodes( void ) const;
	bool includeMaterials( void ) const;
	bool includeAnimations( void ) const;
	bool useCharacterMode( void ) const;
	bool includePortals( void ) const;
	bool worldSpaceOrigin( void ) const;
	bool resolvePaths( void ) const;
	float unitScale( ) const;
	bool localHierarchy( ) const;

	const BW::string &animationName( void ) const;

	// Query and set the referenceNodes file. The filename string is an
	// absolute path.  Serialisation of this value will be a resource
	// relative path.
	const BW::string &referenceNodesFile( void ) const;
	void              referenceNodesFile( const BW::string& refNodesFile );

	unsigned int staticFrame( void ) const;


	enum ExportMode
	{
		NORMAL = 0,
		STATIC,
		STATIC_WITH_NODES,
		MESH_PARTICLES,
		EXPORTMODE_END
	};

	bool	exportAnim() const { return exportAnim_; };
	void		exportAnim( bool exportAnim ) { exportAnim_ = exportAnim; };

	ExportMode	exportMode( ) const { return exportMode_; };
	void		exportMode( ExportMode exportMode ) { exportMode_ = exportMode; };

	enum NodeFilter
	{
		ALL = 0,
		SELECTED,
		VISIBLE,
		NODEFILTER_END
	};

	NodeFilter	nodeFilter( ) const { return nodeFilter_; };
	void		nodeFilter( NodeFilter nodeFilter ) { nodeFilter_ = nodeFilter; };

	bool		transformToOrigin( ) const { return transformToOrigin_; };
	void		transformToOrigin( bool state ) { transformToOrigin_ = state; };

	bool		allowScale( ) const { return allowScale_; };
	void		allowScale( bool state ) { allowScale_ = state; };

	bool		bumpMapped( ) const { return bumpMapped_; };
	void		bumpMapped( bool state ) { bumpMapped_ = state; };

	bool		fixCylindrical( ) const { return fixCylindrical_; };
	void		fixCylindrical( bool state ) { fixCylindrical_ = state; };

	bool		keepExistingMaterials( ) const { return keepExistingMaterials_; };
	void		keepExistingMaterials( bool state ) { keepExistingMaterials_ = state; };

	bool		snapVertices( ) const { return snapVertices_; };
	void		snapVertices( bool state ) { snapVertices_ = state; };

	bool		stripRefPrefix( ) const { return stripRefPrefix_; };
	void		stripRefPrefix( bool state ) { stripRefPrefix_ = state; };

	bool		referenceNode( ) const { return referenceNode_; };
	void		referenceNode( bool state ) { referenceNode_ = state; };

	bool		disableVisualChecker( ) const { return disableVisualChecker_; };
	void		disableVisualChecker( bool state ) { disableVisualChecker_ = state; };

	bool		useLegacyScaling( ) const { return useLegacyScaling_; };
	void		useLegacyScaling( bool state ) { useLegacyScaling_ = state; };

	bool		useLegacyOrientation( ) const { return useLegacyOrientation_; };
	void		useLegacyOrientation( bool state ) { useLegacyOrientation_ = state; };

	bool		sceneRootAdded( ) const { return sceneRootAdded_; };
	void		sceneRootAdded( bool state ) { sceneRootAdded_ = state; };

	void		visualTypeIdentifier( const BW::string& s ) { visualTypeIdentifier_ = s; }
	const BW::string& visualTypeIdentifier() const { return visualTypeIdentifier_; }
	
	int maxBones( ) const { return maxBones_; };
	void		maxBones( int value ) { maxBones_ = value; };

	bool		copyExternalTextures( ) const { return copyExternalTextures_; }
	void		copyExternalTextures( bool state ) { copyExternalTextures_ = state; }

	const BW::string& copyTexturesTo( ) const { return copyTexturesTo_; }
	void		copyTexturesTo( const BW::string& s ) { copyTexturesTo_ = s; }

	static ExportSettings& ExportSettings::instance();

private:
	ExportSettings();
	//~ static BOOL CALLBACK dialogProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	bool			includeMeshes_;
	bool			includeEnvelopesAndBones_;
	bool			includeNodes_;
	bool			includeMaterials_;
	bool			includeAnimations_;
	bool			useCharacterMode_;
	bool			includePortals_;
	bool			worldSpaceOrigin_;
	bool			resolvePaths_;
	float			unitScale_;
	bool			localHierarchy_;
	bool			transformToOrigin_;
	bool			allowScale_;
	bool			bumpMapped_;
	bool			fixCylindrical_;	
	bool			keepExistingMaterials_;
	bool			snapVertices_;
	bool			stripRefPrefix_;
	bool			referenceNode_;
	bool			disableVisualChecker_;
	bool			useLegacyScaling_;
	bool			useLegacyOrientation_;
	bool			sceneRootAdded_;
	bool			copyExternalTextures_;
	BW::string		copyTexturesTo_;

	int				maxBones_;

	bool			exportAnim_;
	ExportMode		exportMode_;
	NodeFilter		nodeFilter_;

	BW::string		animationName_;

	unsigned int	staticFrame_; //the static frame output
	unsigned int	frameFirst_;
	unsigned int	frameLast_;

	BW::string		referenceNodesFileAbs_;

	BW::string		visualTypeIdentifier_;

	ExportSettings(const ExportSettings&);
	ExportSettings& operator=(const ExportSettings&);

	BW::string getReferenceFile( HWND hWnd );
	BW::string settingFilename_;
	friend std::ostream& operator<<(std::ostream&, const ExportSettings&);
};

BW_END_NAMESPACE

#if defined( CODE_INLINE )
#include "expsets.ipp"
#endif

#endif // EXPSETS_HPP
