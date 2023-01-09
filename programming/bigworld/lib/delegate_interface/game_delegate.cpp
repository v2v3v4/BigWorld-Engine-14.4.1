#include "game_delegate.hpp"
#include "cstdmf/debug.hpp"

BW_BEGIN_NAMESPACE
		
namespace
{
	IGameDelegate * g_gameDelegate = NULL;
} // anonymous namespace 

IGameDelegate* IGameDelegate::instance()
{
	return g_gameDelegate;
}

IGameDelegate::IGameDelegate()
{
	INFO_MSG("IGameDelegate::IGameDelegate. Self-registering.\n");

	MF_ASSERT(g_gameDelegate == NULL);
	g_gameDelegate = this;
}

IGameDelegate::~IGameDelegate()
{	
	INFO_MSG("IGameDelegate::~IGameDelegate. Self-UNregistering.\n");
	
	MF_ASSERT(g_gameDelegate != NULL);
	g_gameDelegate = NULL;
}

BW_END_NAMESPACE
