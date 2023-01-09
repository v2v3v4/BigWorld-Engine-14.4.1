#include "config.hpp"
#include "packers.hpp"
#include "packer_helper.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/file_system.hpp"
#include "resmgr/multi_file_system.hpp"
#include "image_packer.hpp"
#ifndef MF_SERVER
#include "moo/texture_manager.hpp"
#include "moo/texture_compressor.hpp"
#endif // MF_SERVER

#include "xml_packer.hpp"


BW_BEGIN_NAMESPACE

IMPLEMENT_PACKER( ImagePacker )

int ImagePacker_token = 0;


bool ImagePacker::prepare( const BW::string& src, const BW::string& dst )
{
	// TODO: This is somewhat of a temporary workaround so that we can have
	// "encrypted" dds files for the 1.8.0 evaluation release. If running with
	// the --encrypt command line, we want dds files to be processed by
	// XmlPacker.
	if (XmlPacker::shouldEncrypt())
		return false;

	BW::StringRef ext = BWResource::getExtension( src );
	// TODO: compare_lower
	if (ext.equals_lower( "bmp" ) ||
		ext.equals_lower( "tga" ) ||
		ext.equals_lower( "png" ) ||
		ext.equals_lower( "jpg" ))
		type_ = IMAGE;
	else if (ext.equals_lower( "dds" ))
		type_ = DDS;
	else
		return false;

	src_ = src;
	dst_ = dst;

	return true;
}

bool ImagePacker::print()
{
	if ( src_.empty() )
	{
		printf( "Error: ImagePacker Not initialised properly\n" );
		return false;
	}

	if ( type_ == DDS )
		printf( "DDSFile: %s\n", src_.c_str() );
	else
		printf( "ImageFile: %s\n", src_.c_str() );
	return true;
}

bool ImagePacker::pack()
{
#ifndef MF_SERVER
	if ( src_.empty() || dst_.empty() )
	{
		printf( "Error: ImagePacker Not initialised properly\n" );
		return false;
	}

	if ( type_ == DDS )
	{
		// if it's a DDS image file, find out if it has a matching source image
		BW::StringRef baseName = BWResource::removeExtension( src_ );
		bool isFont = false;

#ifndef PACK_FONT_DDS
		// Make sure that it is not a font DDS
		BW::string directory = 
			BWResource::getFilePath( BWResolver::dissolveFilename( baseName ) );
		BW::StringRef fontName = BWResource::getFilename( baseName );

		MultiFileSystemPtr mfs = BWResource::instance().fileSystem();
		IFileSystem::Directory pFolder;
		mfs->readDirectory( pFolder, directory );
			
		if (pFolder.size())
		{
			IFileSystem::Directory::iterator it = pFolder.begin();
			IFileSystem::Directory::iterator end = pFolder.end();

			for (; !isFont && it != end; ++it)
			{
				BW::string fileName = directory + (*it);
				if (BWResource::getExtension( fileName ) == "font")
				{
					DataSectionPtr fontFile = 
						BWResource::openSection( fileName );
					if (fontFile)
					{
						BW::string sourceFont = 
							fontFile->readString( "creation/sourceFont", "" );
						int fontSize = abs( fontFile->readInt( 
											"creation/sourceFontSize", 0 ) );
	
						char buffer[256];
						bw_snprintf( buffer, sizeof(buffer), "%s_%d", 
												sourceFont.c_str(), fontSize );
	
						if (fontName == buffer)
						{
							isFont = true;
						}
					}
				}
			}
		}
#endif // PACK_FONT_DDS

		// if the DDS is not a font, then copy it
		if (!isFont)
		{
			if (!PackerHelper::fileExists( dst_ ))
			{
				if (!PackerHelper::copyFile( src_, dst_ ))
				{
					return false;
				}
			}
		}

		// the DDS was copied, or we want to skip it, so return true
		return true;
	}
	else
	{
		BW::string dissolved = BWResolver::dissolveFilename( src_ );
		BW::string ddsName = BWResource::removeExtension( dissolved ) + ".dds";
		ddsName = BWResolver::resolveFilename( ddsName );
		if (PackerHelper::fileExists( ddsName ))
		{
			// A converted DDS exists so lets copy that instead
			src_ = ddsName;
			dst_ = BWResource::removeExtension( dst_ ) + ".dds";
			if (!PackerHelper::fileExists( dst_ ))
			{
				if (!PackerHelper::copyFile( src_, dst_ ))
				{
					return false;
				}
			}
		}
		else if (BWResource::getExtension( src_ ) == 
			BWResource::getExtension( dst_ ))
		{
			if (!PackerHelper::fileExists( dst_ ))
			{
				if (!PackerHelper::copyFile( src_, dst_ ))
				{
					return false;
				}
			}
		}
		else if (!PackerHelper::fileExists( dst_ ))
		{
			printf( "Error: Cannot copy source %s to destination %s.\n", 
				src_.c_str(), dst_.c_str() );
			return false;
		}

		// the source image was copied, or we want to skip it, so return true
		return true;
	}
#else // MF_SERVER

	// just skip image files for the server

#endif // MF_SERVER

	return true;
}

BW_END_NAMESPACE
