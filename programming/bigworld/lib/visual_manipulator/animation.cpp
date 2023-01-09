#include "pch.hpp"

#include "animation.hpp"
#include "cstdmf/binaryfile.hpp"
#include "math/blend_transform.hpp"


BW_BEGIN_NAMESPACE

using namespace VisualManipulator;


Animation::Animation() :
frameCount_( 0 )
{
}

void Animation::addChannel( const BW::string& identifier, const MatrixVector& transforms )
{
	channels_.push_back( Channel( identifier, transforms ) );
	frameCount_ = std::max<uint32>( (uint32)transforms.size(), frameCount_ );
}

bool Animation::save( const BW::string& filename )
{
	FILE* f = fopen( filename.c_str(), "wb" );
	if (!f)
	{
		return false;
	}

	BinaryFile animation( f );
	
	animation << float(frameCount_) << name_ << name_ << channels_.size();

	Channels::iterator it = channels_.begin();
	while (it != channels_.end())
	{
		MatrixVector& transforms = it->second;
		typedef std::pair<float, Vector3> ScaleKey;
		typedef std::pair<float, Vector3> PositionKey;
		typedef std::pair<float, Quaternion> RotationKey;

		BW::vector< int > boundTable;
		BW::vector< ScaleKey > scales;
		BW::vector< PositionKey > positions;
		BW::vector< RotationKey > rotations;

		if (transforms.size() < frameCount_)
		{
			transforms.resize( frameCount_, transforms.back() );
		}

		for (uint32 i = 0; i < transforms.size();  i++)
		{
			BlendTransform bt( transforms[i] );
			bt.normaliseRotation();

			boundTable.push_back( i + 1 );
			scales.push_back( ScaleKey( float(i), bt.scaling() ) );
			positions.push_back( PositionKey( float(i), bt.translation() ) );
			rotations.push_back( RotationKey( float(i), bt.rotation() ) );
		}

		animation << int( 1 );
		animation << it->first;
		animation.writeSequence( scales );
		animation.writeSequence( positions );
		animation.writeSequence( rotations );
		animation.writeSequence( boundTable );
		animation.writeSequence( boundTable );
		animation.writeSequence( boundTable );

		++it;
	}

	fclose( f );

	return true;
}

BW_END_NAMESPACE
