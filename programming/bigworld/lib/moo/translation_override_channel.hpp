#ifndef TRANSLATION_OVERRIDE_CHANNEL_HPP
#define TRANSLATION_OVERRIDE_CHANNEL_HPP

#include "animation.hpp"


BW_BEGIN_NAMESPACE

namespace Moo
{

/**
 *	This class implements an animation channel that just
 *	updates the translation of the base animation.
 *
 *	For example you can use this to fix up one set of
 *	animations for use on a different skeleton that has
 *	the same bones, but different bone lengths.
 */
class TranslationOverrideChannel : public AnimationChannel
{
public:
	TranslationOverrideChannel( const Vector3& translation, bool override, AnimationChannelPtr pBaseChannel, AnimationPtr pBaseAnim );
	TranslationOverrideChannel();
	~TranslationOverrideChannel();

	virtual	Matrix		result( float time ) const;
	virtual void		result( float time, Matrix& out ) const;
	virtual void		result( float time, BlendTransform& out ) const;
	virtual void		nodeless(  float time, float blendRatio ) const 
	{
		pBaseChannel_->nodeless( time, blendRatio ); 
	}


	virtual bool		load( BinaryFile & bf );
	virtual bool		save( BinaryFile & bf ) const;

	virtual void		preCombine ( const AnimationChannel & rOther ) {};
	virtual void		postCombine( const AnimationChannel & rOther ) {};

	virtual int			type() const { return 7; };

	const Vector3&		translation( ) const { return translation_; }
	void				translation( Vector3& v ) { translation_ = v; }

	bool				override() const { return override_; }
	void				override( bool state ) { override_ = state; }

	AnimationChannelPtr	pBaseChannel() const { return pBaseChannel_; }
	void				pBaseChannel( AnimationChannelPtr pChannel ) { pBaseChannel_ = pChannel; this->identifier( pBaseChannel_->identifier() ); }

	AnimationPtr		pBaseAnim() const { return pBaseAnim_; }
	void				pBaseAnim( AnimationPtr pAnim ) { pBaseAnim_ = pAnim; }

	virtual AnimationChannel * duplicate() const
		{ 
			if (!override_)
				return pBaseChannel_->duplicate();
			return new TranslationOverrideChannel( *this ); 
		}

private:
//	TranslationOverrideChannel( const TranslationOverrideChannel& );

	Vector3				translation_;
	bool				override_;
	AnimationChannelPtr	pBaseChannel_;
	AnimationPtr		pBaseAnim_;

	static AnimationChannel* New()
		{ 
			return new TranslationOverrideChannel(); 
		}

	static TypeRegisterer s_rego_;
};

} // namespace Moo

BW_END_NAMESPACE

#endif // TRANSLATION_OVERRIDE_CHANNEL_HPP
