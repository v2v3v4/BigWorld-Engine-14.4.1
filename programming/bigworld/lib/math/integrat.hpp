/**
 *	@file
 *	This file implements an integrate function.
 *
 *	@ingroup Math
 */

#ifndef INTEGRAT_HPP
#define INTEGRAT_HPP

BW_BEGIN_NAMESPACE

/**
 *	This function integrates the input Functor for one step of size h for the
 *	given initial value x. It does this numerically using Runge-Kutta (Order
 *	Four).
 *
 *	X_TYPE must support :
 *	@code
 *	float * X_TYPE
 *	X_TYPE + X_TYPE
 *	+= X_TYPE
 *	@endcode
 */
template <class Functor, class X_TYPE>
inline void integrate(const Functor & F, X_TYPE & x, float h)
{
	// Integrate the function using Runge-Kutta (Order Four)

	X_TYPE A1 = F(0, x);
	X_TYPE xTemp = x + (h/2.f) * A1;

	X_TYPE A2 =  F(h/2.f, xTemp);
	xTemp = x + (h/2.f) * A2;

	X_TYPE A3 = F(h/2.f, xTemp);
	xTemp = x + h * A3;

	X_TYPE A4 = F(h, xTemp);

	x += (h/6.f) * (A1 + A2 + A2 + A3 + A3 + A4);
}


/**
 *	This class implements an example functor. It must implement operator() as
 *	shown. The operator should return the value of the function to be
 *	integrated. This is probably not a good example because it integrates
 *	y' = -cy (where c is the approach rate). The exact solution to this is
 *	y = e^(-ct).
 *
 *	@ingroup Math
 */
template <class TYPE>
class ApproachFunctor
{
public:
	/// Constructor.
	ApproachFunctor(const TYPE & desiredValue,
			float approachRate = 1.f) :
		desiredValue_(desiredValue),
		approachRate_(approachRate){};

	/**
	 *	This method prints the () operator.
	 */
	TYPE operator()(float t, const TYPE & x) const
	{
		return approachRate_ * (desiredValue_ - x);
	}

private:
	const TYPE & desiredValue_;
	float approachRate_;
};


/**
 *	This function is a helper so that you do not have to specify the type of the
 *	desiredValue.
 *
 *	@ingroup Math
 */
template <class TYPE>
inline ApproachFunctor<TYPE> makeApproach(const TYPE & desiredValue,
										  float approachRate = 1.f)
{
	return ApproachFunctor<TYPE>(desiredValue,
		approachRate);
}

BW_END_NAMESPACE

#endif // INTEGRAT_HPP
