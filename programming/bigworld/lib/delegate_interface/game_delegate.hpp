#ifndef GAME_DELEGATE_HPP
#define GAME_DELEGATE_HPP

#include "cstdmf/bw_namespace.hpp"

BW_BEGIN_NAMESPACE

class ISpaceWorldFactory;
class IEntityDelegateFactory;

/**
 * This interface represents an EntityDelegate which is a "bridge" or "gateway"
 * object, through an Entity communicates with the additional functionality, 
 * which is implemented elsewhere.
 */
class IGameDelegate
{
public:
	IGameDelegate();
	virtual ~IGameDelegate();

	virtual bool initialize( const char * resPaths ) = 0;
	
	virtual void shutDown() = 0;

	virtual ISpaceWorldFactory* getSpaceWorldFactory() = 0;
	
	virtual IEntityDelegateFactory* getEntityDelegateFactory() = 0;

	virtual void update() = 0;
	
	static IGameDelegate* instance();
};

BW_END_NAMESPACE

#endif // GAME_DELEGATE_HPP
