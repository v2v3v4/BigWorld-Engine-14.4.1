
#include "pch.hpp"

#include "animation.hpp"

#ifndef CODE_INLINE

BW_BEGIN_NAMESPACE

#include "animation.ipp"

BW_END_NAMESPACE

#endif

#include "cstdmf/binaryfile.hpp"
#include "resmgr/multi_file_system.hpp"
#include "resmgr/bwresource.hpp"
#include "animation_manager.hpp"
#include "streamed_animation_channel.hpp"
#include "interpolated_animation_channel.hpp"
#include "translation_override_channel.hpp"
#include "node_catalogue.hpp"

#ifdef _DEBUG
	#include "cstdmf/profile.hpp"
	#include <strstream>
#endif

#include "cstdmf/string_builder.hpp"

DECLARE_DEBUG_COMPONENT2( "Moo", 0 )

BW_BEGIN_NAMESPACE

namespace Moo {

/**
 * Constructor for animation
 * @param anim pointer to the animation to copy
 * @param root the rootnode of the hierarchy the animation applies to
 */
Animation::Animation( Animation * anim, NodePtr root ) :
	totalTime_( anim->totalTime() ),
	identifier_( anim->identifier() ),
	internalIdentifier_( anim->internalIdentifier() ),
	pItinerantRoot_( NULL ),
	pStreamer_( anim->pStreamer_ ),
	pMother_( anim ),
	tickers_( anim->tickers_ )
{
	BW_GUARD;
	for( uint32 i = 0; i < anim->nChannelBinders(); i++ )
	{
		AnimationChannelPtr pAC = anim->channelBinder( i ).channel();
		channelBinders_.push_back( ChannelBinder( pAC, root->find( pAC->identifier() ) ) );
	}
}

/**
 * Contructor for animation
 * @param anim the animation to apply 
 */
Animation::Animation( Animation * anim ) :
	totalTime_( anim->totalTime() ),
	identifier_( anim->identifier() ),
	internalIdentifier_( anim->internalIdentifier() ),
	pItinerantRoot_( NULL ),
	pStreamer_( anim->pStreamer_ ),
	pMother_( anim ),
	tickers_( anim->tickers_ )
{
    BW_GUARD;
	NodePtr nullMooNodePointer = NULL;

	for( uint32 i = 0; i < anim->nChannelBinders(); i++ )
	{
		AnimationChannelPtr pAC = anim->channelBinder( i ).channel();
			Moo::NodeCatalogue& nodeCatalogue =
				Moo::NodeCatalogue::instance();
		NodePtr pNode = nodeCatalogue.find( pAC->identifier().c_str() );
		if (!pNode)
		{
			pNode = new Moo::Node;
			pNode->identifier( pAC->identifier() );
			pNode = nodeCatalogue.findOrAdd( pNode );
		}
		channelBinders_.push_back(ChannelBinder( pAC,pNode ));
	}
}

/**
 * Constructor for animation
 */
Animation::Animation() :
	totalTime_( 0 ),
	pItinerantRoot_( NULL )
{
}

/**
 * Destructor for animation
 */
Animation::~Animation()
{
	BW_GUARD;
	// remove ourselves from the animation manager's global map.
	// we will only be in it if we are an orphan ... if we have
	// a mother then she will be the one in the map.
	// However, we'll only be doing this if the AnimationManager actually
	// exists.  New Moo::PulseLights are able to store animations and are
	// implicitly held on to by their OmniLight representation in
	// LightContainers.  As of writing this comment, the Visual::draw()'s
	// static local variable called 'helper' is able to store LightContainers
	// for a longer lifetime than the AnimationManager exists, so without re-
	// writing the complex lifetime requirements of the management systems it's
	// easier to just not assume here that AnimationManager is still alive.
	if (!pMother_ && AnimationManager::pInstance())
	{
		AnimationManager::instance().del( this );
	}
}

uint32 Animation::nChannelBinders( ) const
{
	return static_cast<uint32>(channelBinders_.size());
}

const ChannelBinder& Animation::channelBinder( uint32 i ) const
{
	BW_GUARD;
	IF_NOT_MF_ASSERT_DEV( channelBinders_.size() > i )
	{
		MF_EXIT( "index out of range" );
	}

	return channelBinders_[ i ];
}

ChannelBinder& Animation::channelBinder( uint32 i )
{
	BW_GUARD;
	IF_NOT_MF_ASSERT_DEV( channelBinders_.size() > i )
	{
		MF_EXIT( "index out of range" );
	}

	return channelBinders_[ i ];
}

void Animation::addChannelBinder( const ChannelBinder& binder )
{
	BW_GUARD;
	channelBinders_.push_back( binder );
	pItinerantRoot_ = NULL;
	if (binder.channel() && binder.channel()->wantTick())
		tickers_.push_back( binder.channel() );
}

/**
 * This method plays a frame of an animation.
 * @param time the frame to play back
 */
void Animation::animate(  float time ) const
{
	BW_GUARD;
	ChannelBinderVector::const_iterator it = channelBinders_.begin();
	ChannelBinderVector::const_iterator end = channelBinders_.end();

	while (it != end)
	{
		it->animate( time );
		++it;
	}
}

#if 0
/**
 * This method blends two animations together
 * @param anim the animation to blend with.
 * @param time1 the frame to play back on this animation
 * @param time2 the frame to play back on the passed in animation
 * @param t the blend ratio between the two animations 0 = this animation 1 = passed in animation
 */
void Animation::blend( AnimationPtr anim, float time1, float time2, float t  ) const
{
	BW_GUARD;
	if( t <= 0 )
	{
		animate( time1 );
		return;
	}

	anim->animate( time2 );

	if( t >= 1 )
		return;


	ChannelBinderVector::const_iterator it = channelBinders_.begin();
	ChannelBinderVector::const_iterator end = channelBinders_.end();

	while (it != end)
	{
		NodePtr n = it->node();
		if( n )
		{
			BlendTransform b2( n->transform() );
			BlendTransform b1( it->channel()->result( time1 ) );
			b1.blend( t, b2 );
			b1.output( n->transform() );
		}

		++it;
	}
}
#endif

/**
 * This method blends a number of animations together
 * @param first the iterator of the first blended animation to play back.
 * @param final the iterator of the last + 1 blended animation to play back.
 */
void Animation::animate( BlendedAnimations::const_iterator first,
						BlendedAnimations::const_iterator final )
{
	BW_GUARD;
	if ( first+1 == final )
	{
		first->animation_->animate( first->frame_ );
	}
	else if (first != final)
	{
		BlendTransform channelTransform;

		ChannelBinderVector::const_iterator chan;
		ChannelBinderVector::const_iterator chanEnd;
		int chanIndex;

		for (BlendedAnimations::const_iterator it = first;
			it != final;
			++it)
		{
			if( it->blendRatio_ != 0 )
			{
				chan = it->animation_->channelBinders_.begin();
				chanEnd = it->animation_->channelBinders_.end();
				chanIndex = 0;
				while( chan != chanEnd )
				{
					float newBr = it->blendRatio_;
					if( it->alphas_ )
					{
						newBr *= (*it->alphas_)[ chanIndex ];
					}

					NodePtr n = chan->node();
					if( n )
					{
						if( newBr > 0 )
						{
							float oldBr = n->blend( Node::s_blendCookie_ );
							float sumBr = oldBr + newBr;
							chan->channel()->result( it->frame_, channelTransform );

							if (oldBr == 0.f)
							{
								n->blendTransform() = channelTransform;
							}
							else
							{
								n->blendTransform().blend( newBr / sumBr, channelTransform );
							}
							n->blend( Node::s_blendCookie_, sumBr );
						}
					}
					else
					{
						chan->channel()->nodeless( it->frame_, newBr );
					}

					++chan;
					++chanIndex;
				}
			}
		}
	}

	Node::s_blendCookie_ = (Node::s_blendCookie_+1) & 0x0fffffff;
}


/**
 * This method blends this animation into the node hierarchy
 * @param blendCookie the cookie of the current blend
 * @param frame the frame to play back
 * @param blendRatio the ratio of blend for this animation
 * @param pAlphas blendfactors for individual nodes
 */
void Animation::animate( int blendCookie, float frame, float blendRatio,
	const NodeAlphas * pAlphas )
{
	BW_GUARD;
	if (pStreamer_)
	{
		pStreamer_->touch( frame );
		pStreamer_->touch( min( totalTime_, frame + 8.f ) );
		pStreamer_->touch( max( 0.f, frame - 8.f ) );
	}

	if (blendRatio <= 0.f) return;

	BlendTransform channelTransform;

	ChannelBinderVector::const_iterator chan;
	int chanIndex;

	for (chan = channelBinders_.begin(), chanIndex = 0;
		chan != channelBinders_.end();
		++chan, ++chanIndex)
	{
		float newBr = (pAlphas == NULL) ?
			blendRatio : blendRatio * (*pAlphas)[ chanIndex ];

		const NodePtr& n = chan->node();
		if (!n)
		{
			chan->channel()->nodeless( frame, newBr );
			continue;
		}

		if( newBr > 0.f )
		{
			chan->channel()->result( frame, channelTransform );

			float oldBr = n->blend( blendCookie );
			float sumBr = oldBr + newBr;

			if (oldBr == 0.f)
			{
				n->blendTransform() = channelTransform;
			}
			else
			{
				n->blendTransform().blend( newBr / sumBr, channelTransform );
			}
			n->blend( blendCookie, sumBr );
		}
		else if (newBr < 0.f)
		{
			float oldBr = n->blend( blendCookie );
			if (oldBr > 0.f)
			{
				Matrix mtemp;
				chan->channel()->result( frame, channelTransform );
				channelTransform.blend( 1.f - newBr, BlendTransform() );
				channelTransform.output( mtemp );
				n->transform().preMultiply( mtemp );
			}
		}
	}
}


/**
 *	This method ticks this animation.
 *	Note that unlike with the animate methods, frame numbers here
 *	should not wrap around if the animation is being looped.
 */
void Animation::tick( float dtime, float oframe, float nframe,
	float bframe, float eframe )
{
	BW_GUARD;
	if (tickers_.empty()) return;

	for (AnimationChannelVector::const_iterator it = tickers_.begin();
		it != tickers_.end();
		it++)
	{
		(*it)->tick( dtime, oframe, nframe, bframe, eframe );
	}
}


/**
 *	This method finds the channel corresponding to the given node
 */
AnimationChannelPtr Animation::findChannel( NodePtr node ) const
{
	BW_GUARD;
	ChannelBinderVector::const_iterator it = channelBinders_.begin();
	ChannelBinderVector::const_iterator end = channelBinders_.end();

	while (it != end)
	{
		if( it->node() == node )
		{
			return it->channel();
		}
		++it;
	}
	return NULL;
}

AnimationChannelPtr Animation::findChannel( const BW::string& identifier )
{
	BW_GUARD;
	ChannelBinderVector::const_iterator it = channelBinders_.begin();
	ChannelBinderVector::const_iterator end = channelBinders_.end();

	while (it != end)
	{
		if( it->channel()->identifier() == identifier )
		{
			return it->channel();
		}
		++it;
	}
	return NULL;
}

/**
 * This method returns the itinerant root of the animation. The channel closest
 * to the root that actually moves.
 * @return the channelbinder for the itinerant root.
 */
ChannelBinder * Animation::itinerantRoot( ) const
{
	BW_GUARD;
	if( pItinerantRoot_ )
		return pItinerantRoot_;

	// for now we assume that the itinerant root channel is always the
	//  second one in the animation; after the scene root, which stays still.
	if (channelBinders_.size() >= 2)
	{
		pItinerantRoot_ = const_cast<ChannelBinder*>( &channelBinders_[ 1 ] );
	}


#if 0
	// Try to find a channel that is the closest child of the root of all the other channels.
	bool foundRoot = false;

	// Step throught all the channels to find a root
	for( uint32 i = 0; i < channelBinders_.size() && foundRoot == false; i++ )
	{
		uint32 closestChild = 0xffffff;
		uint32 maxDescendants = 0;
		pItinerantRoot_ = NULL;
		const ChannelBinder& root = channelBinders_[ i ];
		if( root.node() )
		{
			pItinerantRoot_ = NULL;
			foundRoot = true;

			// Step through all the channels to find out if they are all children, and also
			// to find the closest child to the root.
			for( uint32 j = 0; j < channelBinders_.size() && foundRoot == true; j++ )
			{
				if( j != i )
				{
					NodePtr pChild = channelBinders_[ j ].node();
					if( pChild != NULL )
					{
						uint32 childDistance = 0;
						uint32 nDescendants = pChild->countDescendants();
						// Are we a child?
						while( pChild && !( pChild == root.node() ) )
						{
							childDistance++;
							pChild = pChild->parent();
						}
						foundRoot = pChild == root.node();

						// Are we the closest child, if we are a child at all?
						if ((foundRoot && childDistance < closestChild) ||
							(foundRoot && childDistance == closestChild && nDescendants > maxDescendants ) )
						{
							closestChild = childDistance;
							pItinerantRoot_ = const_cast<ChannelBinder*>( &channelBinders_[ j ] );
							maxDescendants = nDescendants;
						}
					}
				}
			}
		}
	}
#endif

	if( !pItinerantRoot_ )
	{
		WARNING_MSG( "Moo::Animation::itinerantRoot:"
				"Animation %s has no channel that matches the criteria for the itinerant root!\n"
				"\t(stuffed node hierarchy)\n",
			identifier_.c_str() );
		if(channelBinders_.size() >= 1)
		{
			pItinerantRoot_ = const_cast<ChannelBinder*>( &channelBinders_[ 0 ] );
		}
	}

	return pItinerantRoot_;
}

/**
 *	Load the given resource ID into this animation
 */
bool Animation::load( const BW::string & resourceID )
{
	BW_GUARD;
	DataSectionPtr pAnimSect;
	BinaryPtr bptr;

	const StreamedDataCache::EntryInfo * ei = NULL;

	// find the modified time of the file if it's there
	uint64 modifiedTime = BWResource::modifiedTime( resourceID );

	// get our data either from a file or an anim cache
	BinaryFile * pbf = NULL;

	bool usingCache = StreamedAnimation::s_loadingAnimCache_ != NULL &&
		(ei = StreamedAnimation::s_loadingAnimCache_->
		findEntryData( resourceID, 0, modifiedTime )) != NULL;

	for(;;)
	{
		bw_safe_delete(pbf);

		if( usingCache )
		{
			//DEBUG_MSG( "Loading animation from cache: %s\n", resourceID.c_str() );

			StreamedDataCache* pCache = StreamedAnimation::s_loadingAnimCache_;
			//uint32 preloadSize;
			//pCache->preload( dataOffset, sizeof(preloadSize), &preloadSize );
			//dataOffset += sizeof(preloadSize);

			pbf = new BinaryFile( NULL );
			void * preloadData = pbf->reserve( ei->preloadSize_ );
			pCache->preload( ei->fileOffset_, ei->preloadSize_, preloadData );

			pStreamer_ = new StreamedAnimation();
			StreamedAnimation::s_currentAnim_ = &*pStreamer_;
		}
		else
		{
			pAnimSect = BWResource::openSection( resourceID );
			if (!pAnimSect) return false;

			bptr = pAnimSect->asBinary();

			if (!bptr) return false;

			pbf = new BinaryFile( NULL );
			pbf->wrap( (void*)bptr->data(), bptr->len(), false );
		}
		BinaryFile & bf = *pbf;


	#ifdef _DEBUG
		uint64	loadTime = timestamp();
	#endif

		// read the anim header
		bf >> totalTime_ >> identifier_ >> internalIdentifier_;

		Animation* pOverride = NULL;

		// now read the channel data
		int	numChannelBinders = 0;
		bf >> numChannelBinders;
		for (int i = 0; i < numChannelBinders; ++i)
		{
			int	type;
			bf >> type;
			if ( type == AnimationChannel::MORPH_ANIMATION_CHANNEL_NOT_SUPPORTED )
			{
				// morph animation is no longer supported, skipping it
				break;
			}
			AnimationChannelPtr	pAC = AnimationChannel::construct( type );
			if (pAC)
			{
				if (pAC->load( bf ))
				{
					this->addChannelBinder( ChannelBinder( pAC, NULL ) );		
				}
				else
				{
					ERROR_MSG(	"Error in resource file: '%s' channel: '%s'\n",
								resourceID.c_str(),
								pAC->identifier().c_str());
					bf.error(true);
					break;
				}
			}
			else
			{
				ERROR_MSG( "Unable to construct animation channel from file: '%s' channel binder: %d type: %d\n",
					resourceID.c_str(), i, type );
				bf.error(true);
				break;
			}


			if (type == AnimationChannel::TRANSLATION_OVERRIDE_CHANNEL)
			{
				TranslationOverrideChannel* toc = reinterpret_cast<TranslationOverrideChannel*>(pAC.getObject());
				pOverride = toc->pBaseAnim().getObject();
			}
		}

		if( !bf )
		{
			if( usingCache )
			{
				//We do this to try again without using the cache.
				usingCache = false;
				StreamedAnimation::s_loadingAnimCache_->deleteOnClose( true );
				continue;
			}
			else
			{
				bw_safe_delete(pbf);
				return false;
			}
		}

		if (pOverride)
		{
			MF_ASSERT_DEV(!pStreamer_.hasObject());
			pStreamer_ = pOverride->pStreamer_;
		}

		// and set up the streaming animation if we found one
		bool explicitSave = false;
		if (pStreamer_ && !pOverride)
		{
			if (ei)
			{
				pStreamer_->load( bf, totalTime_,
					StreamedAnimation::s_loadingAnimCache_,
					ei->fileOffset_ + ei->preloadSize_ );
			}
			else
			{
				explicitSave = true;
			}
		}

		StreamedAnimation::s_currentAnim_ = NULL;
		const bool ret = !!bf;
		bw_safe_delete(pbf);

		// if we want to stream but we didn't load from the cache,
		// add ourselves to the cache for next time.
		if (ret && StreamedAnimation::s_savingAnimCache_ != NULL &&
			((!pStreamer_ && !pOverride) || explicitSave) )
		{
			this->save( resourceID, modifiedTime );
		}

		if( !ret && usingCache )
		{
			usingCache = false;
			StreamedAnimation::s_loadingAnimCache_->deleteOnClose( true );
			continue;
		}
		return ret;
	}

	MF_EXIT( "Animation::load: Should never reach here" );

	return false;
}


/**
 *	Helper class for converting saved animation channels
 */
class StreamedAnimConverter : public InterpolatedAnimationChannel::Converter
{
public:
	StreamedAnimConverter() : pIneligible_( NULL ) { }

	virtual int type()
		{ return 5; }
	virtual bool eligible( const InterpolatedAnimationChannel & iac )
		{ return static_cast<const AnimationChannel*>( &iac ) != pIneligible_; }
	virtual bool saveAs( BinaryFile & bf,
		const InterpolatedAnimationChannel & iac );

	AnimationChannel * pIneligible_;
};

bool StreamedAnimConverter::saveAs( BinaryFile & bf,
	const InterpolatedAnimationChannel & iac )
{
	BW_GUARD;
	StreamedAnimation * pStreamer = StreamedAnimation::s_currentAnim_;

	StreamedAnimationChannel * pSAC = new StreamedAnimationChannel();

	pStreamer->prep( pSAC );
	pSAC->mirror( iac );
	pStreamer->add( pSAC );

	return pSAC->save( bf );
}

static StreamedAnimConverter s_streamedAnimConverter;


/**
 * Performs one of two actions depending on the state of
 * StreamedAnimation::s_savingAnimCache_:
 *  - If NULL: saves to a .animation file.
 *  - Otherwise: saves data to the streaming cache in a form
 *	that can be loaded more quickly (data will be added to
 *	the appropriate .anca file).
 * 
 * @param resourceID The resource ID for this animation data.
 * @param useModifiedTime When saving a .anca file, this is used as
 *	the modification time for the stream cache entry.
 * @return False on failure.
*/
bool Animation::save( const BW::string & resourceID, uint64 useModifiedTime )
{
	BW_GUARD;

	const bool convertToStreamed =
		(StreamedAnimation::s_savingAnimCache_ != NULL);

	BinaryFile * pbf = NULL;

	if (!convertToStreamed)
	{
		BW::string	filename = resourceID;
		if (filename.empty())
		{
			ASSET_MSG( "Animation::save: Could not resolve resource ID '%s'\n",
				resourceID.c_str() );
			return false;
		}

		TRACE_MSG( "Animation::save: Writing converted animation to file: %s\n",
			resourceID.c_str() );

		FILE * f = BWResource::instance().fileSystem()->posixFileOpen(
			filename, "wb" );
		if (f == NULL)
		{
			char errorMsgString[2048] = { 0 };
			BW::StringBuilder errorStringBuilder(
				errorMsgString, ARRAY_SIZE( errorMsgString ) );
			errorStringBuilder.appendf(
				"Failed to open animation file to save: %s\n",
				filename.c_str() ); 

#ifndef MF_SERVER
			//  get the last windows system error message
			char lastErrorString[2048] = { 0 };
			if(BWUtil::getLastErrorMsg( lastErrorString,2048 ))
			{
				errorStringBuilder.appendf(
					" windowserror: %s", lastErrorString );
			}
#endif // MF_SERVER

			ERROR_MSG( "%s\n", errorStringBuilder.string() );
			return false;
		}

		pbf = new BinaryFile( f );
	}
	else //saving to streaming buffer.
	{
		TRACE_MSG( "Animation::save: Writing converted animation to "
			"streaming cache: %s\n", resourceID.c_str() );

		pbf = new BinaryFile( NULL );

		const int INITIAL_WRITE_BUFFER_SIZE = 100 * 1024;
		pbf->reserve(INITIAL_WRITE_BUFFER_SIZE);
	}
	MF_ASSERT(pbf != NULL);

	// no early returns from here on

	StreamedAnimationPtr pStreamerSave = pStreamer_;
	if (convertToStreamed)
	{
		pStreamer_ = new StreamedAnimation();
		pStreamer_->load( *pbf, totalTime_,
			StreamedAnimation::s_savingAnimCache_, uint32(-1) );

		StreamedAnimation::s_currentAnim_ = &*pStreamer_;

		InterpolatedAnimationChannel::s_saveConverter_ =
			&s_streamedAnimConverter;
		ChannelBinder * pCB = this->itinerantRoot();
		s_streamedAnimConverter.pIneligible_ = pCB ? &*pCB->channel() : NULL;
	}

	(*pbf) << totalTime_ << identifier_ << internalIdentifier_;

	const int numChannelBinders = static_cast<int>(channelBinders_.size());
	(*pbf) << numChannelBinders;
	for (int i = 0; i < numChannelBinders; ++i)
	{
		AnimationChannelPtr	pAC = channelBinders_[i].channel();
		(*pbf) << pAC->type();
		pAC->save( *pbf );
	}

	if (pStreamer_)
	{
		pStreamer_->save( *pbf );
	}

	if (!convertToStreamed || !(*pbf))
	{
		const bool result = !!(*pbf);
		bw_safe_delete(pbf);
		return result;
	}

	// Done with standard saving. Now save the data to streamed data cache.
	InterpolatedAnimationChannel::s_saveConverter_ = NULL;
	StreamedAnimation::s_currentAnim_ = NULL;

	StreamedDataCache* pCache = StreamedAnimation::s_savingAnimCache_;

	// Tell the cache about us.
	const uint32 preloadOffset = pCache->addEntryData(
		resourceID, StreamedDataCache::CACHE_VERSION, useModifiedTime );
	const void * preloadBuffer = pbf->data();
	MF_ASSERT( preloadBuffer != NULL );
	const uint32 preloadSize = static_cast<uint32>(pbf->cursorPosition());
	MF_ASSERT( preloadSize <= static_cast<uint32>(pbf->dataSize()) );
	pCache->immsave( preloadOffset, preloadSize, preloadBuffer );

	// save out our streamed data
	const uint32 streamedOffset = preloadOffset + preloadSize;
	const uint32 streamedSize = pStreamer_->dam( streamedOffset );

	// ok now get rid of the streamer; it was only for conversion
	pStreamer_ = pStreamerSave;

	// tell the cache we've finished writing
	pCache->endEntryData( preloadSize, streamedSize);

	// and finally get the cache to save out its directory
	pCache->saveSelf();

	bw_safe_delete(pbf);
	return true;
}


void Animation::translationOverrideAnim( AnimationPtr pBase, AnimationPtr pTranslationReference,
								const BW::vector< BW::string >& noOverrideChannels )
{
		BW_GUARD_PROFILER( Animation_translationOverrideAnim );
		this->channelBinders_.clear();
		this->pStreamer_ = pBase->pStreamer_;
		this->totalTime_ = pBase->totalTime_;
		this->identifier_ = pBase->identifier_;
		this->internalIdentifier_ = pBase->internalIdentifier_;


		ChannelBinderVector::iterator it = pBase->channelBinders_.begin();
		ChannelBinderVector::iterator end = pBase->channelBinders_.end();

		while (it != end )
		{
			bool override = false;
			Vector3 offset( 0, 0, 0 );
			AnimationChannelPtr pBaseChannel = it->channel();
			AnimationChannelPtr pTransChan = pTranslationReference->findChannel( it->channel()->identifier() );
			if ( pTransChan.hasObject() &&
				std::find( noOverrideChannels.begin(), noOverrideChannels.end(), pBaseChannel->identifier() ) == noOverrideChannels.end() )
			{
				override = true;
				offset = pTransChan->result( 0 )[3];
			}

			AnimationChannelPtr pNew = new TranslationOverrideChannel( offset, override, pBaseChannel, pBase );
			this->addChannelBinder( ChannelBinder( pNew, NULL ) );
			it++;
		}
}


} // namespace Moo

BW_END_NAMESPACE

// animation.cpp
