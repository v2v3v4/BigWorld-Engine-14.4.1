
#include "mfxexp.hpp"
#include "mfxfile.hpp"

#ifndef CODE_INLINE
#include "mfxfile.ipp"
#endif

BW_BEGIN_NAMESPACE

MFXFile::MFXFile()
: stream_( NULL )
{
}

MFXFile::~MFXFile()
{
}

bool MFXFile::openForOutput( const BW::string &filename )
{
	BW::string loweredFilename = filename;
	int len = static_cast<int>(loweredFilename.length());
	if (len>4)
	{
		for (int i=len-3;i<len;i++)
		{
			loweredFilename[i] = tolower(loweredFilename[i]);
		}
	}

	stream_ = fopen( loweredFilename.c_str(), "wb" );
	if( stream_ )
		return true;
	return false;
}

bool MFXFile::openForInput( const BW::string & filename )
{
	stream_ = fopen( filename.c_str(), "rb" );
	return stream_ != NULL;
}

bool MFXFile::close( void )
{
	if( stream_ )
	{
		if( 0 == fclose( stream_ ) )
			return true;
	}

	return false;
}


std::ostream& operator<<(std::ostream& o, const MFXFile& t)
{
	o << "MFXFile\n";
	return o;
}


BW_END_NAMESPACE

/*mfx.cpp*/
