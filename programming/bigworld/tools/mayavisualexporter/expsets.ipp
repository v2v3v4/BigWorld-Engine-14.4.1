#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif

BW_BEGIN_NAMESPACE

INLINE
void ExportSettings::setExportMeshes( bool b )
{
	includeMeshes_ = b;
}

INLINE
void ExportSettings::setExportEnvelopesAndBones( bool b )
{
	includeEnvelopesAndBones_ = b;
}

INLINE
void ExportSettings::setExportNodes( bool b )
{
	includeNodes_ = b;
}

INLINE
void ExportSettings::setExportMaterials( bool b )
{
	includeMaterials_ = b;
}

INLINE
void ExportSettings::setExportAnimations( bool b )
{
	includeAnimations_ = b;
}

INLINE
void ExportSettings::setUseCharacterMode( bool b )
{
	useCharacterMode_ = b;
}

INLINE
void ExportSettings::setAnimationName( const BW::string &s )
{
	animationName_ = s;
}

INLINE
void ExportSettings::setStaticFrame( unsigned int i )
{
	staticFrame_ = i;
}

INLINE
void ExportSettings::setFrameRange( unsigned int first, unsigned int last )
{
	frameFirst_ = first;
	frameLast_ = last;
}

INLINE
bool ExportSettings::includeMeshes( void ) const
{
	return includeMeshes_;
}

INLINE
bool ExportSettings::includeEnvelopesAndBones( void ) const
{
	return includeEnvelopesAndBones_;
}

INLINE
bool ExportSettings::includeNodes( void ) const
{
	return includeNodes_;
}

INLINE
bool ExportSettings::includeMaterials( void ) const
{
	return includeMaterials_;
}

INLINE
bool ExportSettings::includeAnimations( void ) const
{
	return includeAnimations_;
}

INLINE
bool ExportSettings::useCharacterMode( void ) const
{
	return useCharacterMode_;
}

INLINE
const BW::string &ExportSettings::animationName( void ) const
{
	return animationName_;
}

INLINE
unsigned int ExportSettings::staticFrame( void ) const
{
	return staticFrame_;
}

INLINE
void ExportSettings::setIncludePortals( bool b )
{
	includePortals_ = b;
}

INLINE
void ExportSettings::setWorldSpaceOrigin( bool b )
{
	worldSpaceOrigin_ = b;
}

INLINE
bool ExportSettings::includePortals( void ) const
{
	return includePortals_;
}

INLINE
bool ExportSettings::worldSpaceOrigin( void ) const
{
	return worldSpaceOrigin_;
}

INLINE
void ExportSettings::setResolvePaths( bool b )
{
	resolvePaths_ = b;
}

INLINE
bool ExportSettings::resolvePaths( void ) const
{
	return resolvePaths_;
}

INLINE
void ExportSettings::setUnitScale( float f)
{
	unitScale_ = f;
}

INLINE
float ExportSettings::unitScale( ) const
{
	if ( this->useLegacyScaling() )
		return unitScale_;
		
	static float scale = 0.f;
	if( scale == 0.f )
	{
		MDistance dist;
		dist.setValue( 1 );
		scale = (float)dist.asMeters();
	}
	return scale;
}

INLINE
const BW::string &ExportSettings::referenceNodesFile( void ) const
{
	return referenceNodesFileAbs_;
}

INLINE
void ExportSettings::referenceNodesFile( const BW::string &refNodesFile )
{
	referenceNodesFileAbs_ = refNodesFile;
}

INLINE
bool ExportSettings::localHierarchy( ) const
{
	return localHierarchy_;
}

INLINE
void ExportSettings::setLocalHierarchy( bool b )
{
	localHierarchy_ = b;
}

BW_END_NAMESPACE

/*expsets.ipp*/