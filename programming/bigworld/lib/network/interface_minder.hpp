#ifndef INTERFACE_MINDER_HPP
#define INTERFACE_MINDER_HPP

#include "interface_element.hpp"
#include "misc.hpp"

#include "cstdmf/stdmf.hpp"

BW_BEGIN_NAMESPACE

namespace Mercury
{

class NetworkInterface;

/**
 * 	The InterfaceMinder class manages a set of interface elements. It provides
 * 	an iterator for iterating over this set.
 *
 * 	@ingroup mercury
 */
class InterfaceMinder
{
public:
	InterfaceMinder( const char * name );

	InterfaceElement & add( const char * name, int8 lengthStyle,
			int lengthParam, InputMessageHandler * pHandler = NULL );

	MessageID addRange( const InterfaceElement & ie,
			int rangePortion );

	InputMessageHandler * handler( int index );
	void handler( int index, InputMessageHandler * pHandler );
	const InterfaceElement & interfaceElement( uint8 id ) const;

	void registerWithInterface( NetworkInterface & networkInterface );
	Reason registerWithMachined( const Address & addr, int id ) const;
	Reason registerWithMachinedAs( const char * name,
			const Address & addr, int id ) const;

private:
	InterfaceElements		elements_;

	const char *			name_;
};

}	// end of namespace Mercury

#ifdef CODE_INLINE
#include "interface_minder.ipp"
#endif

BW_END_NAMESPACE

#endif // INTERFACE_MINDER_HPP
