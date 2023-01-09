#ifndef STREAMED_ANIMATION_CHANNEL_HPP
#define STREAMED_ANIMATION_CHANNEL_HPP

#include "animation_channel.hpp"
#include "streamed_data_cache.hpp"


BW_BEGIN_NAMESPACE

namespace Moo
{

class InterpolatedAnimationChannel;

// This is how many keys are in a block.
// Currently the upper limit would be 25.
#define STREAM_BLOCK_SIZE 16

class StreamedAnimation;
class StreamedAnimationChannel;

typedef SmartPointer< StreamedAnimation >			StreamedAnimationPtr;
typedef SmartPointer< StreamedAnimationChannel >	StreamedAnimationChannelPtr;

/**
 *	This class has all the information for streaming in an animation.
 *	Currently it is owned by a normal Moo animation.
 *	It might be better to just merge it with that class.
 */
class StreamedAnimation : public ReferenceCount
{
public:
	StreamedAnimation();
	~StreamedAnimation();

	void prep( StreamedAnimationChannel * pChannel );
	void add( StreamedAnimationChannel * pChannel );

	void load( BinaryFile & bf, float totalTime,
		StreamedDataCache * pSource, uint32 streamedOffset );
	void save( BinaryFile & bf );

	uint32 dam( uint32 streamedOffset );

	void touch( float time );

private:

	// a block looks like:
	// uint8*		channel data size for each channel in the animation
	// uint8[0-3]	byte padding to a uint32
	// ......		channel data

	struct StreamedBlock;

	struct CacheRecord
	{
		CacheRecord( StreamedAnimation * owner, StreamedBlock * block );
		~CacheRecord();

		BinaryPtr			pData_;
		StreamedAnimation	* owner_;
		CacheRecord			* next_;
		CacheRecord			* prev_;

		CacheRecord( const CacheRecord & other );
		CacheRecord & operator=( const CacheRecord & other );

		void touch();
		void link();
		void unlink();
	};

	struct StreamedBlock
	{
		uint32							offset_;
		uint32							size_;
		CacheRecord *					data_;
		StreamedDataCache::TrackerPtr	reader_;
	};

	friend struct CacheRecord;

	void flick( uint32 block );

	void bind( uint32 block );
	void loose( uint32 block );

	StreamedDataCache	* pSource_;

	typedef BW::vector< StreamedAnimationChannelPtr > Channels;
	Channels			channels_;

	StreamedBlock *		blocks_;
	uint32				numBlocks_;

	static CacheRecord *	s_cacheHead_;
	static CacheRecord *	s_cacheTail_;
	static size_t			s_cacheSize_;

public:
	static size_t			s_cacheMaxSize_;

	uint32 numBlocks()					{ return numBlocks_; }

	static THREADLOCAL( StreamedDataCache* ) s_loadingAnimCache_;
	static THREADLOCAL( StreamedDataCache* ) s_savingAnimCache_;

	static THREADLOCAL( StreamedAnimation* ) s_currentAnim_;
};

/**
 *	This struct is one block of data for a channel in-place.
 */
struct ChannelBlock
{
	/**
	 * This is the header structure for each data type
	 * it is used for scale, position and rotation data
	 */
	struct Header
	{
		uint8	durBefore_;
		uint8	durAfter_;
		uint8	rosterOffset_;
		uint8	keyDataOffset_;
	};
	Header	sHeader_;
	Header	pHeader_;
	Header	rHeader_;

	const uint8* scaleRoster() const
	{ 
		return ((uint8*)(this+1)) + (sHeader_.rosterOffset_ & 0x7F); 
	}

	const uint8* positionRoster() const
	{ 
		return ((uint8*)(this+1)) + (pHeader_.rosterOffset_ & 0x7F); 
	}

	const uint8* rotationRoster() const
	{ return ((uint8*)(this+1)) + (rHeader_.rosterOffset_ & 0x7F); }

	const Vector3* scaleKeyData() const
	{ 
		return (const Vector3*)(((uint8*)(this+1)) + 
			sHeader_.keyDataOffset_ * sizeof(uint32) );
	}

	const Vector3 * positionKeyData() const
	{ 
		return (const Vector3*)(((uint8*)(this+1)) + 
			pHeader_.keyDataOffset_ * sizeof(uint32) ); 
	}

	const Quaternion * rotationKeyData() const
	{ 

		return (const Quaternion*)(((uint8*)(this+1)) + 
			rHeader_.keyDataOffset_ * sizeof(uint32) ); 
	}

	uint8 calcNumWords() const;
};

/**
 *	This class is an animation channel that is streamed in from disk.
 */
class StreamedAnimationChannel : public AnimationChannel
{
public:
	StreamedAnimationChannel();
	~StreamedAnimationChannel();

	StreamedAnimationChannel( const StreamedAnimationChannel & other );
	StreamedAnimationChannel& operator=( const StreamedAnimationChannel & );

	virtual	Matrix		result( float time ) const;
	virtual void		result( float time, Matrix& out ) const;
	virtual void		result( float time, BlendTransform& out ) const;

	virtual bool		load( BinaryFile & bf );
	virtual bool		save( BinaryFile & bf ) const;

	virtual void		preCombine ( const AnimationChannel & rOther );
	virtual void		postCombine( const AnimationChannel & rOther );

	virtual int			type() const
		{ return 5; }

	virtual AnimationChannel * duplicate() const
		{ return new StreamedAnimationChannel( *this ); }

	void mirror( const InterpolatedAnimationChannel & iac );

private:
	ChannelBlock		**pBlocks_;
	uint32				numBlocks_;

	Vector3				scaleFallback_;
	Vector3				positionFallback_;
	Quaternion			rotationFallback_;

	friend class StreamedAnimation;

	static AnimationChannel * New()
		{ return new StreamedAnimationChannel(); }
	static TypeRegisterer s_rego_;
};



}	// namespace Moo

BW_END_NAMESPACE

#endif // STREAMED_ANIMATION_CHANNEL_HPP
