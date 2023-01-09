#ifndef STREAMED_DATA_CACHE_HPP
#define STREAMED_DATA_CACHE_HPP

#include "cstdmf/concurrency.hpp"

#define SEPARATE_READ_THREAD	1

BW_BEGIN_NAMESPACE

namespace Moo
{

/**
 *	This class is a whole bunch of data blocks
 */
class StreamedDataCache
{
public:
	static const int CACHE_VERSION;

public:
	StreamedDataCache( const BW::string & resourceID, bool createIfAbsent );
	~StreamedDataCache();

	/**
	 * This is the information for each entry in the .anca file
	 * Each entry describes the data for one animation in the file
	 */
	struct EntryInfo
	{
		// The amount of data to preload from the .anca file
		uint32	preloadSize_;
		// The size of the data to be streamed from the .anca file
		uint32	streamSize_;
		// The offset of the start of this data in the .anca file
		uint32	fileOffset_;
		// The version of the data in the file
		uint32	version_;
		// The modification time of the source animation
		uint64	modifiedTime_;
	};

	/**
	 * This class holds the state of a load request.
	 */
	class Tracker : public SafeReferenceCount
	{
	public:
		Tracker( int offset ) :
			offset_( offset ),
			finished_( 0 )
		{};
		Tracker( int offset, int finished ) :
			offset_( offset ),
			finished_( finished )
		{};
		int offset_;
		int finished_;// 0 = not read, 1 = read ok, -1 = read error
	};

	typedef SmartPointer< Tracker > TrackerPtr;

	bool fileAccessDone( TrackerPtr tracker );

	const EntryInfo * findEntryData( const BW::string & name,
		uint32 minVersion, uint64 minModified );
	uint32 addEntryData( const BW::string & name,
		uint32 version, uint64 modified );
	void endEntryData( uint32 psize, uint32 ssize );

	bool preload( uint32 offset, uint32 size, void * data );
	bool immsave( uint32 offset, uint32 size, const void * data );

	// anim data looks like:
	// uint32   preload data size (including streamed data sizes)
	// ......   preload data (standard animation)
	// uint32*	streamed data size for each block in the anim
	// ------	(end of preload data)
	// ......	streamed data

	TrackerPtr load( uint32 offset, BinaryPtr pData );
	TrackerPtr save( uint32 offset, uint32 size, const void * data );

	bool good();
	uint numEntries()				{ return (uint)(directory_.size()); }
	void deleteOnClose( bool doc )	{ deleteOnClose_ = doc; }

	void loadSelf();
	void saveSelf();

private:
	bool validateEntryData( uint32 headerStart );

	BW::string		fileName_;
	FILE*			file_;

	typedef BW::map< BW::string, EntryInfo > EntryInfos;
	EntryInfos		directory_;

	EntryInfo *		lastEntry_;
	bool			deleteOnClose_;

};


}	// namespace Moo

BW_END_NAMESPACE

#endif // STREAMED_DATA_CACHE_HPP
