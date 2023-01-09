#include "pch.hpp"

#include "nodefull_model_animation.hpp"

#include "moo/discrete_animation_channel.hpp"
#include "moo/interpolated_animation_channel.hpp"
#include "resmgr/sanitise_helper.hpp"

#include "model.hpp"
#include "nodefull_model.hpp"


DECLARE_DEBUG_COMPONENT2( "Model", 0 )


BW_BEGIN_NAMESPACE

/**
 *	Constructor for NodefullModel's Animations
 */
NodefullModelAnimation::NodefullModelAnimation( NodefullModel & rOwner,
		Moo::AnimationPtr anim, DataSectionPtr pAnimSect ) :
	rOwner_( rOwner ),
	anim_( anim ),
	firstFrame_( pAnimSect->readInt( "firstFrame", 0 ) ),
	lastFrame_( pAnimSect->readInt( "lastFrame", -1 ) ),
	frameRate_( pAnimSect->readFloat( "frameRate", 30.f ) ),
	alphas_( NULL ),
	cognate_( pAnimSect->readString( "cognate", "" ) )
{
	BW_GUARD;
	// load and check standard stuff
	int rawFrames = int(anim->totalTime());
	if (firstFrame_ < 0) firstFrame_ = rawFrames + firstFrame_ + 1;
	if (lastFrame_ < 0) lastFrame_  = rawFrames + lastFrame_ + 1;
	firstFrame_ = Math::clamp( 0, firstFrame_, rawFrames );
	lastFrame_ = Math::clamp( 0, lastFrame_, rawFrames );

	// calculate number of frames and duration
	frameCount_ = abs( lastFrame_ - firstFrame_ );
	duration_ = float( frameCount_ ) / frameRate_;
	looped_ = (duration_ > 0.f);	// zero-length => not looped

	// Our animations now do node alpha stuff, because it makes
	// more sense for a node related thing to be in animations.
	// And only only ours do it because only ours have nodes.
	// This does mean that none of this stuff is inherited by overriding
	//  animations. But I think that's something we'll be able to live with.
	OrderedStringMap<float>	alphaNames;
	DataSectionPtr	aSection = pAnimSect->openSection( "alpha" );
	if (aSection)
	{
		for (DataSectionIterator iter = aSection->begin();
			iter != aSection->end();
			++iter)
		{
			DataSectionPtr pDS = *iter;

			// Need to unsanitise the section names
			BW::string id = SanitiseHelper::substringReplace
				(
					pDS->sectionName(),
					SanitiseHelper::SANITISING_TOKEN,
					SanitiseHelper::SPACE_TOKEN
				);

			alphaNames[id.c_str()] = pDS->asFloat( 0.f );
		}
	}

	// if there's no alphas then that's all
	if (alphaNames.size() == 0) return;

	// figure out the alphas from our alphanames
	alphas_ = new Moo::NodeAlphas();	// initialise to fully on
	alphas_->insert( alphas_->end(), anim_->nChannelBinders(), 1.f );

	// build a map of the nodes in this animation
	BW::map<Moo::Node*,int> chanNodes;
	for (uint i=0; i < anim_->nChannelBinders(); i++)
	{
		chanNodes.insert( std::make_pair(
			anim_->channelBinder( i ).node().getObject(), i ) );

		// Initialise the alpha values with the loaded alpha values
		BW::string channelID = 
			anim_->channelBinder( i ).channel()->identifier();

		OrderedStringMap<float>::iterator it = alphaNames.find( channelID );
		if (it != alphaNames.end())
			(*alphas_)[i] = it->second;
	}

	// now do a standard traversal of our nodes
	static struct AlphaTravInfo
	{
		float	alpha;
		int		pop;
	} stack[ NODE_STACK_SIZE ];
	int			sat = 0;

	stack[sat].alpha = 1.f;
	stack[sat].pop = (int)rOwner.nodeTree_.size();
	for (uint i = 0; i < rOwner.nodeTree_.size(); i++)
	{
		while (stack[sat].pop-- <= 0) --sat;

		NodeTreeData & ntd = rOwner.nodeTree_[i];

		// find the alpha of this node
		OrderedStringMap<float>::iterator aiter =
			alphaNames.find( ntd.pNode->identifier().c_str() );
		float nalpha = aiter == alphaNames.end() ?
			stack[sat].alpha : aiter->second;

		// set it in the alpha vector if the anim has this node
		BW::map<Moo::Node*,int>::iterator citer = chanNodes.find( ntd.pNode );
		if (citer != chanNodes.end()) (*alphas_)[ citer->second ] = nalpha;

		// and visit its children if it has any
		if (ntd.nChildren > 0)
		{
			++sat;

			if (sat >= NODE_STACK_SIZE)
			{
				--sat;
				ERROR_MSG( "Node Hierarchy too deep, unable to determine alpha values\n" );
				/* Set this equal to the end to prevent infinite loops 
				*/
				stack[sat].alpha = nalpha;
				stack[sat].pop = ntd.nChildren;
				return;
			}

			stack[sat].alpha = nalpha;
			stack[sat].pop = ntd.nChildren;
		}
	}
}


/**
 *	Destructor
 */
NodefullModelAnimation::~NodefullModelAnimation()
{
	BW_GUARD;
	bw_safe_delete(alphas_);
}


/**
 *	This function returns true if the NodefullModelAnimation is in a valid
 *	state.
 *	This function will not raise a critical error or assert if called but may
 *	commit ERROR_MSGs to the log.
 *
 *	@return		Returns true if the animation is in a valid state, otherwise
 *				false.
 */
bool NodefullModelAnimation::valid() const
{
	return true;
}


/**
 *	This method ticks the animation for the given time interval.
 */
void NodefullModelAnimation::tick( float dtime, float otime, float ntime )
{
	BW_GUARD;
	otime *= frameRate_;
	ntime *= frameRate_;
	if (firstFrame_ <= lastFrame_)
	{
		otime = firstFrame_ + otime;
		ntime = firstFrame_ + ntime;
	}
	else
	{
		otime = firstFrame_ - otime;
		ntime = firstFrame_ - ntime;
	}

	anim_->tick( dtime, otime, ntime, float(firstFrame_), float(lastFrame_) );
}


/**
 *	This method applies the animation with the given parameters
 *	to the global node table. If flags are positive, then the first
 *	bit indicates whether or not movement should be taken out of the
 *	animation, and the second bit indicates whether or not it should
 *	be played as a coordinated animation.
*/
void NodefullModelAnimation::play( float time, float blendRatio, int flags )
{
	BW_GUARD;
	Moo::ChannelBinder * pIR = NULL;

	float frame = time * frameRate_;
	frame = firstFrame_ <= lastFrame_ ?
		firstFrame_ + frame : firstFrame_ - frame;

	if (flags > 0)
	{
		flags &= 7;
		pIR = anim_->itinerantRoot();
		if (!alternateItinerants_[flags]) this->precalcItinerant( flags );
		pIR->channel( alternateItinerants_[flags] );
	}

	anim_->animate( Model::blendCookie(), frame, blendRatio, alphas_ );

	if (flags > 0)
	{
		pIR->channel( alternateItinerants_[0] );
	}
}


/**
 *	This method precalculates the appropriate itinerant channel
 */
void NodefullModelAnimation::precalcItinerant( int flags )
{
	BW_GUARD;
	if (!flags) return;

	Moo::ChannelBinder * pIR = anim_->itinerantRoot();

	alternateItinerants_[0] = pIR->channel();

	Moo::AnimationChannel * pNewAC = NULL;

	// make a copy of the current channel
	pNewAC = pIR->channel()->duplicate();

	// apply the changes indicated by 'flags' to it
	if (pNewAC != NULL)
	{
		if (flags & 1)    // isMovement
        {
            Matrix first; pIR->channel()->result( float(firstFrame_), first );
			Matrix final; pIR->channel()->result( float(lastFrame_), final );

			final.invert();
			flagFactors_[0].multiply( final, first );

			SmartPointer<Moo::InterpolatedAnimationChannel> motion =
				new Moo::InterpolatedAnimationChannel();

			if ( firstFrame_ != 0 )
				motion->addKey( 0.0f, Matrix::identity );

			motion->addKey( float(firstFrame_), Matrix::identity );
			motion->addKey( float(lastFrame_),  flagFactors_[0] );

			motion->finalise();

			pNewAC->postCombine( *motion );
		}

		if (flags & 2)	// isCoordinated
		{
			// ok, what's the normal transform for this node
			//Matrix	originT = pIR->node()->transform();
			int cogIdx = rOwner_.getAnimation( cognate_ );
			ModelAnimation * pCog = cogIdx < 0 ?
				rOwner_.lookupLocalDefaultAnimation() :
				rOwner_.lookupLocalAnimation( cogIdx );
			Matrix originT;
			pCog->flagFactor( -1, originT );

//			Vector3 oTo = originT.applyToOrigin();
//			Quaternion oTq = Quaternion( originT );
//			dprintf( "desired translation is (%f,%f,%f) q (%f,%f,%f,%f)\n",
//				oTo[0], oTo[1], oTo[2], oTq[0], oTq[1], oTq[2], oTq[3] );

			// and what's the transform used in this animation
			Matrix	offsetT; pIR->channel()->result( 0, offsetT );

			// find the transform by which offsetT should be
			// postmultiplied in order to get back to originT
			offsetT.invert();
			flagFactors_[1].multiply( offsetT, originT );

			SmartPointer<Moo::DiscreteAnimationChannel> coord =
				new Moo::DiscreteAnimationChannel();
			coord->addKey( 0, flagFactors_[1] );
			coord->finalise();

			pNewAC->postCombine( *coord );
		}

		if (flags & 4)	// isImpacting
		{
			// we save the current animation channel (pNewAC) in flagFactor2,
			// with the first frame transform removed from it, then we set
			// pNewAC to be just the first frame transform the whole time.

			Matrix firstT; pNewAC->result( 0, firstT );
			Matrix firstTI; firstTI.invert( firstT );

			flagFactor2_ = pNewAC;
			SmartPointer<Moo::DiscreteAnimationChannel> smooth =
				new Moo::DiscreteAnimationChannel();
			smooth->addKey( 0, firstTI );
			smooth->finalise();
			flagFactor2_->preCombine( *smooth );

			Moo::DiscreteAnimationChannel * pNewACDerived =
				new Moo::DiscreteAnimationChannel();
			pNewACDerived->addKey( 0, firstT );
			pNewACDerived->finalise();
			pNewAC = pNewACDerived;

			flagFactors_[2] = Matrix::identity;
		}
	}
	else
	{
		ERROR_MSG( "NodefullModelAnimation::precalcItinerant:"
			"Could not duplicate animation channel of type %d\n",
			pIR->channel()->type() );

		pNewAC = pIR->channel().getObject();
	}

	// and store it for later switching
	alternateItinerants_[flags] = pNewAC;
}


/**
 *	Get the factor matrix for the given flags.
 */
void NodefullModelAnimation::flagFactor( int flags, Matrix & mOut ) const
{
	BW_GUARD;
	float param = mOut[0][0];

	mOut.setIdentity();
	if (flags < 0)
	{
		if (flags == -1)
		{
			anim_->itinerantRoot()->channel()->result(
				float(firstFrame_), mOut );
		}
		return;
	}

	if (flags & (1<<0))
	{
		mOut.preMultiply( this->NodefullModelAnimation::flagFactorBit(0) );
	}
	if (flags & (1<<1))
	{
		mOut.preMultiply( this->NodefullModelAnimation::flagFactorBit(1) );
	}
	if (flags & (1<<2))
	{
		this->NodefullModelAnimation::flagFactorBit(2);
		mOut.preMultiply(
			flagFactor2_->result( firstFrame_ + frameRate_ * param ) );
	}
}


/**
 *	Get the given flag factor part
 */
const Matrix & NodefullModelAnimation::flagFactorBit( int bit ) const
{
	BW_GUARD;
	if (!alternateItinerants_[ (int)(1<<bit) ])
		const_cast<NodefullModelAnimation*>(this)->precalcItinerant( 1<<bit );
	return flagFactors_[ bit ];
}


/**
 *	Get the amount of memory used by this animation
 */
uint32 NodefullModelAnimation::sizeInBytes() const
{
	BW_GUARD;
	uint32 sz = this->ModelAnimation::sizeInBytes();
	sz += sizeof(*this) - sizeof(ModelAnimation);

	// TODO: restore this
	//if (alphas_ != NULL)
	//	sz += contentSize( *alphas_ );
	sz += (uint32)cognate_.length();
	return sz;
}

BW_END_NAMESPACE


// nodefull_model_animation.cpp
