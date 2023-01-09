#ifndef RENDER_CONTEXT_DEBUG
#define RENDER_CONTEXT_DEBUG

#ifdef _DEBUG

#include <iostream>


BW_BEGIN_NAMESPACE

class RenderContext;

namespace Moo
{
    std::ostream &printRenderContextState(std::ostream &out, RenderContext &rc);
} // namespace Moo

BW_END_NAMESPACE

#endif // _DEBUG

#endif // RENDER_CONTEXT_DEBUG
