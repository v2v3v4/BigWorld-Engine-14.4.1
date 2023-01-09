#ifndef MFXFILE_HPP
#define MFXFILE_HPP

#include "chunkids.hpp"

#include <iostream>

BW_BEGIN_NAMESPACE

class MFXFile
{
public:
	MFXFile();
	~MFXFile();

	bool openForOutput( const BW::string &filename );
	bool openForInput( const BW::string & filename );
	bool close( void );

	void writeChunkHeader( ChunkID id, int totalSize, int size );
	void writePoint( const Point3 &p );
	void writeColour( const Color &c );
	void writeUV( const Point3 &uv );
	void writeMatrix( const Matrix3 &m );
	void writeInt( int i );
	void writeFloat( float f );
	void writeString( const BW::string &s );
	void writeTriangle( unsigned int a, unsigned int b, unsigned int c );
	static unsigned int getStringSize( const BW::string &s );

	bool readChunkHeader( ChunkID &id, int &totalSize, int &size );
	bool readInt( int &i );
	bool readString( BW::string &s );
	void nextChunk();

private:

	FILE	*stream_;

	int		nextChunkStart_;

	MFXFile(const MFXFile&);
	MFXFile& operator=(const MFXFile&);

	friend std::ostream& operator<<(std::ostream&, const MFXFile&);
};

BW_END_NAMESPACE

#ifdef CODE_INLINE
#include "mfxfile.ipp"
#endif

#endif
/*mfx.hpp*/
