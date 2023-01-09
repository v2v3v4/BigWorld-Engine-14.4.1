#ifndef ANIMATION_CHANNEL_HPP
#define ANIMATION_CHANNEL_HPP

#include <iostream>
#include "cstdmf/smartpointer.hpp"
#include "cstdmf/bw_map.hpp"
#include "node.hpp"
#include "math/blend_transform.hpp"


BW_BEGIN_NAMESPACE

class BinaryFile;


namespace Moo
{
class AnimationChannel;
typedef SmartPointer< AnimationChannel > AnimationChannelPtr;

/**
 *	This is the base class for channels in an Animation.
 *	Each channel controls the animation of the transform for a particular Node.
 */
class AnimationChannel : public SafeReferenceCount
{
public:
	enum AnimationaChannelType
	{
		DISCRETE_ANIMATION_CHANNEL = 0,
		INTERPOLATED_ANIMATION_CHANNEL_COMPRESSION_ON_EXPLICIT_COMPRESSION_FACTORS_OFF = 1,
		MORPH_ANIMATION_CHANNEL_NOT_SUPPORTED = 2, // NO LONGER SUPPORTED
		INTERPOLATED_ANIMATION_CHANNEL_COMPRESSION_OFF = 3,
		INTERPOLATED_ANIMATION_CHANNEL_COMPRESSION_ON_EXPLICIT_COMPRESSION_FACTORS_ON = 4,
		STREAMED_ANIMATION_CHANNEL = 5,
		CUE_CHANNEL = 6,
		TRANSLATION_OVERRIDE_CHANNEL = 7,
		NUM_ANIMATION_CHANNELS
	};

	AnimationChannel();
	AnimationChannel( const AnimationChannel & other );
	~AnimationChannel();

	static void fini();

	virtual	Matrix		result( float time ) const
		{ return Matrix::identity; }
	virtual void		result( float time, Matrix& out ) const
		{ out = Matrix::identity; }
	virtual void		result( float time, BlendTransform& out ) const
		{ Matrix m; this->result( time, m ); out.init( m ); }

	virtual void		nodeless( float time, float blendRatio ) const { }

	virtual bool		wantTick() const	{ return false; }
	virtual void		tick( float dtime, float otime, float ntime,
							float btime, float etime ) const { }

	const BW::string&	identifier( ) const;
	void				identifier( const BW::string& identifier );

	virtual bool		load( BinaryFile & bf );
	virtual bool		save( BinaryFile & bf ) const;

	virtual void		preCombine ( const AnimationChannel & rOther ) = 0;
	virtual void		postCombine( const AnimationChannel & rOther ) = 0;

	virtual int			type() const = 0;

	static AnimationChannel * construct( int type );

	virtual AnimationChannel * duplicate() const = 0;

protected:
	typedef AnimationChannel * (*Constructor)();
	static void registerChannelType( int type, Constructor cons );

	/**
	 *	Helper class to register animation channel types
	 */
	class TypeRegisterer
	{
	public:
		TypeRegisterer( int type, Constructor cons )
			{ AnimationChannel::registerChannelType( type, cons ); }
	};

	bool compareChannelBase( const AnimationChannel & other ) const;

private:
	AnimationChannel& operator=(const AnimationChannel&);

	BW::string		identifier_;

	typedef BW::map< int, Constructor > ChannelTypes;
	static ChannelTypes		* s_channelTypes_;
};

typedef BW::vector< SmartPointer<AnimationChannel> > AnimationChannelVector;


/**
 *	This class binds an AnimationChannel to a Node. This allows the same
 *	AnimationChannel to be reused to control multiple node hierarchies
 *	with compatible structures, since only the ChannelBinders need be
 *	duplicated, not the whole AnimationChannel.
 */
class ChannelBinder 
{
private:
	AnimationChannelPtr			channel_;
	NodePtr						node_;

public:

	ChannelBinder( AnimationChannelPtr channel, NodePtr node )
	: channel_( channel )
	, node_( node )
	{
	}

	~ChannelBinder( )
	{
	}

	inline void animate( float time ) const
	{
		if (!channel_) return;
		if (node_)
		{
			channel_->result( time, node_->transform() );
		}
		else
		{
			channel_->nodeless( time, 1.f );
		}
	}

	const AnimationChannelPtr&	channel( ) const
	{
		return channel_;
	}

	void channel( AnimationChannelPtr channel )
	{
		channel_ = channel;
	}

	const NodePtr& node( ) const
	{
		return node_;
	}

	void node( NodePtr node )
	{
		node_ = node;
	}

};

typedef BW::vector< ChannelBinder > ChannelBinderVector;


}

#ifdef CODE_INLINE
#include "animation_channel.ipp"
#endif

BW_END_NAMESPACE

#endif // ANIMATION_CHANNEL_HPP
