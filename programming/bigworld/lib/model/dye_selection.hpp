#ifdef _MSC_VER 
#pragma once
#endif

#ifndef DYE_SELECTION_HPP
#define DYE_SELECTION_HPP


#include "dye_prop_setting.hpp"


BW_BEGIN_NAMESPACE

/**
 *	Inner class to represent a dye selection (for billboards)
 */
class DyeSelection
{
public:
	BW::string		matterName_;
	BW::string		tintName_;
	BW::vector< DyePropSetting > properties_;
};

BW_END_NAMESPACE

#endif // DYE_SELECTION_HPP
