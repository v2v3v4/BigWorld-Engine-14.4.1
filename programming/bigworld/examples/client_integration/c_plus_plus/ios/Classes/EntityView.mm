#import "EntityView.hpp"

#import "ApplicationDelegate.h"
#import "WorldLayer.h"
#import "WorldScene.h"
#import "WorldUIModel.h"

#import "CCRemoveFromParent.h"
#import "SpriteSheetManager.h"

#import "math/vector3.hpp"

#import "connection_model/bw_entity.hpp"


/**
 *	Constructor.
 */
EntityView::EntityView( EntityViewDataProvider & provider ):
		provider_( provider ),
		sprite_( nil ),
		currentAnimationKey_( nil ),
		currentAnimation_( nil )
{
}


/**
 *	Destructor.
 */
EntityView::~EntityView()
{
	if (sprite_ && [sprite_ parent] != nil)
	{
		[sprite_ runAction:
		 [CCSequence actions: 
		  [CCFadeOut actionWithDuration: 0.25],
		  [CCRemoveFromParent action],
		  nil]];
		
		[sprite_ release];
		sprite_ = nil;
	}
	
	if (currentAnimation_)
	{
		[currentAnimation_ release];
	}
	
	if (currentAnimationKey_)
	{
		[currentAnimationKey_ release];
	}
}


/**
 *	This method is called to update the entity view to the entity's state.
 */
void EntityView::update()
{	
	if (sprite_ == nil)
	{
		this->initSprite();
	}
	
	WorldLayer * worldLayer = this->worldLayer();
	
	const BW::Vector3 & position = provider_.pViewEntity()->position();
	
	sprite_.position = ccp( position.x, position.z );
	sprite_.rotation = -(worldLayer.rotation);
	sprite_.scale = worldLayer.uiModel.entityScale * 2.0f;
	
	const BW::Direction3D & direction = provider_.pViewEntity()->direction();
	float newRotation = -(CC_RADIANS_TO_DEGREES( direction.yaw ) +
						  (worldLayer.rotation));
	
	// Wrap the rotation to a positive value.
	// NOTE: using fmodf returns strange values if negative. So keeping this
	// method.
	
	newRotation = newRotation - 360.0f * floorf( newRotation / 360.0f );
	//newRotation = fmodf(newRotation, 360.0f);
	
	// NOTE: This should be adding one animation per entity type per 
	// direction.
	// TODO: Make this have multiple animations rather than a set idle.
	// At some point grab an animation name from the entity.
						  
	SpriteSheetManager * spriteManager = [SpriteSheetManager sharedManager];
	
	NSString * spriteName = 
		[NSString stringWithUTF8String: provider_.spriteName().c_str()];
	
	NSString * newAnimation = 
		[spriteManager loadAnimationForEntity: spriteName 
							withAnimationName: @"idle"
								  inDirection: newRotation];
	
	// Set the new animation if we're not trying to load the same animation
	// again.
	if (![newAnimation isEqualToString: currentAnimationKey_])
	{
		currentAnimationKey_ = [newAnimation retain];
		
		// gtrab our new animation and run it on the sprite
		CCAnimation * newAnimation = 
			[spriteManager makeAnimation: currentAnimationKey_
									 fps: 30.0f];
		
		if (newAnimation)
		{
			[sprite_ stopAction: currentAnimation_];
			id action = [CCAnimate actionWithAnimation: newAnimation 
								  restoreOriginalFrame: NO];
			currentAnimation_ = [[CCRepeatForever actionWithAction: action] retain];
			[sprite_ runAction: currentAnimation_];
		}
	}
}


/**
 *	This method returns the world layer to add sprites to.
 */
WorldLayer * EntityView::worldLayer()
{
	return [WorldScene sharedWorldScene].worldLayer;
}


/**
 *	This method creates the sprite for this entity view.
 */
void EntityView::initSprite()
{
	int z = 10;
	NSString * spriteName =
	[NSString stringWithUTF8String: provider_.spriteName().c_str()];
	
	[sprite_ release];
	sprite_ = nil;
	
	if (provider_.pViewEntity()->isPlayer())
	{
		z += 1;
	}
	
	SpriteSheetManager * spriteManager = [SpriteSheetManager sharedManager];
	currentAnimationKey_ = [[spriteManager loadAnimationForEntity: spriteName
												withAnimationName: @"idle"
													  inDirection: 0]
							retain];
	
	CCAnimation * newAnimation =
	[spriteManager makeAnimation: currentAnimationKey_
							 fps: 30.0f];
	
	// Let Cocos throw the exception if the frame is nil (not found).
	// Same with the animation.
	
	sprite_ = [[CCSprite node] retain];
	
	id action = [CCAnimate actionWithAnimation: newAnimation
						  restoreOriginalFrame: NO];
	
	[sprite_ runAction: [CCRepeatForever actionWithAction: action]];
	
	// So our 'feet' are on the ground.
	[sprite_ setAnchorPoint: CGPointMake( 0.5f, 0 )];
	
	WorldLayer * worldLayer = this->worldLayer();

	[worldLayer addChild: sprite_
					   z: z];
	
	sprite_.opacity = 0;
	BW::Vector3 tint = provider_.spriteTint();
	sprite_.color = ccc3( tint.x, tint.y, tint.z );
	[sprite_ runAction: [CCFadeIn actionWithDuration: 0.25]];
}


/**
 *	This method implements the hit testing for a given location.
 */
bool EntityView::hitTest( const CGPoint & location ) const
{
	CGRect hitRect = [sprite_ boundingBox];
	
	if ((hitRect.size.width < 22) || (hitRect.size.height < 22))
	{
		hitRect = CGRectMake( hitRect.origin.x, hitRect.origin.y, 22, 22 );
	}
	
	return CGRectContainsPoint( hitRect, location );
}


/**
 *	This method handles a click event.
 */
void EntityView::handleClick()
{
	provider_.onClick();
}


// EntityView.mm
