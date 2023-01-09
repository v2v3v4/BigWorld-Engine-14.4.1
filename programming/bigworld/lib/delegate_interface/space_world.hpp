#ifndef SPACE_WORLD_HPP
#define SPACE_WORLD_HPP

#include "cstdmf/bw_namespace.hpp"
#include "cstdmf/shared_ptr.hpp"

BW_BEGIN_NAMESPACE

class ISpaceWorld
{
public:
	virtual ~ISpaceWorld() {}
	
	virtual int getNumObjects() const = 0;
	
	// Get boundary in a "neutral" format: min-max pairs
	virtual std::pair<float,float> getBoundsX() const = 0;
	virtual std::pair<float,float> getBoundsY() const = 0;
	virtual std::pair<float,float> getBoundsZ() const = 0;
};

typedef shared_ptr<ISpaceWorld> ISpaceWorldPtr;

BW_END_NAMESPACE

#endif // SPACE_WORLD_HPP
