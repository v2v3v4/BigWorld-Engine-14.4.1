#include "pch.hpp"

#include "animation_channel.hpp"

#include "cstdmf/binaryfile.hpp"

DECLARE_DEBUG_COMPONENT2( "Moo", 0 )


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "animation_channel.ipp"
#endif

namespace Moo
{

/**
 *	Constructor
 */
AnimationChannel::AnimationChannel()
{
}

/**
 *	Copy constructor
 */
AnimationChannel::AnimationChannel( const AnimationChannel & other )
{
	identifier_ = other.identifier_;
}

/**
 *	Destructor
 */
AnimationChannel::~AnimationChannel()
{
}


bool AnimationChannel::load( BinaryFile & bf )
{
	bf >> identifier_;
	return !!bf;
}

bool AnimationChannel::save( BinaryFile & bf ) const
{
	bf << identifier_;
	return !!bf;
}


/**
 *	This static method constructs a new channel of the given type.
 */
AnimationChannel * AnimationChannel::construct( int type )
{
	BW_GUARD;
	ChannelTypes::iterator found = s_channelTypes_->find( type );
	if (found != s_channelTypes_->end())
		return (*found->second)();

	ERROR_MSG( "AnimationChannel::construct: "
		"Channel type %d is unknown.\n", type );
	return NULL;
}

/**
 *	Static method to register a channel type
 */
void AnimationChannel::registerChannelType( int type, Constructor cons )
{
	BW_GUARD;
	if (s_channelTypes_ == NULL) s_channelTypes_ = new ChannelTypes();
	(*s_channelTypes_)[ type ] = cons;
	INFO_MSG( "AnimationChannel: registered type %d\n", type );
}

/**
 *	This interface doesn't have an operator== and neither do most subclasses.
 *	Subclasses that do implement one can use this function to check
 *	base-class members for equivalence.
 *	
 *	@return True if this is equivalent to @a other.
 */
bool AnimationChannel::compareChannelBase(
	const AnimationChannel & other ) const
{
	return identifier() == other.identifier();
}

/*static*/ void AnimationChannel::fini()
{
	BW_GUARD;
	bw_safe_delete(s_channelTypes_);
}

/// static initialiser
AnimationChannel::ChannelTypes * AnimationChannel::s_channelTypes_ = NULL;

extern int CueChannel_token;
static int s_channelTokens = CueChannel_token;

} // namespace Moo

BW_END_NAMESPACE

// animation_channel.cpp
