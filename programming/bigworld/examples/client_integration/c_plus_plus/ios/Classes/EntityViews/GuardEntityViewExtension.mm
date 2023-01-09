#include "GuardEntityViewExtension.hpp"

#include "Entities/Guard.hpp"

#include <sstream>


/*
 *	Override from EntityViewDataProvider.
 */
BW::string GuardEntityViewExtension::spriteName() const
{
	return "orc";
}


/*
 *	Override from EntityViewDataProvider.
 */
BW::string GuardEntityViewExtension::popoverString() const
{
	BW::ostringstream popoverStream;
	
	const Guard * pGuard = this->GuardExtension::pEntity();
	popoverStream << "Guard, Health: " <<
		int( pGuard->healthPercent() ) << "%";
	
	return popoverStream.str();
}


// GuardEntityViewExtension.cpp
