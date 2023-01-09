#ifndef INTERPOLATED_ANIMATION_CHANNEL_HPP
#define INTERPOLATED_ANIMATION_CHANNEL_HPP

#include <iostream>
#include "animation_channel.hpp"
#include "cstdmf/concurrency.hpp"


BW_BEGIN_NAMESPACE

namespace Moo
{

/**
 *	An animation channel with interpolation between the keys.
 */
class InterpolatedAnimationChannel : public AnimationChannel
{
public:

	typedef BW::vector< std::pair< float, Vector3 > > ScaleKeyframes;
	typedef BW::vector< std::pair< float, Vector3 > > PositionKeyframes;
	typedef BW::vector< std::pair< float, Quaternion > > RotationKeyframes;

	typedef uint32 IndexType;
	/**
	 *	An index vector helper class.
	 */
	class IndexVector : public BW::vector< IndexType >
	{
	public:
		IndexType upper_bound( int card ) const
		{
			if (card < 0) return 0;
			if (card >= (signed)this->size()) return this->back();
			return this->operator[]( card );
		}
		IndexVector& operator = ( const BW::vector<IndexType>& input )
		{
			this->resize( input.size() );
			for (uint32 i = 0; i < input.size(); i++ )
			{
				(*this)[i] = value_type( input[i] );
			}
			return *this;
		}
	};

	InterpolatedAnimationChannel( bool canCompress = true, bool readCompressionFactors = false );
	InterpolatedAnimationChannel( const InterpolatedAnimationChannel & other );
	~InterpolatedAnimationChannel();

	virtual bool compareChannel( const InterpolatedAnimationChannel & rhs ) const;

	bool				valid() const;

	virtual Matrix		result( float time ) const;
	virtual void		result( float time, Matrix& out ) const;
	virtual void		result( float time, BlendTransform& out ) const;

	void				addKey( float time, const Matrix& key );
	void				addScaleKey( float time, const Vector3& key );
	void				addPositionKey( float time, const Vector3& key );
	void				addRotationKey( float time, const Quaternion& key );

	void				reduceKeyFrames( float cosAngleError, float scaleError, float positionError );
	/* Factors specify the amount of frames to keep: 1 = all, 0 = none */
	void				reduceKeyFramesTo( float rotationFactor, float scaleFactor, float positionFactor );

	void				reduceRotationKeys( float cosAngleError, Moo::NodePtr node = NULL );
	void				reduceScaleKeys( float scaleError );
	void				reducePositionKeys( float positionError );

	void				finalise();

	virtual void		preCombine ( const AnimationChannel & rOther );
	virtual void		postCombine( const AnimationChannel & rOther );

	virtual bool		load( BinaryFile & bf );
	virtual bool		save( BinaryFile & bf ) const;

	virtual int			type() const;

	virtual AnimationChannel * duplicate() const
		{ return new InterpolatedAnimationChannel( *this ); }

	uint				nScaleKeys() const
	{
		MF_ASSERT( scaleKeys_.size() <= UINT_MAX );
		return ( uint ) scaleKeys_.size();
	}

	uint				nPositionKeys() const
	{
		MF_ASSERT( positionKeys_.size() <= UINT_MAX );
		return ( uint ) positionKeys_.size();
	}

	uint				nRotationKeys() const
	{
		MF_ASSERT( rotationKeys_.size() <= UINT_MAX );
		return ( uint ) rotationKeys_.size();
	}

	float				scaleCompressionError() const { return scaleCompressionError_; }
	void				scaleCompressionError( float f ) { scaleCompressionError_ = f; }
	float				positionCompressionError() const { return positionCompressionError_; }
	void				positionCompressionError( float f ) { positionCompressionError_ = f; }
	float				rotationCompressionError() const { return rotationCompressionError_; }
	void				rotationCompressionError( float f ) { rotationCompressionError_ = f; }

	static void			inhibitCompression( bool b ) { inhibitCompression_ = b; }

	/**
	 * An estimate of the disk size for the channel, used by modelviewer for
	 * previewing
	 */
	int					size() const;

	/**
	 *	This inner class lets interpolated animation channels be converted
	 *	on save to different channels of different (unspecified) types.
	 */
	class Converter
	{
	public:
		virtual int type() = 0;
		virtual bool eligible( const InterpolatedAnimationChannel & iac ) = 0;
		virtual bool saveAs( BinaryFile & bf,
			const InterpolatedAnimationChannel & iac ) = 0;
	};

	static THREADLOCAL( Converter* ) s_saveConverter_;

	friend class StreamedAnimationChannel;

private:
	ScaleKeyframes		scaleKeys_;
	PositionKeyframes	positionKeys_;
	RotationKeyframes	rotationKeys_;

	IndexVector			scaleIndex_;
	IndexVector			positionIndex_;
	IndexVector			rotationIndex_;

	/* These indices are used to look up the 'upper_bound' key in
		their corresponding vector at a given time */

	bool				readCompressionInfo_;

	float				scaleCompressionError_;
	float				positionCompressionError_;
	float				rotationCompressionError_;

	/* Defaults to true, meaning animations won't be compressed on load */
	static bool			inhibitCompression_;

	InterpolatedAnimationChannel& operator=(const InterpolatedAnimationChannel&);

	friend std::ostream& operator<<(std::ostream&, const InterpolatedAnimationChannel&);

	static AnimationChannel * New1()
		{ return new InterpolatedAnimationChannel(); }
	static AnimationChannel * New3()
		{ return new InterpolatedAnimationChannel( false ); }
	static AnimationChannel * New4()
		{ return new InterpolatedAnimationChannel( true, true ); }
	static TypeRegisterer s_rego1_, s_rego3_, s_rego4_;
};

}

#ifdef CODE_INLINE
#include "interpolated_animation_channel.ipp"
#endif

BW_END_NAMESPACE


#endif
/*interpolated_animation_channel.hpp*/
