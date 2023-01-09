#ifndef BW_STD_HPP
#define BW_STD_HPP

//
// This header brings std or std::tr1 content into BW::std conditionally based
// on compiler version/vendor.
// NOTE: This header is incompatible with 'using namespace BW'.
//
namespace std { namespace tr1 { } }
namespace BW
{
#if _MSC_VER || defined( EMSCRIPTEN ) || __cplusplus >= 201103L
namespace std = ::std;
#else
namespace std {
namespace tr1 = ::std::tr1;
using namespace ::std::tr1;
using namespace ::std;
}
#endif
} // namespace BW

#endif // BW_STD_HPP
