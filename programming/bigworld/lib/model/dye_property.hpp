#ifdef _MSC_VER 
#pragma once
#endif

#ifndef DYE_PROPERTY_HPP
#define DYE_PROPERTY_HPP

#include "math/vector4.hpp"


BW_BEGIN_NAMESPACE

/**
 *	Inner class to represent a dye property
 */
class DyeProperty
{
public:
	int				index_;
	intptr			controls_;
	int				mask_;
	int				future_;
	Vector4			default_;
};

BW_END_NAMESPACE


#endif // DYE_PROPERTY_HPP

