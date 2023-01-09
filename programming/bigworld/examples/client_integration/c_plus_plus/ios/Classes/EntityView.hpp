#ifndef ENTITY_VIEW_HPP
#define ENTITY_VIEW_HPP

#import "cocos2d.h"

#include "math/vector3.hpp"

#include <string>

namespace BW
{
	class BWEntity;
}


@class WorldLayer;


/**
 *	This interface is used by EntityView to supply data and behaviour for this entity view.
 */
class EntityViewDataProvider
{
public:
	/** Destructor. */
	virtual ~EntityViewDataProvider() {}
	
	/**
	 *	This method returns the associated entity.
	 */
	virtual const BW::BWEntity * pViewEntity() const = 0;

	/**
	 *	This method returns the sprite tint colour to use.
	 */
	virtual const BW::Vector3 spriteTint() const = 0;

	/**
	 *	This method returns the name of the sprite to use.
	 */
	virtual BW::string spriteName() const = 0;

	/**
	 *	This method returns a string to be displayed in the entity info pop-up.
	 */
	virtual BW::string popoverString() const = 0;
	
	/**
	 *	This method handles clicks on this entity's sprite.
	 */
	virtual void onClick() {}
	
	
protected:
	/** Constructor. */
	EntityViewDataProvider() {}
};


/**
 *	This class represents the visual representation of the entity. 
 */
class EntityView
{
public:
	EntityView( EntityViewDataProvider & dataProvider );
	
	~EntityView();

	/**
	 *	This method returns the associated entity.
	 */
	const BW::BWEntity * pEntity() const { return provider_.pViewEntity(); }
	
	void update();
	

	/** Return the name of the sprite to use. */ 
	BW::string spriteName() const
	{
		return provider_.spriteName();
	}
	
	/** Return the sprite tint colour. */
	const BW::Vector3 spriteTint() const
	{
		return provider_.spriteTint();
	}
	
	/** Return the popover string when the entity is tapped. */
	BW::string popoverString() const
	{
		return provider_.popoverString();
	}
	
	/** Return the sprite used by this entity view. */
	CCSprite * sprite()
	{
		return sprite_;
	}

	bool hitTest( const CGPoint & location ) const;
	void handleClick();
	
private:
	WorldLayer * worldLayer();
	
	void initSprite();

	EntityViewDataProvider &	provider_;
	CCSprite *					sprite_;
	NSString *					currentAnimationKey_;
	CCAction *					currentAnimation_;
};


#endif // ENTITY_VIEW_HPP
