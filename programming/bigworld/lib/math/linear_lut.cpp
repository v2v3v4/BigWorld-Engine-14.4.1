#include "pch.hpp"
#include "math/linear_lut.hpp"
#include "math/mathdef.hpp"
#include <algorithm>

BW_BEGIN_NAMESPACE

/**
 *	This constructs an empty LinearInterp.
 */
LinearLUT::LinearLUT():
	lowerBC_(BC_CONSTANT_EXTEND),
	upperBC_(BC_CONSTANT_EXTEND),
	cachedPos_((size_t)-1)
{
}


/**
 *	Find the interpolated value at x using the data and boundary conditions.
 *
 *	@param x			The x-value to look up.
 *	@return				The y-value given the data and boundary conditions.
 */
float LinearLUT::operator()(float x) const
{
	// See if the x value is within the cached segment:
	if 
	(
		cachedPos_ != (size_t)-1 
		&& 
		data_[cachedPos_].x <= x && x <= data_[cachedPos_ + 1].x
	)
	{
		return 
			Math::lerp
			(
				x, 
				data_[cachedPos_].x, data_[cachedPos_ + 1].x, 
				data_[cachedPos_].y, data_[cachedPos_ + 1].y
			);
	}

	size_t sz = data_.size();

	// Handle degenerate cases:
	if (sz == 0)
	{
		return 0.0f;
	}
	else if (sz == 1)
	{
		if (lowerBC_ == BC_ZERO && x < data_[0].x)
			return 0.0f;
		else if (upperBC_ == BC_ZERO && x > data_[0].x)
			return 0.0f;
		else
			return data_[0].y;
	}

	// Handle the boundary cases:
	if (x < data_[0].x)
	{
		switch (lowerBC_)
		{
		case BC_ZERO:
			return 0.0f;
		case BC_CONSTANT_EXTEND:
			return data_[0].y;
		case BC_WRAP:
			x = fmod(x - data_[0].x, data_[sz - 1].x - data_[0].x) + data_[0].x;
			break;
		case BC_LINEAR_EXTEND:
			return 
				Math::lerp
				(
					x, 
					data_[0].x, data_[1].x, 
					data_[0].y, data_[1].y
				);
		}
	}
	if (x > data_[sz - 1].x)
	{
		switch (upperBC_)
		{
		case BC_ZERO:
			return 0.0f;
		case BC_CONSTANT_EXTEND:
			return data_[sz - 1].y;
		case BC_WRAP:
			x = fmod(x - data_[0].x, data_[sz - 1].x - data_[0].x) + data_[0].x;
			break;
		case BC_LINEAR_EXTEND:
			return 
				Math::lerp
				(
					x, 
					data_[sz - 2].x, data_[sz - 1].x, 
					data_[sz - 2].y, data_[sz - 1].y
				);
		}
	}

	// Find the position within the table using a binary search:
	size_t loIdx = 0;
	size_t hiIdx = sz - 1;
	while (hiIdx - loIdx > 1)
	{
		size_t midX = (loIdx + hiIdx)/2;
		if (x < data_[midX].x)
			hiIdx = midX;
		else
			loIdx = midX;
	}

	cachedPos_ = loIdx; // Save the cached lookup point

	// Finally do the linear interpolation:
	return 
		Math::lerp
		(
			x, 
			data_[cachedPos_].x, data_[cachedPos_ + 1].x, 
			data_[cachedPos_].y, data_[cachedPos_ + 1].y
		);
}


/**
 *	Set the data for interpolation.
 *
 *	@param d			The data as x,y pairs.  Note that we resort the data
 *						based upon x-coordinate.  We also assume that the data
 *						does not have any pairs where the x-coordinate is
 *						duplicated.
 */
void LinearLUT::data(BW::vector<Vector2> const &d)
{
	data_ = d;
	std::sort(data_.begin(), data_.end());

	cachedPos_ = (size_t)-1; // Invalidate the cached lookup
}


/**
 *	Get the interpolation data.
 *
 *	@return				The data used to do the interpolation.
 */
BW::vector<Vector2> const &LinearLUT::data() const
{
	return data_;
}


/**
 *	Set the lower boundary condition.
 *
 *	@param bc			The new lower boundary condition.
 */
void LinearLUT::lowerBoundaryCondition(BoundaryCondition bc)
{
	lowerBC_ = bc;
}


/**
 *	Get the lower boundary condition.
 *
 *	@return				The lower boundary condition.
 */
LinearLUT::BoundaryCondition LinearLUT::lowerBoundaryCondition() const
{
	return lowerBC_;
}


/**
 *	Set the upper boundary condition.
 *
 *	@param bc			The new upper boundary condition.
 */
void LinearLUT::upperBoundaryCondition(BoundaryCondition bc)
{
	upperBC_ = bc;
}


/**
 *	Get the upper boundary condition.
 *
 *	@return				The upper boundary condition.
 */
LinearLUT::BoundaryCondition LinearLUT::upperBoundaryCondition() const
{
	return upperBC_;
}

BW_END_NAMESPACE

// linear_lut.cpp
