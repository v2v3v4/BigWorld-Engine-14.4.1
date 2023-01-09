#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif

namespace Moo
{
	INLINE
	const BW::string&	AnimationChannel::identifier( ) const
	{
		return identifier_;
	}

	INLINE
	void AnimationChannel::identifier( const BW::string& identifier )
	{
		identifier_ = identifier;
	}

}
/*animation_channel.ipp*/
