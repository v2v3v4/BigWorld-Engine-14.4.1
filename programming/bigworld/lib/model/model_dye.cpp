#include "pch.hpp"

#include "model_dye.hpp"


BW_BEGIN_NAMESPACE

ModelDye::ModelDye( const Model & model, int matterIndex, int tintIndex )
	:	matterIndex_( matterIndex ),
		tintIndex_( tintIndex )
{
	BW_GUARD;
	IF_NOT_MF_ASSERT_DEV( matterIndex >= std::numeric_limits< int16 >::min() )
	{
		MF_EXIT( "matterIndex is outside the required range < std::numeric_limits< int16 >::min()" );
	}
	IF_NOT_MF_ASSERT_DEV( matterIndex <= std::numeric_limits< int16 >::max() )
	{
		MF_EXIT( "matterIndex is outside the required range > std::numeric_limits< int16 >::max()" );
	}

	IF_NOT_MF_ASSERT_DEV( tintIndex >= std::numeric_limits< int16 >::min() )
	{
		MF_EXIT( "tintIndex is outside the required range < std::numeric_limits< int16 >::min()" );
	}

	IF_NOT_MF_ASSERT_DEV( tintIndex <= std::numeric_limits< int16 >::max() )
	{
		MF_EXIT( "tintIndex is outside the required range > std::numeric_limits< int16 >::max()" );
	}

	//MF_ASSERT( matterIndex == -1 || matterIndex < model.
}


ModelDye::ModelDye( const ModelDye & modelDye )
	:	matterIndex_( modelDye.matterIndex_ ),
		tintIndex_( modelDye.tintIndex_ )
{
	BW_GUARD;
}


ModelDye & ModelDye::operator=( const ModelDye & modelDye )
{
	BW_GUARD;
	matterIndex_ = modelDye.matterIndex_;
	tintIndex_ = modelDye.tintIndex_;

	return *this;
}


bool ModelDye::isNull() const
{
	return (matterIndex_ == -1) && (tintIndex_ == -1);
}


int16 ModelDye::matterIndex() const
{
	return matterIndex_;
}


int16 ModelDye::tintIndex() const
{
	return tintIndex_;
}

BW_END_NAMESPACE

// model_dye.cpp
