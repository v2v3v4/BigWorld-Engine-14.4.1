#include "pch.hpp"

#include "interface_minder.hpp"

#include "machined_utils.hpp"
#include "network_interface.hpp"

DECLARE_DEBUG_COMPONENT2( "Network", 0 );


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "interface_minder.ipp"
#endif

namespace Mercury
{

// -----------------------------------------------------------------------------
// Section: InterfaceMinder
// -----------------------------------------------------------------------------

/**
 * 	This method adds an interface element (Mercury method) to the interface minder.
 *  @param name             Name of the interface element.
 * 	@param lengthStyle		Specifies whether the message is fixed or variable.
 *	@param lengthParam		This depends on lengthStyle.
 *	@param pHandler			The message handler for this interface.
 */
InterfaceElement & InterfaceMinder::add( const char * name,
	int8 lengthStyle, int lengthParam, InputMessageHandler * pHandler )
{
	const MessageID id = static_cast<MessageID>(elements_.size());

	// Set up the new bucket and add it to the list
	InterfaceElement element( name, id, lengthStyle, lengthParam,
		pHandler );

	elements_.push_back( element );
	return elements_.back();
}


/**
 *	This method populates the interface with a range of the same interface
 *	element.
 *
 *	@param ie			The InterfaceElement to use.
 *	@param rangePortion	Specifies the proportion of the remaining range to
 *		consume. A value of x specifies 1/x.
 */
MessageID InterfaceMinder::addRange( const InterfaceElement & ie,
		int rangePortion )
{
	MF_ASSERT( ie.id() == elements_.back().id() );

	const size_t startID = elements_.size();
	const size_t endID = startID + (254 - startID)/rangePortion;

	while (elements_.size() <= endID)
	{
		this->add(
			ie.name(), ie.lengthStyle(), ie.lengthParam(), ie.pHandler() );
	}

	return MessageID( endID );
}


/**
 * 	This method registers all the minded interface elements with an interface.
 *
 * 	@param networkInterface	The network interface to register with.
 */
void InterfaceMinder::registerWithInterface(
		NetworkInterface & networkInterface )
{
	for (uint i=0; i < elements_.size(); ++i)
	{
		const InterfaceElement & element = elements_[i];
		networkInterface.interfaceTable().serve( element, element.pHandler() );
	}
}


/**
 *  This method registers this interface with machined on behalf of the nub.
 */
Reason InterfaceMinder::registerWithMachined( const Address & addr,
	int id ) const
{
	return this->registerWithMachinedAs( name_, addr, id );
}

Reason InterfaceMinder::registerWithMachinedAs( const char * name,
		const Address & addr, int id ) const
{
	return MachineDaemon::registerWithMachined( addr, name, id );
}

} // namespace Mercury

BW_END_NAMESPACE

// interface_minder.cpp

