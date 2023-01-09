// bwresource.ipp

#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif

BW_BEGIN_NAMESPACE

#ifdef EDITOR_ENABLED
// wide versions, editor-only
//-----------------------------------------------------------------------------
INLINE const BW::wstring BWResource::getFilenameW( const BW::string& file )
{
	return bw_utf8tow( getFilename( file ).to_string() );
}


//-----------------------------------------------------------------------------
INLINE const BW::wstring BWResource::getFilenameW( const BW::wstring& file )
{
	return bw_utf8tow( getFilename( bw_wtoutf8( file ) ).to_string() );
}


//-----------------------------------------------------------------------------
INLINE const BW::wstring BWResource::getExtensionW( const BW::string& file )
{
	return bw_utf8tow( getExtension( file ).to_string() );
}

//-----------------------------------------------------------------------------

INLINE const BW::wstring BWResource::getExtensionW( const BW::wstring& file )
{
	return bw_utf8tow( getExtension( bw_wtoutf8( file ) ).to_string() );
}


//-----------------------------------------------------------------------------
INLINE const BW::wstring BWResource::removeExtensionW( const BW::string& file )
{
	return bw_utf8tow( removeExtension( file ).to_string() );
}


//-----------------------------------------------------------------------------
INLINE const BW::wstring BWResource::removeExtensionW( const BW::wstring& file )
{
	return bw_utf8tow( removeExtension( bw_wtoutf8( file ) ).to_string() );
}


//-----------------------------------------------------------------------------
INLINE bool BWResource::validPathW( const BW::wstring& file )
{
	return validPath( bw_wtoutf8( file ) );
}


//-----------------------------------------------------------------------------
INLINE void BWResource::defaultDriveW( const BW::wstring& drive )
{
	return defaultDrive( bw_wtoutf8( drive ) );
}


//-----------------------------------------------------------------------------
INLINE const BW::wstring BWResource::findFileW( const BW::string& file )
{
	return bw_utf8tow( findFile( file ) );
}


//-----------------------------------------------------------------------------
INLINE const BW::wstring BWResource::removeDriveW( const BW::string& file )
{
	return bw_utf8tow( removeDrive( file ) );
}


//-----------------------------------------------------------------------------
INLINE const BW::wstring BWResource::dissolveFilenameW( const BW::string& file )
{
	return bw_utf8tow( dissolveFilename( file ) );
}


//-----------------------------------------------------------------------------
INLINE const BW::wstring BWResource::resolveFilenameW( const BW::string& file )
{
	return bw_utf8tow( resolveFilename( file ) );
}


//-----------------------------------------------------------------------------
INLINE const BW::wstring BWResource::findFileW( const BW::wstring& file )
{
	return bw_utf8tow( findFile( bw_wtoutf8( file ) ) );
}


//-----------------------------------------------------------------------------
INLINE const BW::wstring BWResource::removeDriveW( const BW::wstring& file )
{
	return bw_utf8tow( removeDrive( bw_wtoutf8( file ) ) );
}


//-----------------------------------------------------------------------------
INLINE const BW::wstring BWResource::dissolveFilenameW( const BW::wstring& file )
{
	return bw_utf8tow( dissolveFilename( bw_wtoutf8( file ) ) );
}


//-----------------------------------------------------------------------------
INLINE const BW::wstring BWResource::resolveFilenameW( const BW::wstring& file )
{
	return bw_utf8tow( resolveFilename( bw_wtoutf8( file ) ) );
}

#endif // EDITOR_ENABLED

BW_END_NAMESPACE

