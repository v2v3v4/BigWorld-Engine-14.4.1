#include "pch.hpp"

#include "interpolated_animation_channel.hpp"

#include "cstdmf/binaryfile.hpp"


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "interpolated_animation_channel.ipp"
#endif

namespace Moo
{

/**
 *	Constructor
 */
InterpolatedAnimationChannel::InterpolatedAnimationChannel( bool canCompress, bool readCompressionInfo )
	: readCompressionInfo_( readCompressionInfo )
{
}

/**
 *	Copy constructor
 */
InterpolatedAnimationChannel::InterpolatedAnimationChannel( const InterpolatedAnimationChannel & other ) :
	AnimationChannel( other ),
	scaleKeys_( other.scaleKeys_ ),
	positionKeys_( other.positionKeys_ ),
	rotationKeys_( other.rotationKeys_ ),
	scaleIndex_( other.scaleIndex_ ),
	positionIndex_( other.positionIndex_ ),
	rotationIndex_( other.rotationIndex_ ),
	readCompressionInfo_( other.readCompressionInfo_ ),
	scaleCompressionError_( other.scaleCompressionError_ ),
	positionCompressionError_( other.positionCompressionError_ ),
	rotationCompressionError_( other.rotationCompressionError_ )
{
}

/**
 *	Destructor
 */
InterpolatedAnimationChannel::~InterpolatedAnimationChannel()
{
}


/**
 *	@return True if this is equivalent to @a other.
 */
bool InterpolatedAnimationChannel::compareChannel(
	const InterpolatedAnimationChannel & rhs ) const
{
	BW_GUARD;

	return compareChannelBase(rhs)
		&&
		(scaleKeys_ == rhs.scaleKeys_)
		&&
		(positionKeys_ == rhs.positionKeys_)
		&&
		(rotationKeys_ == rhs.rotationKeys_)
		&&
		(scaleIndex_ == rhs.scaleIndex_)
		&&
		(positionIndex_ == rhs.positionIndex_)
		&&
		(rotationIndex_ == rhs.rotationIndex_)
		&&
		(readCompressionInfo_ == rhs.readCompressionInfo_)
		&&
		(scaleCompressionError_ == rhs.scaleCompressionError_)
		&&
		(positionCompressionError_ == rhs.positionCompressionError_)
		&&
		(rotationCompressionError_ == rhs.rotationCompressionError_);
}


template<class KeyFrame>
struct KeyFrameLessThanFunctor
{
	bool operator()( const KeyFrame & a, const KeyFrame & b )
	{
		return a.first < b.first;
	}
};


template< class KeyVal, class Interpolation >
inline void interpolateKeys(
	const BW::vector< std::pair< float, KeyVal > >& keyFrames,
	const InterpolatedAnimationChannel::IndexVector & indexs,
	Interpolation interpolate,
	float time,
	KeyVal& out )
{
	BW_GUARD;
	IF_NOT_MF_ASSERT_DEV(!keyFrames.empty())
	{
		return;
	}


	typedef	std::pair< float, KeyVal >						KeyFrame;
	typedef typename BW::vector<KeyFrame>::const_iterator	KeyIterator;


	KeyIterator upperBound = std::upper_bound(	keyFrames.begin(),
												keyFrames.end(),
												KeyFrame(time,KeyVal()),
												KeyFrameLessThanFunctor<KeyFrame>());

	if (upperBound == keyFrames.begin())
	{
		out = keyFrames.front().second;
	}
	else if (upperBound == keyFrames.end())
	{
		out = keyFrames.back().second;
	}
	else
	{
		KeyIterator previouseKeyFrame = upperBound - 1;

		float time1 = previouseKeyFrame->first;
		float time2 = upperBound->first;
		float t = ( time - time1 ) / ( time2 - time1 );
		interpolate( &out, &previouseKeyFrame->second, &upperBound->second, t );
	}
}



inline void buildTransform( const Quaternion& rot, const Vector3& scale, const Vector3& pos, Matrix& out )
{
	Vector3* row0 = reinterpret_cast<Vector3*>( out.m[0] );
	Vector3* row1 = reinterpret_cast<Vector3*>( out.m[1] );
	Vector3* row2 = reinterpret_cast<Vector3*>( out.m[2] );
	Vector3* row3 = reinterpret_cast<Vector3*>( out.m[3] );

	D3DXMatrixRotationQuaternion( &out, &rot );
	*row0 *= scale.x;
	*row1 *= scale.y;
	*row2 *= scale.z;
	*row3 = pos;
}

Matrix	InterpolatedAnimationChannel::result( float time ) const
{
	Matrix m;
	this->result( time, m );
	return m;
}

void InterpolatedAnimationChannel::result( float time, Matrix& out ) const
{
	Quaternion rot;
	interpolateKeys( rotationKeys_, rotationIndex_, D3DXQuaternionSlerp, time, rot );

	Vector3 scale;
	interpolateKeys( scaleKeys_, scaleIndex_, D3DXVec3Lerp, time, scale );

	Vector3 pos;
	interpolateKeys( positionKeys_, positionIndex_, D3DXVec3Lerp, time, pos );

	buildTransform( rot, scale, pos, out );
}


void InterpolatedAnimationChannel::result( float time, BlendTransform& out ) const
{
	Quaternion rot;
	interpolateKeys( rotationKeys_, rotationIndex_, D3DXQuaternionSlerp, time, rot );

	Vector3 scale;
	interpolateKeys( scaleKeys_, scaleIndex_, D3DXVec3Lerp, time, scale );

	Vector3 pos;
	interpolateKeys( positionKeys_, positionIndex_, D3DXVec3Lerp, time, pos );

	out = BlendTransform( rot, scale, pos );
}


/**
 *	Key comparison helper class. Compares the first element of each pair.
 */
template <class X, class Y>
class CompareFirstOfPair
{
	public:
		bool operator ()( const ::std::pair<X,Y> & pa, const ::std::pair<X,Y> & pb )
		{
			return pa.first < pb.first;
		}
};

template <class VType>
void buildIndex( BW::vector< std::pair< float, VType > > & vector,
	InterpolatedAnimationChannel::IndexVector & index )
{
	BW_GUARD;
	index.clear();

	// get out now if there's nothing left
	if (vector.empty())
	{
		return;
	}

	// OK, the index contains what the upper_bound function
	// on a map would return for each integer time.

	// First of all sort the vector
	std::sort( vector.begin(), vector.end(), CompareFirstOfPair<float,VType>() );

	// Now for each integer time between 0 and the time at the back
	// of the vector, put into the Index the nearest index into the
	// vector whose time is strictly greater than the integer time
	typename BW::vector< std::pair< float, VType > >::iterator it =
		vector.begin();
	MF_ASSERT( vector.size() <= UINT_MAX );
	for (uint i = 0; i <= (vector.back().first + 1); i++)
	{
		while (it != vector.end() && it->first <= i)
			it++;

		index.push_back( ( uint ) ( it - vector.begin() ) );
	}

	// The last value in our Index will actually be the index of
	// the end of the input vector.
}

void InterpolatedAnimationChannel::addKey( float time, const Matrix& key )
{
	BW_GUARD;
	Matrix m = key;
	Vector3* row0 = reinterpret_cast<Vector3*>( m.m[0] );
	Vector3* row1 = reinterpret_cast<Vector3*>( m.m[1] );
	Vector3* row2 = reinterpret_cast<Vector3*>( m.m[2] );
	Vector3* row3 = reinterpret_cast<Vector3*>( m.m[3] );

	Vector3 in;

	float scale0 = D3DXVec3Length( row0 );
	float scale1 = D3DXVec3Length( row1 );
	float scale2 = D3DXVec3Length( row2 );

	*row0 *= 1.f / scale0;
	*row1 *= 1.f / scale1;
	*row2 *= 1.f / scale2;

	D3DXVec3Cross( &in, row0, row1 );
	if( D3DXVec3Dot( &in, row2 ) < 0 )
	{
		*row2 *= -1;
		scale2 *= -1;
	}

	this->addScaleKey( time, Vector3( scale0, scale1, scale2 ) );
	this->addPositionKey( time, *row3 );
	Quaternion q;
	D3DXQuaternionRotationMatrix( &q, &m );
	this->addRotationKey( time, q );
}


void InterpolatedAnimationChannel::addScaleKey( float time, const Vector3& key )
{
	scaleKeys_.push_back( std::make_pair( time, key ) );
	buildIndex( scaleKeys_, scaleIndex_ );
}

void InterpolatedAnimationChannel::addPositionKey( float time, const Vector3& key )
{
	positionKeys_.push_back( std::make_pair( time, key ) );
	buildIndex( positionKeys_, positionIndex_ );
}

void InterpolatedAnimationChannel::addRotationKey( float time, const Quaternion& key )
{
	rotationKeys_.push_back( std::make_pair( time, key ) );
	buildIndex( rotationKeys_, rotationIndex_ );
}




template<class MapType>
void cleanKeys( BW::vector< std::pair<float,MapType> >& theMap )
{
	BW_GUARD;
	// Iterate through the entire mapping, removing runs of duplicate keys.
	// (leaving a key at the start and end of the sequence)
	typedef BW::vector<std::pair<float,MapType > > MAP_TYPE;
	if ( theMap.size() > 1 )
	{
		typename MAP_TYPE::iterator it = theMap.begin();

		bool finished = false;

		int i = 0;
		typename MAP_TYPE::iterator first = it;
		while ( !finished )
		{
			it = theMap.begin() + i;
			typename MAP_TYPE::iterator delit = it++;

			if ( it != theMap.end() )
			{
				if ( delit->second == it->second )
				{
					if ( delit != first )				
						theMap.erase( delit );
					else
						i++;
				}
				else
				{
					first = it;
					i++;
				}
			}
			else
			{
				finished = true;
			}
		}

	}

	// Only really should remove the end marker key if they're the only ones in the file...
	if ( theMap.size() == 2)
	//if ( theMap.size() > 1 )
	{
		typename MAP_TYPE::iterator it = theMap.end();
		it--;
		bool finished = false;

		while ( !finished )
		{
			typename MAP_TYPE::iterator delit = it--;
			if ( delit->second == it->second )
				theMap.erase( delit );
			if ( it == theMap.begin() )
				finished = true;
		}

	}
}

// determines the depth of the nodes hierachy ( recursive )
int countDepth(Moo::NodePtr node)
{
	BW_GUARD;
	if (node==NULL)
		return 0;

	int depth=0;
	if (node->nChildren() == 0)
		depth=0;
	else
	{
		depth=1;		
		int childrenDepth=0;
		for (uint32 i=0; i<node->nChildren(); i++)
		{
			int childDepth = countDepth(node->child(i));
			if (childDepth > childrenDepth)
				childrenDepth=childDepth;
		}
		depth += childrenDepth;
	}
	return depth;
}

void InterpolatedAnimationChannel::reduceRotationKeys( float cosAngleError, Moo::NodePtr node )
{
	BW_GUARD;
	if (cosAngleError == 1.f)
		return;

	// Adding a scaling of the error factor for nodes with a large depth below them
	// this allows the lower children nodes to be compressed alot more than the ones
	// higher on the hierachy, improving the compression.
	if (node)
	{
		float angleError = acosf( Math::clamp(-1.0f, cosAngleError, +1.0f) );	
		int depthCount = countDepth(node);
		if (depthCount<1)
			depthCount=1;

		if (depthCount>1)
			angleError = (1.f / float(depthCount)) * angleError;
		cosAngleError = cosf(angleError);

		if (cosAngleError == 1.f)
			return;
	}

	// This occurs after the above code block since the block above
	// may early out.  If we get to here the rotation keys and
	// the rotation indices will be handled correctly.
	cleanKeys( rotationKeys_ );	

	//RotationKeyframes::iterator rit = rotationKeys_.begin();
    uint i = 0;
	//while( rit != rotationKeys_.end() )
    while (i != rotationKeys_.size())
	{
		//RotationKeyframes::iterator first = rit++;
        RotationKeyframes::iterator first = rotationKeys_.begin() + i++;
        RotationKeyframes::iterator rit = first + 1;

		bool stillGoing = rit != rotationKeys_.end();

		while( stillGoing )
		{
			RotationKeyframes::iterator delit = rit++;
            i++;

			if( rit != rotationKeys_.end() )
			{
				Quaternion q;
				D3DXQuaternionSlerp( &q, &first->second, &rit->second, (delit->first - first->first ) / ( rit->first - first->first ) );
				D3DXQuaternionNormalize( &q, &q );
				Quaternion delq = delit->second;
				D3DXQuaternionNormalize( &delq, &delq );
				if( D3DXQuaternionDot( &q, &delq ) > cosAngleError )
				{
					rotationKeys_.erase( delit );
                    rit = rotationKeys_.begin() + --i;
				}
				else
				{
					//--rit;
                    --i;
					stillGoing = false;
				}
			}
			else
				stillGoing = false;
		}
	}

	buildIndex( rotationKeys_, rotationIndex_ );
}

void InterpolatedAnimationChannel::reduceScaleKeys( float scaleError )
{
	BW_GUARD;
	cleanKeys( scaleKeys_ );

	//ScaleKeyframes::iterator sit = scaleKeys_.begin();
    uint i = 0;
	//while( sit != scaleKeys_.end() )
    while (i != scaleKeys_.size())
	{
        ScaleKeyframes::iterator sit = scaleKeys_.begin() + i++;
		ScaleKeyframes::iterator first = sit++;

		bool stillGoing = sit != scaleKeys_.end();
		while( stillGoing )
		{
			ScaleKeyframes::iterator delit = sit++;
            i++;

			if( sit != scaleKeys_.end() )
			{
				Vector3 v = ( ( sit->second - first->second ) / ( sit->first - first->first ) );
				v *= delit->first - first->first;
				v += first->second;
				v.x /= delit->second.x;
				v.y /= delit->second.y;
				v.z /= delit->second.z;
				v -= Vector3( 1, 1, 1 );
				if( fabs( v.x ) < scaleError &&
					fabs( v.y ) < scaleError &&
					fabs( v.z ) < scaleError )
				{
					scaleKeys_.erase( delit );
                    sit = scaleKeys_.begin() + --i;
				}
				else
				{
					//--sit;
                    --i;
					stillGoing = false;
				}
			}
			else
				stillGoing = false;
		}
	}

	buildIndex( scaleKeys_, scaleIndex_ );
}

void InterpolatedAnimationChannel::reducePositionKeys( float positionError )
{
	BW_GUARD;
	cleanKeys( positionKeys_ );

	//PositionKeyframes::iterator pit = positionKeys_.begin();
    uint i = 0;
	//while( pit != positionKeys_.end() )
    while( i != positionKeys_.size() )
	{
		//PositionKeyframes::iterator first = pit++;
        PositionKeyframes::iterator first = positionKeys_.begin() + i++;
        PositionKeyframes::iterator pit = first + 1;
		bool stillGoing = pit != positionKeys_.end();
		while( stillGoing )
		{
			PositionKeyframes::iterator delit = pit++;
            i++;

			if( pit != positionKeys_.end() )
			{
				Vector3 v = ( ( pit->second - first->second ) / ( pit->first - first->first ) );
				v *= delit->first - first->first;
				v += first->second;
				v -= delit->second;
				if( D3DXVec3Length( &v ) <= positionError )
				{
					positionKeys_.erase( delit );
                    pit = positionKeys_.begin() + --i;
				}
				else
				{
					//--pit;
                    --i;
					stillGoing = false;
				}
			}
			else
				stillGoing = false;
		}
	}

	buildIndex( positionKeys_, positionIndex_ );
}

void InterpolatedAnimationChannel::reduceKeyFramesTo( float rotationError, float scaleError, float positionError )
{
    BW_GUARD;
	uint maxs = (uint) (nScaleKeys() * scaleError);
    uint maxp = (uint) (nPositionKeys() * positionError);
    uint maxr = (uint) (nRotationKeys() * rotationError);

    const float MAX = 100.f;

    float scaleval = FLT_MIN;
    while (nScaleKeys() > maxs && scaleval < MAX)
    {
        scaleval *= 1.5f;
        reduceScaleKeys( scaleval );
    }
    float positionval = FLT_MIN;
    while (nPositionKeys() > maxp && positionval < MAX)
    {
        positionval *= 1.5f;
        reducePositionKeys( positionval );
    }
    float rotval = FLT_MIN;
    while (nRotationKeys() > maxr && rotval < MAX)
    {
        rotval *= 1.5f;
        reduceRotationKeys( cosf( rotval ) );
    }
}

/**
 *	Call this method when you have finished adding/reducing keys.
 *	Currently it is only necessary for memory accounting balancing.
 */
void InterpolatedAnimationChannel::finalise()
{
}

void InterpolatedAnimationChannel::preCombine( const AnimationChannel & rOther )
{
	BW_GUARD;
	InterpolatedAnimationChannel hack;

	PositionKeyframes::iterator pit = positionKeys_.begin();
	RotationKeyframes::iterator rit = rotationKeys_.begin();
	ScaleKeyframes::iterator sit = scaleKeys_.begin();

	PositionKeyframes::iterator pend = positionKeys_.end();
	RotationKeyframes::iterator rend = rotationKeys_.end();
	ScaleKeyframes::iterator send = scaleKeys_.end();

	float time = 0.f;
	Matrix m;
	Matrix us;

	while ( pit!=pend || rit!=rend || sit != send )
	{
		float pitTime = (pit != pend) ? pit->first : FLT_MAX;
		float ritTime = (rit != rend) ? rit->first : FLT_MAX;
		float sitTime = (sit != send) ? sit->first : FLT_MAX;

		float time = min( min(pitTime, ritTime), sitTime );

		if (pitTime == time)	++pit;
		if (ritTime == time)	++rit;
		if (sitTime == time)	++sit;

		rOther.result( time, m );
		this->result( time, us );
		us.preMultiply( m );
		hack.addKey( time, us );
	}

	scaleKeys_ = hack.scaleKeys_;
	positionKeys_ = hack.positionKeys_;
	rotationKeys_ = hack.rotationKeys_;
	scaleIndex_ = hack.scaleIndex_;
	positionIndex_ = hack.positionIndex_;
	rotationIndex_ = hack.rotationIndex_;
}

void InterpolatedAnimationChannel::postCombine( const AnimationChannel & rOther )
{
	BW_GUARD;
	InterpolatedAnimationChannel hack;

	PositionKeyframes::iterator pit = positionKeys_.begin();
	RotationKeyframes::iterator rit = rotationKeys_.begin();
	ScaleKeyframes::iterator sit = scaleKeys_.begin();

	PositionKeyframes::iterator pend = positionKeys_.end();
	RotationKeyframes::iterator rend = rotationKeys_.end();
	ScaleKeyframes::iterator send = scaleKeys_.end();

	float time = 0.f;
	Matrix m;
	Matrix us;

	while ( pit!=pend || rit!=rend || sit != send )
	{
		float pitTime = (pit != pend) ? pit->first : FLT_MAX;
		float ritTime = (rit != rend) ? rit->first : FLT_MAX;
		float sitTime = (sit != send) ? sit->first : FLT_MAX;

		float time = min( min(pitTime, ritTime), sitTime );

		if (pitTime == time)	++pit;
		if (ritTime == time)	++rit;
		if (sitTime == time)	++sit;

		rOther.result( time, m );
		this->result( time, us );
		us.postMultiply( m );
		hack.addKey( time, us );
	}

	scaleKeys_ = hack.scaleKeys_;
	positionKeys_ = hack.positionKeys_;
	rotationKeys_ = hack.rotationKeys_;
	scaleIndex_ = hack.scaleIndex_;
	positionIndex_ = hack.positionIndex_;
	rotationIndex_ = hack.rotationIndex_;
}




/**
 *	Load method.
 */
bool InterpolatedAnimationChannel::load( BinaryFile & bf )
{
	BW_GUARD;
	if( !this->AnimationChannel::load( bf ) )
		return false;
	if (readCompressionInfo_)
	{
		bf >> scaleCompressionError_;
		bf >> positionCompressionError_;
		bf >> rotationCompressionError_;
	}
	else
	{
		scaleCompressionError_ = 0.00001f;
		positionCompressionError_ = 0.00001f;
		rotationCompressionError_ = 0.0005f;
	}

	bf.readSequence( scaleKeys_ );
	bf.readSequence( positionKeys_ );
	bf.readSequence( rotationKeys_ );
	bf.readSequence( scaleIndex_ );
	bf.readSequence( positionIndex_ );
	bf.readSequence( rotationIndex_ );
	buildIndex( scaleKeys_, scaleIndex_ );
	buildIndex( positionKeys_, positionIndex_ );
	buildIndex( rotationKeys_, rotationIndex_ );

	// now compress with the factors we read in
	if (!inhibitCompression_)
	{
		reduceKeyFrames( cosf( rotationCompressionError_ ),
			scaleCompressionError_,
			positionCompressionError_ );
	}

	if (!this->valid())
		return false;


	return !!bf;
}


/**
 *	Save method.
 */
bool InterpolatedAnimationChannel::save( BinaryFile & bf ) const
{
	BW_GUARD;
	if (!this->valid())
		return false;

	// see if we're being converted on save
	if (s_saveConverter_ != NULL && s_saveConverter_->eligible( *this ))
		return s_saveConverter_->saveAs( bf, *this );

	// nope, process with our base class save then
	if( !this->AnimationChannel::save( bf ) )
		return false;

	// Save out as a type 4, with the compression factors
	// The keyframes shouldn't be compressed in this case either
	if (type() == 4)
	{
		bf << scaleCompressionError_;
		bf << positionCompressionError_;
		bf << rotationCompressionError_;
	}

	bf.writeSequence( scaleKeys_ );
	bf.writeSequence( positionKeys_ );
	bf.writeSequence( rotationKeys_ );
	bf.writeSequence( scaleIndex_ );
	bf.writeSequence( positionIndex_ );
	bf.writeSequence( rotationIndex_ );

	return !!bf;
}


int InterpolatedAnimationChannel::type() const
{
	BW_GUARD;
	if (s_saveConverter_ != NULL && s_saveConverter_->eligible( *this ))
		return s_saveConverter_->type();

	// We're being run in modeleditor, so save with the compression factors
	if (inhibitCompression_)
		return 4;

	// We've read in an old style file without compression factors
	// specified, and we compressed it on load, so again save it out raw
	else
		return 3;
}


int InterpolatedAnimationChannel::size() const
{
	return static_cast<int>(
		sizeof( uint ) + scaleKeys_.size() * sizeof( ScaleKeyframes::value_type )
		+ sizeof( uint ) + positionKeys_.size() * sizeof( PositionKeyframes::value_type )
		+ sizeof( uint ) + rotationKeys_.size() * sizeof( RotationKeyframes::value_type )
		+ sizeof( uint ) + scaleIndex_.size()
		+ sizeof( uint ) + positionIndex_.size()
		+ sizeof( uint ) + rotationIndex_.size() );
}


bool InterpolatedAnimationChannel::valid() const
{
	BW_GUARD;
	if (rotationKeys_.empty())
		return false;

	if (positionKeys_.empty())
		return false;

	if (scaleKeys_.empty())
		return false;

	for (uint i = 0; i < rotationIndex_.size(); ++i)
	{
		if (rotationIndex_[i] <= 0 || rotationIndex_[i] > rotationKeys_.size())
			return false;
	}

	for (uint i = 0; i < positionIndex_.size(); ++i)
	{
		if (positionIndex_[i] <= 0 || positionIndex_[i] > positionKeys_.size())
			return false;
	}

	for (uint i = 0; i < scaleIndex_.size(); ++i)
	{
		if (scaleIndex_[i] <= 0 || scaleIndex_[i] > scaleKeys_.size())
			return false;
	}

	return true;
}


std::ostream& operator<<(std::ostream& o, const InterpolatedAnimationChannel& t)
{
	o << "InterpolatedAnimationChannel\n";
	return o;
}


/*
	By default, we don't want to compress:

	ModelViewer wants the uncompressed data in any case
	client doesn't want to take the time
	xbsync is the only one who wants to do the compression
*/
bool InterpolatedAnimationChannel::inhibitCompression_ = true;

THREADLOCAL( InterpolatedAnimationChannel::Converter* )
	InterpolatedAnimationChannel::s_saveConverter_ = NULL;

InterpolatedAnimationChannel::TypeRegisterer
	InterpolatedAnimationChannel::s_rego1_(
	INTERPOLATED_ANIMATION_CHANNEL_COMPRESSION_ON_EXPLICIT_COMPRESSION_FACTORS_OFF, New1 );
InterpolatedAnimationChannel::TypeRegisterer
	InterpolatedAnimationChannel::s_rego3_(
	INTERPOLATED_ANIMATION_CHANNEL_COMPRESSION_OFF, New3 );
InterpolatedAnimationChannel::TypeRegisterer
	InterpolatedAnimationChannel::s_rego4_(
	INTERPOLATED_ANIMATION_CHANNEL_COMPRESSION_ON_EXPLICIT_COMPRESSION_FACTORS_ON, New4 );


} // namespace Moo

BW_END_NAMESPACE

// interpolated_animation_channel.cpp
