#ifndef COMPILED_SPACE_FORWARD_DECLARATIONS_HPP
#define COMPILED_SPACE_FORWARD_DECLARATIONS_HPP

//-----------------------------------------------------------------------------
// Standard inclusions
#include "cstdmf/bw_namespace.hpp"
#include "cstdmf/stdmf.hpp"

//-----------------------------------------------------------------------------
// API Exposure
#include "compiled_space_dll.hpp"

//-----------------------------------------------------------------------------
// Forward declarations

BW_BEGIN_NAMESPACE

template <typename IndexType>
class IndexSpan;

BW_END_NAMESPACE

namespace BW {

typedef IndexSpan<uint16>  DataSpan;

} // namespace BW

#endif // COMPILED_SPACE_FORWARD_DECLARATIONS_HPP
