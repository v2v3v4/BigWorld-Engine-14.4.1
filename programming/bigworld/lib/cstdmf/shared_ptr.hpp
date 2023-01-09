#ifndef SHARED_PTR_HPP
#define SHARED_PTR_HPP

#if defined( __EMSCRIPTEN__ ) || defined( _MSC_VER)

#include <memory>

namespace BW 
{
using std::shared_ptr;
} // namespace BW

#else // !defined( __EMSCRIPTEN__ )

#include <tr1/memory>

namespace BW 
{
using std::tr1::shared_ptr;
} // namespace BW

#endif // defined( __EMSCRIPTEN__ )

#endif // SHARED_PTR_HPP
