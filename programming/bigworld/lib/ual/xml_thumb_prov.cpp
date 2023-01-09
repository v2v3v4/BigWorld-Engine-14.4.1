/**
 *	XML File Thumbnail Provider (particles, lights, etc)
 */


#include "pch.hpp"
#include "cstdmf/bw_string.hpp"
#include "xml_thumb_prov.hpp"
#include "ual_manager.hpp"
#include "chunk/chunk_light.hpp"
#include "particle/meta_particle_system.hpp"
#include "particle/particle_system.hpp"
#include "resmgr/datasection.hpp"
#include "resmgr/bwresource.hpp"
#include "moo/render_context.hpp"
#include "common/string_utils.hpp"

BW_BEGIN_NAMESPACE

int XmlThumbProv_token;


// Implement the XML File Provider factory
IMPLEMENT_THUMBNAIL_PROVIDER( XmlThumbProv )


/**
 *	This method isused to find out if the xml file is a particle system.
 *
 *	@param file		Asset file requesting the thumbnail.
 *	@return		True if the XML file is a particle system.
 */
bool XmlThumbProv::isParticleSystem( const BW::wstring& file )
{
	BW_GUARD;

	BW::string nfile;
	bw_wtoutf8( file, nfile );
	return MetaParticleSystem::isParticleSystem( nfile );
}


/**
 *	This method is used to find out if the xml file is a light.
 *
 *	@param file		Asset file requesting the thumbnail.
 *	@return		True if the XML file is a light template file.
 */
bool XmlThumbProv::isLight( const BW::wstring& file )
{
	BW_GUARD;

	BW::string nfile;
	bw_wtoutf8( file, nfile );
	DataSectionPtr ds = BWResource::openSection( nfile );
	if ( !ds )
		return false; // file is not a datasection or doesn't exist

	// hardcoding light's section names because of a lack of better way at 
	// the moment:
	if ( ds->openSection( "ambientLight" ) == NULL &&
		 ds->openSection( "directionalLight" ) == NULL &&
		 ds->openSection( "omniLight" ) == NULL &&
		 ds->openSection( "spotLight" ) == NULL &&
		 ds->openSection( "pulseLight" ) == NULL &&
		 ds->openSection( "flare" ) == NULL )
	{
		 return false;
	}

	return true;
}


/**
 *	This method simply returns the name of the image file used as a thumbnail
 *	for particle systems.
 *
 *	@return		Filename of the image used as thumbnail for particle systems.
 */
BW::wstring XmlThumbProv::particleImageFile()
{
	BW_GUARD;

	BW::string nconfigFile;
	BW::wstring wfileName;

	bw_wtoutf8( UalManager::instance().getConfigFile(), nconfigFile );
	bw_utf8tow( BWResource::getFilePath( nconfigFile ), wfileName );
	return wfileName + L"icon_particles.bmp";
}


/**
 *	This method simply returns the name of the image file used as a thumbnail
 *	for lights.
 *
 *	@return		Filename of the image used as thumbnail for lights.
 */
BW::wstring XmlThumbProv::lightImageFile()
{
	BW_GUARD;

	BW::string nconfigFile;
	BW::wstring wfileName;

	bw_wtoutf8( UalManager::instance().getConfigFile(), nconfigFile );
	bw_utf8tow( BWResource::getFilePath( nconfigFile ), wfileName );
	return wfileName + L"icon_light.bmp";
}


/**
 *	This method tells the manager if the asset specified in 'file' can be
 *	handled by the XmlThumbProv.
 *
 *	@param manager	ThumbnailManager instance that is dealing with the asset.
 *	@param file		Asset requesting the thumbnail
 *	@return			True if the asset is an xml file, false otherwise.
 */
bool XmlThumbProv::isValid( const ThumbnailManager& manager, const BW::wstring& file )
{
	BW_GUARD;

	if ( file.empty() )
		return false;

	BW::wstring::size_type dot = file.find_last_of( L'.' );
	BW::wstring ext = file.substr( dot + 1 );

	return wcsicmp( ext.c_str(), L"xml" ) == 0;
}


/**
 *	This method is called to find out if the image for the asset needs to be
 *	created in the background thread, which is never the case for the currenty
 *	handled XML file types.
 *
 *	@param manager	ThumbnailManager instance that is dealing with the asset.
 *	@param file		Asset requesting the thumbnail.
 *	@param thumb	Returns the path to the thumbnail img file for this asset.
 *	@param size		Ignored.
 *	@return			False, we don't need to do anything in the bg thread.
 */
bool XmlThumbProv::needsCreate( const ThumbnailManager& manager, const BW::wstring& file, BW::wstring& thumb, int& size )
{
	BW_GUARD;

	if ( file.empty() || thumb.empty() )
		return false; // invalid input params, return false

	// try to load the xml file as each of the known formats
	// if known, set the thumb filename to the icon's filename
	if ( isParticleSystem( file ) )
	{
		// it's a particle system
		thumb = particleImageFile();
	}
	else if ( isLight( file ) )
	{
		// it's a light system
		thumb = lightImageFile();
	}

	// return false to load the thumb directly
	return false;
}


/**
 *	This class never needs to prepare the thumbnail in the bg thread.
 *
 *	@param manager	Ignored.
 *	@param file		Ignored.
 *	@return			False, we don't need to do anything in the bg thread.
 */
bool XmlThumbProv::prepare( const ThumbnailManager& manager, const BW::wstring& file )
{
	// should never get called
	return false;
}


/**
 *	This class never needs to render the thumbnail.
 *
 *	@param manager	Ignored.
 *	@param file		Ignored.
 *	@param rt		Ignored.
 *	@return			False, we don't need to do anything in the bg thread.
 */
bool XmlThumbProv::render( const ThumbnailManager& manager, const BW::wstring& file, Moo::RenderTarget* rt )
{
	// should never get called
	return false;
}

BW_END_NAMESPACE

