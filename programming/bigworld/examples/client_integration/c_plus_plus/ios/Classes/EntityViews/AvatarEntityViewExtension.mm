#include "AvatarEntityViewExtension.hpp"

#include "Entities/Avatar.hpp"


/*
 *	Override from EntityViewDataProvider.
 */
BW::string AvatarEntityViewExtension::spriteName() const
{
	return "male";
}


/*
 *	Override from EntityViewDataProvider.
 */
BW::string AvatarEntityViewExtension::popoverString() const
{
	return this->AvatarExtension::pEntity()->playerName();
}


/*
 *	Override from EntityViewDataProvider.
 */
void AvatarEntityViewExtension::onClick()
{
	if (!this->pEntity()->isPlayer())
	{
		BW::ostringstream chatMessageStream;
		const Avatar * pAvatar = this->AvatarExtension::pEntity();
		chatMessageStream << "Hello, from " <<
			pAvatar->playerName() << "!";
		pAvatar->cell().chat( chatMessageStream.str() );
	}
	
}


// AvatarEntityViewExtension.mm