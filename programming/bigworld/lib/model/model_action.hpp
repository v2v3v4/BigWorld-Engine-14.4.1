#ifdef _MSC_VER 
#pragma once
#endif

#ifndef MODEL_ACTION_HPP
#define MODEL_ACTION_HPP


#include "forward_declarations.hpp"
#include "match_info.hpp"
#include "resmgr/forward_declarations.hpp"


BW_BEGIN_NAMESPACE

class Capabilities;


/**
 *	Inner class to represent the base Model's action
 */
class ModelAction : public ReferenceCount
{
public:
	ModelAction( DataSectionPtr pSect );
	~ModelAction();

	bool valid( const Model & model ) const;
	bool promoteMotion() const;

	uint32 sizeInBytes() const;

	BW::string	name_;
	BW::string	animation_;
	float		blendInTime_;
	float		blendOutTime_;
	int			track_;
	bool		filler_;
	bool		isMovement_;
	bool		isCoordinated_;
	bool		isImpacting_;

	int			flagSum_;

	bool		isMatchable_;
	MatchInfo	matchInfo_;
};

BW_END_NAMESPACE


#endif // ACTION_HPP
