#include "CreatureEntityViewExtension.hpp"

#include "Entities/Creature.hpp"

#include <sstream>


/*
 *	Override from EntityViewDataProvider.
 */
BW::string CreatureEntityViewExtension::spriteName() const
{
	switch (this->CreatureExtension::pEntity()->creatureType())
	{
		case 1:
		case 2:
			return "striff";
		case 3:
			return "chicken";
		case 4:
			return "spider";
		case 5:
			return "crab";
		case 6:
			return "rat";
		default:
			return "";
	}
}


/*
 *	Override from EntityView.
 */
BW::string CreatureEntityViewExtension::popoverString() const
{
	BW::ostringstream popoverStream;
	
	const Creature * pCreature = this->CreatureExtension::pEntity();
	
	popoverStream << pCreature->creatureName() <<
		", Health: " << int( pCreature->healthPercent() ) << "%";
	
	return popoverStream.str();
}


// CreatureEntityViewExtension.mm
