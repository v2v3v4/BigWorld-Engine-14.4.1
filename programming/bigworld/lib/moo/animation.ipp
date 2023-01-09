// animation.ipp

#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif



namespace Moo {


	INLINE
	void Animation::totalTime( float time )
	{
		totalTime_ = time;
	}

	INLINE
	float Animation::totalTime( ) const
	{
		return totalTime_;
	}

	INLINE
	const BW::string Animation::identifier( ) const
	{
		return identifier_;
	}

	INLINE
	void Animation::identifier( const BW::string& identifier )
	{
		identifier_ = identifier;
	}

	INLINE
	const BW::string Animation::internalIdentifier( ) const
	{
		return internalIdentifier_;
	}


	INLINE
	void Animation::internalIdentifier( const BW::string& ii )
	{
		internalIdentifier_ = ii;
	}


	INLINE
	void Animation::animate( const BlendedAnimations & alist )
	{
		animate( alist.begin(), alist.end() );
	}

};

// animation.ipp
