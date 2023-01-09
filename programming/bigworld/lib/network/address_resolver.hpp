#ifndef ADDRESS_RESOLVER_HPP
#define ADDRESS_RESOLVER_HPP

#include "cstdmf/bw_string.hpp"

#include "cstdmf/bw_namespace.hpp"

BW_BEGIN_NAMESPACE

namespace Mercury
{

namespace AddressResolver
{

bool resolve( const BW::string & hostName, BW::string & out );

} // end namespace AddressResolver

} // end namespace Mercury

BW_END_NAMESPACE

#endif // ADDRESS_RESOLVER_HPP

