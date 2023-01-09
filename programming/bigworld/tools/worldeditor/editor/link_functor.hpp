#ifndef LINK_FUNCTOR_HPP
#define LINK_FUNCTOR_HPP


#include "gizmo/always_applying_functor.hpp"
#include "worldeditor/editor/link_proxy.hpp"

BW_BEGIN_NAMESPACE

/**
 *  This functor helps to link items.
 */
class LinkFunctor : public AlwaysApplyingFunctor
{
public:
    LinkFunctor( LinkProxyPtr linkProxy, bool allowedToDiscardChanges = true );

protected:
	virtual void stopApplyCommitChanges( Tool & tool, bool addUndoBarrier );
	virtual void stopApplyDiscardChanges( Tool & tool );

    LinkProxyPtr            linkProxy_;

private:
    LinkFunctor(LinkFunctor const &);
    LinkFunctor &operator=(LinkFunctor const &);
};

BW_END_NAMESPACE

#endif // LINK_FUNCTOR_HPP
