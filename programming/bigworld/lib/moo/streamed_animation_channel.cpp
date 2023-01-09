
#include "pch.hpp"
#include "streamed_animation_channel.hpp"
#include "cstdmf/binaryfile.hpp"
#include "cstdmf/slot_tracker.hpp"
#include "interpolated_animation_channel.hpp"

DECLARE_DEBUG_COMPONENT2( "Moo", 0 )


BW_BEGIN_NAMESPACE

/** 
 *	The SEPARATE_READ_THREAD token specifies whether streamed animations
 *	should use a separate thread for ReadFile operations.
 *
 *	If this token is not defined, ReadFile will block the main thread, causing
 *  occasional frame-rate glitches.
 */

MEMTRACKER_DECLARE(StreamedAnimation_CacheRecord, "StreamedAnimation_CacheRecord", 0 );
MEMTRACKER_DECLARE(StreamedAnimation_Add, "StreamedAnimation_Add", 0 );

namespace Moo
{

// -----------------------------------------------------------------------------
// Section: StreamedAnimation
// -----------------------------------------------------------------------------

static uint32 s_CRCons = 0;
static uint32 s_CRDest = 0;
static uint32 s_CRLive = uint32(-1);

/**
 *	Constructor
 */
StreamedAnimation::StreamedAnimation() :
	blocks_( NULL )
{
	BW_GUARD;
	if (s_CRLive == uint32(-1))
	{
		s_CRLive = 0;
		MF_WATCH(	"Memory/StreamedBlocks_Cons", s_CRCons,
					Watcher::WT_READ_ONLY, 
					"Number of streamed animation cache records constructed" );
		MF_WATCH(	"Memory/StreamedBlocks_Dest", s_CRDest,
					Watcher::WT_READ_ONLY, 
					"Number of streamed animation cache records destructed" );
		MF_WATCH(	"Memory/StreamedBlocks_Live", s_CRLive,	
					Watcher::WT_READ_ONLY, 
					"Number of streamed animation cache records currently "
					"in memory" );
		MF_WATCH(	"Memory/StreamedBlocks_CacheSize", 
					StreamedAnimation::s_cacheSize_,
					Watcher::WT_READ_ONLY, 
					"The number of bytes used for streamed blocks" );
		MF_WATCH(	"Memory/StreamedBlocks_CacheMaxSize", 
					StreamedAnimation::s_cacheMaxSize_,
					Watcher::WT_READ_WRITE, 
					"The maximum number bytes that will be used for streamed "
					"animation blocks" );
	}

	RESOURCE_COUNTER_ADD(	ResourceCounters::DescriptionPool(
								"Animation/Streamed Animations",
								(uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER),
								( sizeof(*this) ), 0 );
}

/**
 *	Destructor
 */
StreamedAnimation::~StreamedAnimation()
{
	BW_GUARD;
	if (blocks_ != NULL)
	{
		// get rid of any blocks we have loaded
		for (uint i = 0; i < numBlocks_; i++)
		{
			StreamedBlock & sb = blocks_[i];
			if (sb.data_ != NULL)
			{
				// note: if we are still reading in, the block
				// destructor will wait for it to finish.
				bool owned = sb.data_->owner_ != NULL;
				bw_safe_delete( sb.data_ );
				MF_ASSERT_DEV( sb.data_ == NULL || (!owned) );				
			}
		}

		bw_safe_delete_array(blocks_);
	}


	RESOURCE_COUNTER_SUB(	ResourceCounters::DescriptionPool(
								"Animation/Streamed Animations",
								(uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER),
								( sizeof(*this) ), 0 );

}


/**
 *	Prep a channel for streaming with this streamed animation.
 *	This should only be called for channels that want to be
 *	added after loading, i.e. during conversion.
 */
void StreamedAnimation::prep( StreamedAnimationChannel * pChannel )
{
	BW_GUARD;
	MF_ASSERT_DEV( blocks_ != NULL );

	StreamedAnimationChannel & sac = *pChannel;
	sac.pBlocks_ = new ChannelBlock*[numBlocks_];
	sac.numBlocks_ = numBlocks_;
	for (uint j = 0; j < numBlocks_; j++) sac.pBlocks_[j] = NULL;
}


/**
 *	Add a channel to our list
 */
void StreamedAnimation::add( StreamedAnimationChannel * pChannel )
{
	BW_GUARD_MEMTRACKER(StreamedAnimation_Add);

	channels_.push_back( pChannel );

	// if we haven't loaded yet then this is all we need do
	if (blocks_ == NULL) return;

	// if we're here, then this channel was added after we loaded,
	// i.e. it is new and we should add it to the cache.

	// see if this channel punches up the block header size
	int chanNum = static_cast<int>(channels_.size() - 1);
	bool upSize = ((chanNum & 3) == 0);

	// add this channel's data to each of our blocks
	for (uint i = 0; i < numBlocks_; i++)
	{
		StreamedBlock & sb = blocks_[i];
		ChannelBlock * cb = pChannel->pBlocks_[i];
		uint8 cbWords = cb->calcNumWords();

		// make sure we're not trying to add a channel to an animation
		// that is already partly streamed (eugh) - we don't support it
		if (sb.offset_ != uint32(-1))
		{
			ERROR_MSG( "StreamedAnimation::add: "
				"Cannot add new channel to already loaded streamed anim\n" );
			return;
		}

		// allocate or reallocate the data pointer
		if (sb.data_ == NULL)
			sb.data_ = new CacheRecord( NULL, &sb );

		// create a new binary block and copy old data in
		uint32 newsize = (cbWords + int(upSize)) * sizeof(uint32);			
		BinaryPtr newData = 
			new BinaryBlock( NULL, sb.size_ + newsize, "Animation/Streamed Animation Add", NULL );
		memcpy( newData->cdata(), sb.data_->pData_->data(), sb.size_ );

		// set our word count
		if (upSize)
		{
			memmove( ((uint8*)newData->cdata()) + chanNum + sizeof(uint32),
				((uint8*)newData->data()) + chanNum,
				sb.size_ - chanNum );
			*(uint32*)(((uint8*)newData->cdata()) + chanNum) = 0;
			sb.size_ += sizeof(uint32);
		}
		((uint8*)newData->cdata())[chanNum] = cbWords;

		// and copy our data in
		uint32 cbSize = cbWords * sizeof(uint32);
		memcpy( ((uint8*)newData->cdata()) + sb.size_, cb, cbSize );
		sb.size_ += cbSize;

		// replace data in cache record
		sb.data_->pData_ = newData;

		// finally, free the source channel block and reset it to NULL,
		// since they're supposed to point to our data
		bw_free( pChannel->pBlocks_[i] );
		pChannel->pBlocks_[i] = NULL;
	}

	// TODO: If we wanted to actually use this animation now we'd have
	// to go and fix up all the channel block pointers in each channel.
	// Maybe there could be a fixup function to do this if so desired.
}
/**
 *	Load in the data for and set up this streamed animation
 */
void StreamedAnimation::load( BinaryFile & bf, float totalTime,
	StreamedDataCache * pSource, uint32 streamedOffset )
{
	BW_GUARD;
	// store our source
	pSource_ = pSource;

	// find out how many blocks and allocate space for them
	numBlocks_ = (uint32(ceilf(totalTime + 1.f)) + (STREAM_BLOCK_SIZE-1)) /
		STREAM_BLOCK_SIZE;
	blocks_ = new StreamedBlock[numBlocks_];

	// read in the sizes of each block
	for (uint i = 0; i < numBlocks_; i++)
	{
		StreamedBlock & sb = blocks_[i];
		sb.offset_ = streamedOffset;
		sb.data_ = NULL;
		sb.reader_ = NULL;
		// see if we are not yet in the cache
		if (streamedOffset != uint32(-1))
		{
			bf >> sb.size_;
			streamedOffset += sb.size_;
		}
		else
		{
			sb.size_ = 0;
		}
	}

	// tell all our channels how many blocks to expect
	for (uint i = 0; i < channels_.size(); i++)
	{
		this->prep( &*channels_[i] );
	}
}

/**
 *	Save our preload data to the given binary file
 */
void StreamedAnimation::save( BinaryFile & bf )
{
	BW_GUARD;
	for (uint i = 0; i < numBlocks_; i++)
	{
		bf << blocks_[i].size_;
	}
}

/**
 *	Save our streamed animation data out to the anim cache.
 *	This method should only be called on a newly created animation.
 */
uint32 StreamedAnimation::dam( uint32 streamedOffset )
{
	BW_GUARD;
	uint32 streamedSize = 0;

	// just write out each block
	for (uint i = 0; i < numBlocks_; i++)
	{
		StreamedBlock & sb = blocks_[i];
		sb.offset_ = streamedOffset;
		if (sb.data_ != NULL)
			pSource_->immsave( sb.offset_, sb.size_, sb.data_->pData_->data() );

		streamedOffset += sb.size_;
		streamedSize += sb.size_;
	}

	// and return how much we wrote
	return streamedSize;
}


/**
 *	This method touches the given time in the animation.
 *	Currently this is the only way of getting anything loaded,
 *	i.e. after you already want to use it :)
 */

void StreamedAnimation::touch( float time )
{
	BW_GUARD;
	uint32 block = uint32(time) / STREAM_BLOCK_SIZE;
	if (block >= numBlocks_) return;

	StreamedBlock & sb = blocks_[block];	

	// see if we are reading something in
	if (sb.reader_ == NULL)
	{ // not reading in

		// see if the data is already there
		if (sb.data_ != NULL)
		{
			sb.data_->touch();
			return;	// hopefully the common case!
		}

		// ok, let's load then
		sb.data_ = new CacheRecord( this, &sb );
		sb.reader_ = pSource_->load( sb.offset_, sb.data_->pData_ );
		//dprintf( "0x%08X Started reading block %d\n", this, block );
	}
	else
	{ // is reading in

		// see if the reading is complete
		if (pSource_->fileAccessDone( sb.reader_ ))
		{
			//dprintf( "0x%08X Finished reading block %d\n", this, block );
#ifdef SEPARATE_READ_THREAD
			bool ok = sb.reader_->finished_ == 1;
#else
			bool ok = true;
#endif
			// derefernce reader (tracker)
			sb.reader_ = NULL;

			// tell everyone about it then
			if (ok) this->bind( block );
		}

		// make sure it doesn't disappear underneath us anyway
		sb.data_->touch();
	}
}

/**
 *	This private method flicks the given block from our streamed in blocks
 */
void StreamedAnimation::flick( uint32 block )
{
	BW_GUARD;
	StreamedBlock & sb = blocks_[block];

	// wait for it to finish loading if it is still doing so
	// this could happen if there are great demands on a very small cache
	if (sb.reader_ != NULL)
	{
		while (!pSource_->fileAccessDone( sb.reader_ ))
			Sleep( 5 );

		// derefernce reader (tracker)
		sb.reader_ = NULL;
	}
	else
	{
		// otherwise loose any channels bound to this block
		this->loose( block );
	}

	// and we no longer have a record of it
	sb.data_ = NULL;

	// we do not delete the CacheRecord because
	// we are being called by its destructor (yep)
}


/**
 *	This method binds a block that has finished being read in.
 */
void StreamedAnimation::bind( uint32 block )
{
	BW_GUARD;
	StreamedBlock & sb = blocks_[block];

	// tell all channels about the data
	uint numChannels = static_cast<uint>(channels_.size());
	uint32 offset = (numChannels + 3) & ~3L;
	uint8 * sizes = (uint8*)sb.data_->pData_->cdata();

	for (uint i = 0; i < numChannels; i++)
	{
		uchar numWords = *sizes++;
		if (numWords != 0)
		{
			channels_[i]->pBlocks_[block] =
				(ChannelBlock*)(((uint8*)sb.data_->pData_->data()) + offset);
			offset += uint32(numWords) * sizeof(uint32);
		}
		// if no data in block for this channel,
		// then it just uses its fallback transform.
	}
}

/**
 *	This method looses a block in preparation for unloading.
 */
void StreamedAnimation::loose( uint32 block )
{
	BW_GUARD;
	StreamedBlock & sb = blocks_[block];

	// tell all channels about the data
	uint numChannels = static_cast<uint>(channels_.size());
	for (uint i = 0; i < numChannels; i++)
	{
		channels_[i]->pBlocks_[block] = NULL;
	}
}




/**
 *	Constructor
 */
StreamedAnimation::CacheRecord::CacheRecord(
		StreamedAnimation * owner, StreamedBlock * block ) :
	owner_( owner ),
	next_( NULL ),
	prev_( NULL )
{
	BW_GUARD_MEMTRACKER( StreamedAnimation_CacheRecord );

	while ( ( s_cacheSize_ + block->size_ ) > s_cacheMaxSize_ &&
			s_cacheTail_ != NULL && s_cacheTail_ != this )
	{
		delete s_cacheTail_;
	}

	pData_ = new BinaryBlock( NULL, block->size_, "Animation/Streamed Animation Cache", NULL );

	if (owner_ != NULL)
	{
		s_cacheSize_ += block->size_;

		RESOURCE_COUNTER_ADD(	ResourceCounters::DescriptionPool(
									"Animation/Streamed Animation Cache",
									(uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER ),
									( sizeof(*this) + block->size_ ), 0 );

		s_CRCons++;
		s_CRLive++;

		this->link();
	}
}

/**
 *	Destructor
 */
StreamedAnimation::CacheRecord::~CacheRecord()
{
	BW_GUARD;
	if (owner_ != NULL)
	{
		s_CRDest++;
		s_CRLive--;

		this->unlink();

		StreamedBlock * block = NULL;
		uint i = 0;
		for (; i < owner_->numBlocks_; i++)
			if ((block=&owner_->blocks_[i])->data_ == this) break;

		MF_ASSERT_DEV( block );

		if( block )
		{
			s_cacheSize_ -= block->size_;

			RESOURCE_COUNTER_SUB(	ResourceCounters::DescriptionPool(
									"Animation/Streamed Animation Cache",
									(uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER),
									( sizeof(*this) + block->size_ ), 0 );

			owner_->flick( i );
		}
	}

	pData_ = NULL;
}


/**
 *	Touch this cache record
 */
void StreamedAnimation::CacheRecord::touch()
{
	BW_GUARD;
	if (s_cacheHead_ == this) return;

	this->unlink();
	this->link();
}


/**
 *	Link into the head of the cache chain
 */
void StreamedAnimation::CacheRecord::link()
{
	BW_GUARD;
	next_ = s_cacheHead_;
	s_cacheHead_ = this;
	if (next_ != NULL)
		next_->prev_ = this;
	else
		s_cacheTail_ = this;
}

/**
 *	Unlink from the cache chain
 */
void StreamedAnimation::CacheRecord::unlink()
{
	BW_GUARD;
	if (next_ != NULL)
		next_->prev_ = prev_;
	else
		s_cacheTail_ = prev_;
	if (prev_ != NULL)
		prev_->next_ = next_;
	else
		s_cacheHead_ = next_;
	next_ = NULL;
	prev_ = NULL;
}


/// static initialisers
StreamedAnimation::CacheRecord * StreamedAnimation::s_cacheHead_ = NULL;
StreamedAnimation::CacheRecord * StreamedAnimation::s_cacheTail_ = NULL;
size_t StreamedAnimation::s_cacheSize_		= 0;
/**
 *	The maximum number bytes that will be used for streamed animation blocks.
 *	Accessible through the watcher 'Memory/StreamedBlocks_CacheMaxSize'
 *	Setting this value too low can cause the animation system to be starved of 
 *	data resulting in animation glitches and excessive disk usage.
 */
size_t StreamedAnimation::s_cacheMaxSize_	= 1<<20;

THREADLOCAL( StreamedDataCache* ) StreamedAnimation::s_loadingAnimCache_= NULL;
THREADLOCAL( StreamedDataCache* ) StreamedAnimation::s_savingAnimCache_ = NULL;

THREADLOCAL( StreamedAnimation* ) StreamedAnimation::s_currentAnim_ = NULL;



// -----------------------------------------------------------------------------
// Section: ChannelBlock
// -----------------------------------------------------------------------------

uint8 ChannelBlock::calcNumWords() const
{
	BW_GUARD;
	uint8 endWord = 255;
	const uint8 * roster;
	uint8 timeSum;

#define calcNumWords_PART( kname, kchar, ktype )					\
	timeSum = 0;													\
	roster = this->kname##Roster();									\
	const ktype * kname##KeyData = this->kname##KeyData();			\
	do																\
	{																\
		uint8 durHere = *roster++;									\
		timeSum += durHere & 0x7F;									\
		if (!(durHere & 0x80)) kname##KeyData++;					\
	} while (timeSum < STREAM_BLOCK_SIZE);							\
	if (!(kchar##Header_.rosterOffset_ & 0x80)) kname##KeyData++;	\
	if (kname##KeyData != this->kname##KeyData())					\
	{																\
		size_t diff = ((uint32*)kname##KeyData) - ((uint32*)this);	\
		MF_ASSERT( diff <= UCHAR_MAX );								\
		endWord = ( uint8 ) diff;									\
	}

	calcNumWords_PART( scale, s, Vector3 )
	calcNumWords_PART( position, p, Vector3 )
	calcNumWords_PART( rotation, r, Quaternion )

	// if we want to use the fallback transform for everything,
	// then this block is effectively empty
	if (endWord == 255) return 0;

	// otherwise it's just up to endWord
	return endWord;
}

// -----------------------------------------------------------------------------
// Section: StreamedAnimationChannel
// -----------------------------------------------------------------------------


/**
 *	Constructor.
 */
StreamedAnimationChannel::StreamedAnimationChannel() :
	pBlocks_( NULL ),
	numBlocks_( 0 )
{
}


/**
 *	Destructor.
 */
StreamedAnimationChannel::~StreamedAnimationChannel()
{
	BW_GUARD;
	// our StreamedAnimation had better have forgotten about us!
	bw_safe_delete_array(pBlocks_);
}

/**
 *	Copy constructor.
 */
StreamedAnimationChannel::StreamedAnimationChannel(
		const StreamedAnimationChannel & other ) :
	pBlocks_( NULL ),
	numBlocks_( 0 ),
	scaleFallback_( other.scaleFallback_ ),
	positionFallback_( other.positionFallback_ ),
	rotationFallback_( other.rotationFallback_ )
{
}


/**
 *	Slow result method
 */
Matrix StreamedAnimationChannel::result( float time ) const
{
	BW_GUARD;
	Matrix m;
	this->result( time, m );
	return m;
}


/**
 *	Faster result method
 */
void StreamedAnimationChannel::result( float time, Matrix & out ) const
{
	BW_GUARD;
	BlendTransform bt;
	this->result( time, bt );
	bt.output( out );
}

/**
 *	Fastest result method
 */
void StreamedAnimationChannel::result( float ftime, BlendTransform & out ) const
{
	BW_GUARD;
	// see if the block has been loaded
	uint32 itime = uint32(ftime);
	uint32 block = itime / STREAM_BLOCK_SIZE;
	itime -= block * STREAM_BLOCK_SIZE;
	ftime -= block * STREAM_BLOCK_SIZE;
	if (block < numBlocks_ && pBlocks_[block] != NULL)
	{
		// use loaded block then
		ChannelBlock & cb = *pBlocks_[block];
		uint8 lowTime, highTime;
		const uint8 * roster;
		uint8 durHere;
		float param, fdur;

#define SAC_result_PART( kname, kheader, ktype, interpolator )				\
		/* find kname key */												\
		highTime = 0;														\
		roster = cb.kname##Roster();										\
		const ktype * kname##KeyData = cb.kname##KeyData();					\
		do																	\
		{																	\
			durHere = *roster++;											\
			highTime += (durHere & 0x7F);									\
			if (!(durHere & 0x80)) kname##KeyData++;						\
		} while (highTime <= itime);										\
																			\
		/* ok, now scaleKeyData-1 at time lowTime = (highTime-durHere&0x7f)
		// is the bottom key, and scaleKeyData at time highTime is the top key.
		// if durHere >> 7 then bottom data is fallback not scaleKeyData-1
		// if (*roster) >> 7 then top data is fallback
		// boundaries:
		// if lowTime is 0 then subtract durBefore from it
		// if highTime is STREAM_BLOCK_SIZE then add durAfter to it
		// if highTime is STREAM_BLOCK_SIZE then look at rosterOffset>>7 not
		//  *roster to see if top data is fallback */						\
		lowTime = highTime - (durHere & 0x7F);								\
																			\
		const ktype * kname##BotKey, * kname##TopKey;						\
		param = ftime;														\
		fdur = float(durHere & 0x7F);										\
																			\
		/* get bot key and update param & fdur */							\
		if (lowTime != 0)													\
		{																	\
			param -= lowTime;												\
		}																	\
		else																\
		{																	\
			float f = kheader.durBefore_;									\
			param += f;														\
			fdur += f;														\
		}																	\
		/*botTime = lowTime ? lowTime : -int(cb.sHeader_.durBefore);*/		\
		kname##BotKey = (!(durHere & 0x80)) ?								\
			kname##KeyData - 1 : &kname##Fallback_;							\
																			\
		/* get top key and update param & fdur */							\
		if (highTime != STREAM_BLOCK_SIZE)									\
		{																	\
			durHere = *roster;												\
			/*topTime = highTime;*/											\
		}																	\
		else																\
		{																	\
			durHere = kheader.rosterOffset_;								\
			/*topTime = highTime + cb.sHeader_.durAfter_;*/					\
			fdur += kheader.durAfter_;										\
		}																	\
		kname##TopKey = (!(durHere & 0x80)) ?								\
			kname##KeyData : &kname##Fallback_;								\
																			\
		ktype kname##Out;													\
		interpolator( &kname##Out, kname##BotKey, kname##TopKey, param / fdur )


		SAC_result_PART( scale, cb.sHeader_, Vector3, XPVec3Lerp );
		SAC_result_PART( position, cb.pHeader_, Vector3, XPVec3Lerp );
		SAC_result_PART( rotation, cb.rHeader_, Quaternion, XPQuaternionSlerp );

		out = BlendTransform( rotationOut, scaleOut, positionOut );

		/*
		out = BlendTransform(
			cb.rotationKey( itime, rotationFallback_ ),
			cb.scaleKey( itime, scaleFallback_ ),
			cb.positionKey( itime, positionFallback_ ) );
		*/
	}
	else
	{
		// use fallback transform
		out = BlendTransform(
			rotationFallback_, scaleFallback_, positionFallback_ );
	}
}



/**
 *	Load method.
 */
bool StreamedAnimationChannel::load( BinaryFile & bf )
{
	BW_GUARD;
	// our StreamedAnimation will take care of setting up the blocks stuff
	if (StreamedAnimation::s_currentAnim_ != NULL)
		StreamedAnimation::s_currentAnim_->add( this );

	if( !this->AnimationChannel::load( bf ) )
		return false;

	bf >> scaleFallback_;
	bf >> positionFallback_;
	bf >> rotationFallback_;

	return !!bf;
}


/**
 *	Save method.
 */
bool StreamedAnimationChannel::save( BinaryFile & bf ) const
{
	BW_GUARD;
	// our StreamedAnimation will take care of saving out the blocks stuff

	if( !this->AnimationChannel::save( bf ) )
		return false;

	bf << scaleFallback_;
	bf << positionFallback_;
	bf << rotationFallback_;

	return !!bf;
}

void StreamedAnimationChannel::preCombine( const AnimationChannel & rOther )
{
	ERROR_MSG( "StreamedAnimationChanne::preCombine - method not supported" );
}

void StreamedAnimationChannel::postCombine( const AnimationChannel & rOther )
{
	ERROR_MSG( "StreamedAnimationChanne::postCombine - method not supported" );
}

namespace 
{
	/*
	 *	This class helps with creating the blocks used by the streamed animations
	 */
	template <class KeyType>
	class BlockHelper
	{
	public:
		typedef BW::vector< std::pair< float, KeyType > > KeyFrames;
		typedef BW::vector<uint8> Roster;
		typedef BW::vector<KeyType> Values;

		/*
		 *	Constructor
		 *	The constructor copies the keyframe data and makes sure
		 *	we have at least two keys in the animation, this helps
		 *	the generation of the blocks.
		 *	@param frames the keyframes to create blocks from
		 *	@param numBlocks the number of blocks we want to make
		 *	@param defaultKey the default value of the keytype
		 */
		BlockHelper( const KeyFrames& frames, uint32 numBlocks,
			const KeyType& defaultKey ) :
			fallback_( defaultKey ),
			frames_( frames ),
			lastKeyOffset_(),
			firstKeyOffset_(),
			lastKeyFallback_()
		{

			BW_GUARD;
			// Ensure that there are at least two keys in the animation
			// and that the keys that are there cover the full range of the anim
			uint32 totalTime = STREAM_BLOCK_SIZE * numBlocks;

			// If there are actual keyframes, the first frame is used as the fallback
			// Otherwise we add the fallback key as the first frame
			if (frames_.size())
			{
				fallback_ = frames_[0].second;
			}
			else
			{
				frames_.push_back( std::make_pair(0.f, fallback_) );
			}

			// If there is no key at the start of the animation we extend the first key
			// to the start
			if (frames_.front().first > 0.f)
			{
				frames_.insert(frames_.begin(), std::make_pair(0.f, frames_.front().second ) );
			}

			// If there is no key at the end of the animation we extend the first last
			// to the start
			if (frames_.back().first < float(totalTime))
			{
				frames_.push_back( std::make_pair(float(totalTime), frames_.back().second ) );
			}
		}

		/*
		 *	This method sets up the data for a block
		 *	@param blockIndex the index of the block
		 *	@param beginKey the key data for the start of the block
		 *	@param endKey the key data for the end of the block
		 */
		void setBlock(uint32 blockIndex, const KeyType& beginKey, const KeyType& endKey)
		{
			BW_GUARD;
			// Set our default values
			roster_.clear();
			values_.clear();
			lastKeyOffset_ = 0;
			firstKeyOffset_ = 0;

			// Set up the start and end times for the block
			uint32 beginTime = blockIndex * STREAM_BLOCK_SIZE;
			uint32 endTime = beginTime + STREAM_BLOCK_SIZE;
			float beginTimeF = float(beginTime);
			float endTimeF = float(endTime);


			// Find the first and last keys in the block
			// All keyframes from beginFrame up until and including
			// endFrame are to be included in the block
			typename KeyFrames::iterator beginFrame = frames_.begin();
			typename KeyFrames::iterator endFrame = frames_.end();

			while ((beginFrame + 1)->first <= beginTimeF)
			{
				++beginFrame;
			}
			while ((endFrame - 1)->first >= endTimeF)
			{
				--endFrame;
			}

			// Make up the rosters for all the frames between
			// beginFrame and endFrame
			// The roster contains the delta between each frame
			// stored in the block
			typename KeyFrames::iterator internalFrame = beginFrame + 1;

			uint32 prevTime = beginTime;
			while (internalFrame != endFrame)
			{
				uint32 time = uint32(internalFrame->first);
				roster_.push_back( uint8(time - prevTime) );
				values_.push_back( internalFrame->second );
				prevTime = time;
				++internalFrame;
			}

			// Make sure the total time in the roster equals
			// the size of the block
			roster_.push_back( uint8(endTime - prevTime) );

			// The top bit of the roster is used to dictate
			// whether the current key is default or not
			const uint8 USE_DEFAULT_KEY = 1 << 7;

			// Iterate over the values and determine whether
			// the keys are default or not
			typename Values::iterator valuesIt = values_.begin();
			for (uint32 i = 1; i < roster_.size(); i++)
			{
				if (*(valuesIt) == fallback_)
				{
					roster_[i] |= USE_DEFAULT_KEY;
					valuesIt = values_.erase( valuesIt );
				}
				else
				{
					++valuesIt;
				}
			}

			// Get the first and last key.
			// The first and last keys are special
			// in that they don't have to fall within
			// the block itself, they can be offset
			// by up to 255 frames outside the block range

			// Assume the last key is the fallback key
			lastKeyFallback_ = true;

			// Get the offset of the last key
			uint32 lastOffset = uint32(endFrame->first - endTime);

			// The last key is initialised to the end key value
			KeyType lastKey = endFrame->second;

			// If the lastOffset does not fit in a uint8, we use the supplied
			// endkey for the block as our last key
			if (lastOffset != uint32(uint8(lastOffset)))
			{
				lastOffset = 0;
				lastKey = endKey;
			}

			// If the last key is not the same as the fallback add it to our
			// list of values
			if (lastKey != fallback_)
			{
				values_.push_back( lastKey );
				lastKeyFallback_ = false;
			}
			// Store the last key offset
			lastKeyOffset_ = uint8(lastOffset);

			// Get the offset of the first key
			uint32 firstOffset = beginTime - uint32(beginFrame->first);

			// The first key is initialised to the begin key value
			KeyType firstKey = beginFrame->second;


			// If the firstOffset does not fit in a uint8, we use the supplied
			// beginkey for the block as our first key
			if (firstOffset != uint32(uint8(firstOffset)))
			{
				firstOffset = 0;
				firstKey = beginKey;
			}

			// Store the first key offset
			firstKeyOffset_ = uint8(firstOffset);

			// If the first key is the same as the fallback set up the roster to
			// use the default key for the first frame
			// Otherwise add the key at the front of the values list
			if (firstKey == fallback_)
			{
				roster_[0] |= USE_DEFAULT_KEY;
			}
			else
			{
				values_.insert(values_.begin(), firstKey);
			}
		}

		/*
		 *	This method copies the roster to the buffer passed in
		 *	@param pDest the buffer to copy the roster to
		 *	@return pointer to the buffer position after the roster
		 */
		uint8* copyRoster( uint8* pDest )
		{
			BW_GUARD;
			for (uint32 i = 0; i < roster_.size(); i++)
			{
				*(pDest++) = roster_[i];
			}
			return pDest;
		}

		/*
		 *	This method copies the values to the buffer passed in
		 *	@param pDest the buffer to copy the values to
		 *	@return pointer to the buffer position after the values
		 */
		uint8* copyValues( uint8* pDest )
		{
			BW_GUARD;
			KeyType* pData = (KeyType*)pDest;
			for (uint32 i = 0; i < values_.size(); i++)
			{
				*(pData++) = values_[i];
			}
			return (uint8*)pData;
		}

		const KeyType& fallback() { return fallback_; }

		// Get the offset from the first key to the start of the block
		uint8 firstKeyOffset() { return firstKeyOffset_; }
		uint8 lastKeyOffset() { return lastKeyOffset_; }

		// This method returns 1 << 7 if the last key is a fallback key
		// This is so that the value can be used directly
		uint8  lastKeyFallback() { return uint8(lastKeyFallback_ ? 0x80 : 0); }

		uint32 rosterSize() const
		{
			MF_ASSERT( roster_.size() <= UINT_MAX );
			return ( uint32 ) roster_.size();
		}

		uint32 valuesSize() { return static_cast<uint32>(
								sizeof(KeyType) * values_.size()); }

	private:
		Roster					roster_;
		Values					values_;
		KeyType					fallback_;
		KeyFrames				frames_;

		uint8 lastKeyOffset_;
		uint8 firstKeyOffset_;

		bool lastKeyFallback_;
	};
}


/**
 *	Mirror the given interpolated animation channel.
 */
void StreamedAnimationChannel::mirror( const InterpolatedAnimationChannel & iac )
{
	BW_GUARD;
	// copy the base class AnimationChannel data
	this->identifier( iac.identifier() );

	BlockHelper<Vector3> scaleHelper(iac.scaleKeys_, numBlocks_, Vector3( 1.f, 1.f, 1.f ) );
	BlockHelper<Vector3> positionHelper(iac.positionKeys_, numBlocks_, Vector3( 0.f, 0.f, 0.f ) );
	BlockHelper<Quaternion> rotationHelper(iac.rotationKeys_, numBlocks_, Quaternion(0.f, 0.f, 0.f, 1.f) );

	// take the first key of each and make it our fallback
	scaleFallback_ = scaleHelper.fallback();
	positionFallback_ = positionHelper.fallback();
	rotationFallback_ = rotationHelper.fallback();

	for (uint i = 0; i < numBlocks_; i++)
	{
		const float		rangeBegF = float(i*STREAM_BLOCK_SIZE);
		const float		rangeEndF = rangeBegF + float(STREAM_BLOCK_SIZE);

		// This is the blendtransform at the start of this channel block
		BlendTransform btBefore;
		iac.result( rangeBegF, btBefore );

		// This is the blend transform after the current channel block
		BlendTransform btAfter;
		iac.result( rangeEndF, btAfter );

		scaleHelper.setBlock( i, btBefore.scaling(), btAfter.scaling() );
		positionHelper.setBlock( i, btBefore.translation(), btAfter.translation() );
		rotationHelper.setBlock( i, btBefore.rotation(), btAfter.rotation() );

		// Calculate the roster size padded out to 4 bytes
		uint32 rostersSize = (scaleHelper.rosterSize() + 
			positionHelper.rosterSize() +
			rotationHelper.rosterSize() + 
			3)&~3L;

		// The size of the values
		uint32 valuesSize = scaleHelper.valuesSize() + 
			positionHelper.valuesSize() + 
			rotationHelper.valuesSize();


		// Allocate the memory for the block
		// The block is the ChannelBlockHeader plus the rosters plus the data
		ChannelBlock * pcb = (ChannelBlock*)bw_malloc( sizeof(ChannelBlock) +
			rostersSize + valuesSize );

		// Set up the first and last key offsets
		pcb->sHeader_.durBefore_ = scaleHelper.firstKeyOffset();
		pcb->sHeader_.durAfter_ = scaleHelper.lastKeyOffset();

		pcb->pHeader_.durBefore_ = positionHelper.firstKeyOffset();
		pcb->pHeader_.durAfter_ = positionHelper.lastKeyOffset();

		pcb->rHeader_.durBefore_ = rotationHelper.firstKeyOffset();
		pcb->rHeader_.durAfter_ = rotationHelper.lastKeyOffset();

		// write in the rosters and their offsets
		uint8* pData = (uint8*)(pcb+1);
		uint8* pRoster = pData;
		pcb->sHeader_.rosterOffset_ = 0 | scaleHelper.lastKeyFallback();
		pRoster = scaleHelper.copyRoster( pRoster );

		pcb->pHeader_.rosterOffset_ =  uint8(pRoster - pData) | 
			positionHelper.lastKeyFallback();
		pRoster = positionHelper.copyRoster( pRoster );

		pcb->rHeader_.rosterOffset_ = uint8(pRoster - pData) | 
			rotationHelper.lastKeyFallback();
		pRoster = rotationHelper.copyRoster( pRoster );

		// set up the offsets, the offsets are stored in uint32s
		// so sizes need to be divided by the size of uint32
		pcb->sHeader_.keyDataOffset_ = uint8(rostersSize / sizeof(uint32));
		pcb->pHeader_.keyDataOffset_ = uint8(pcb->sHeader_.keyDataOffset_ +
			scaleHelper.valuesSize() / sizeof(uint32));
		pcb->rHeader_.keyDataOffset_ = uint8(pcb->pHeader_.keyDataOffset_ +
			positionHelper.valuesSize() / sizeof(uint32));

		// memset the padding between the roster and the values
		memset( pRoster, 0, pData + rostersSize - pRoster );

		// Copy the data
		uint8* pValues = pData + rostersSize;
		pValues = scaleHelper.copyValues( pValues );
		pValues = positionHelper.copyValues( pValues );
		pValues = rotationHelper.copyValues( pValues );

		// some checking
		if (pcb->sHeader_.keyDataOffset_ == pcb->pHeader_.keyDataOffset_)
		{
			MF_ASSERT_DEV( (pcb->sHeader_.rosterOffset_ & 0x80) &&
				(pcb->scaleRoster()[0] & 0x80) );
		}

		// make this our block pointer
		pBlocks_[i] = pcb;
	}
}

StreamedAnimationChannel::TypeRegisterer
	StreamedAnimationChannel::s_rego_( STREAMED_ANIMATION_CHANNEL, New );



}	// namespace Moo

BW_END_NAMESPACE

// streamed_animation_channel.cpp
