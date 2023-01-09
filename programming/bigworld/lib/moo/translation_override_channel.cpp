#include "pch.hpp"
#include "translation_override_channel.hpp"
#include "animation_manager.hpp"
#include "cstdmf/binaryfile.hpp"


DECLARE_DEBUG_COMPONENT2( "Moo", 0 );


BW_BEGIN_NAMESPACE

namespace Moo
{

// -----------------------------------------------------------------------------
// Section: TranslationOverrideChannel
// -----------------------------------------------------------------------------


TranslationOverrideChannel::TranslationOverrideChannel( const Vector3& translation, bool override, AnimationChannelPtr pBaseChannel, AnimationPtr pBaseAnim )
: translation_( translation ),
  override_( override ),
  pBaseChannel_( pBaseChannel ),
  pBaseAnim_( pBaseAnim )
{
	BW_GUARD;
	MF_ASSERT_DEV( pBaseChannel_.hasObject() );
	if( pBaseChannel_.hasObject() )
		this->identifier( pBaseChannel_->identifier() );
}

/**
 *	Constructor.
 */
TranslationOverrideChannel::TranslationOverrideChannel()
:	translation_( 0, 0, 0 ),
	override_( false )
{
}


/**
 *	Destructor.
 */
TranslationOverrideChannel::~TranslationOverrideChannel()
{

}

Matrix TranslationOverrideChannel::result( float time ) const
{
	BW_GUARD;
	MF_ASSERT_DEV( pBaseChannel_.hasObject() );

	Matrix m;

	if( pBaseChannel_.hasObject() )
		pBaseChannel_->result( time, m );
	else
		m.setIdentity();

	if (override_)
		m.translation( translation_ );
	return m;
}

void TranslationOverrideChannel::result( float time, Matrix& out ) const
{
	BW_GUARD;
	MF_ASSERT_DEV( pBaseChannel_.hasObject() );

	if( pBaseChannel_.hasObject() )
		pBaseChannel_->result( time, out );
	else
		out.setIdentity();

	if (override_)
		out.translation( translation_ );
}

void TranslationOverrideChannel::result( float time, BlendTransform& out ) const
{
	BW_GUARD;
	MF_ASSERT_DEV( pBaseChannel_.hasObject() );

	if( pBaseChannel_.hasObject() )
		pBaseChannel_->result( time, out );

	if (override_)
		out.translation(translation_);
}

bool TranslationOverrideChannel::load( BinaryFile & bf )
{
	BW_GUARD;
	if( !this->AnimationChannel::load( bf ) )
		return false;

	bool ret = false;
	BW::string animResource;
	bf >> animResource;

	this->pBaseAnim_ = AnimationManager::instance().find( animResource );
	if (this->pBaseAnim_.hasObject())
	{
		this->pBaseChannel_ = NULL;
		for (uint32 i = 0; i < this->pBaseAnim_->nChannelBinders(); i++)
		{
			const ChannelBinder& cb = pBaseAnim_->channelBinder( i );
			if (cb.channel()->identifier() == this->identifier())
			{
				this->pBaseChannel_ = cb.channel();
				i = this->pBaseAnim_->nChannelBinders();
				ret = true;
			}
		}
	}

	bf >> this->translation_;
	bf >> this->override_;

	return ret && bf;
}

bool TranslationOverrideChannel::save( BinaryFile & bf ) const
{
	BW_GUARD;
	if( !this->AnimationChannel::save( bf ) )
		return false;
	BW::string animResource = AnimationManager::instance().resourceID( this->pBaseAnim_.getObject() );
	bf << animResource;
	bf << this->translation_;
	bf << this->override_;

	return !!bf;
}


TranslationOverrideChannel::TypeRegisterer
	TranslationOverrideChannel::s_rego_( TRANSLATION_OVERRIDE_CHANNEL, New );

} // namespace Moo

BW_END_NAMESPACE

// translation_override_channel.cpp
