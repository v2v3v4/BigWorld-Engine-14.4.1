#ifndef __PACKER_HELPER_HPP__
#define __PACKER_HELPER_HPP__

#include "cstdmf/bw_namespace.hpp"
#include "cstdmf/bw_string.hpp"

#include <stdio.h>

#include <string>


BW_BEGIN_NAMESPACE

class PackerHelper
{
public:
	// retrieve the command line parameters passed to res_packer
	static int argc() { return s_argc_; }
	static char** argv() { return s_argv_; }

	static void paths( const BW::string& inPath, const BW::string& outPath );

	static const BW::string& inPath();

	static const BW::string& outPath();

	// Inits BWResource and AutoConfig strings to the paths of the "file" parameters
	static bool initResources();

	// Inits Moo, creating a D3D device, etc.
	static bool initMoo();

	// Helper method to copy a file (platform-independent)
	static bool copyFile( const BW::string & src, const BW::string & dst,
		bool silent = false );

	// Helper method to ask if a file exists (platform-independent)
	static bool fileExists( const BW::string& file );

	// Helper method to check if a file1 is newer than a file2 (platform-independent)
	static bool isFileNewer( const BW::string & file1,
		const BW::string & file2 );

	// Helper class to delete files on destruction. Ideal for temp files
	class FileDeleter
	{
	public:
		FileDeleter( const BW::string& file ) : file_( file ) {};
		~FileDeleter() { remove( file_.c_str() ); }
	private:
		BW::string file_;
	};

private:
	static int s_argc_;
	static char** s_argv_;
	static BW::string s_basePath_;
	static BW::string s_inPath;
	static BW::string s_outPath;

#if !defined( MF_SERVER )
	// AutoConfig and method to get the cursor textures, so we can set them to the
	// appropriate format. It has to be done this way to avoid the overkill of
	// having to modify the MouseCursor class and including ashes, pyscript,
	// python and probably some other libs, all for this simple task.
	// Be aware that if the class MouseCursor in src/lib/ashes changes, this
	// AutoConfig and method need to be checked and changed if necesary to reflect
	// any changes in the tag names and/or the required texture format.
	static void initTextureFormats();
#endif

public:
	// Thes methods are not meant to be used by the packers!
	static void setCmdLine( int argc, char** argv )
	{
		s_argc_ = argc;
		s_argv_ = argv;
	}

	static void setBasePath( const BW::string & basePath )
	{ 
		s_basePath_ = basePath;
		if (s_basePath_.length() != 0 && *s_basePath_.rbegin() != '/')
		{
			s_basePath_.append( "/" );
		}
	}
};

BW_END_NAMESPACE

#endif // __PACKER_HELPER_HPP__
