#ifdef _MSC_VER 
#pragma once
#endif

#ifndef DYE_PROP_SETTING_HPP
#define DYE_PROP_SETTING_HPP


#include "math/vector4.hpp"


BW_BEGIN_NAMESPACE

/**
 *	Inner class to represent a dye property setting
 */
class DyePropSetting
{
public:
	DyePropSetting() {}
	DyePropSetting( int i, const Vector4 & v ) : index_( i ), value_( v ) {}

	enum
	{
		PROP_TEXTURE_FACTOR,
		PROP_UV
	};

	int			index_;
	Vector4		value_;
};

BW_END_NAMESPACE


#endif // DYE_PROP_SETTING_HPP
