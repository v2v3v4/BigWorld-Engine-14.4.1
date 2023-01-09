//
//  EntityInfoLayer.mm
//  bigworld
//
//  Created by Thomas Cowell on 15/06/11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#import "EntityInfoLayer.h"
#import "DeviceRotation.h"
#import "SpriteSheetManager.h"

#import "EntityView.hpp"

#import "WorldLayer.h"

#import "connection_model/bw_entity.hpp"


#define HIDE_TIMEOUT 2 // seconds

@implementation EntityInfoLayer

@synthesize contentViewController;
@synthesize popoverController;
@synthesize entityView;
@synthesize hideTimer;

@synthesize interfaceOrientation;
@synthesize shouldUpdateMessage;


- (id) init
{
	self = [super init];
			
	if (self != nil) 
	{
		[self registerForDeviceRotation];
		
		// we don't want the orientation to change as cocos2D does NOT
		// handle the rotation, The UIView does.
		self.interfaceOrientation = UIInterfaceOrientationLandscapeRight;
				//[UIApplication sharedApplication].statusBarOrientation;
	}

	return self;
}

- (void) dealloc
{
	[self unscheduleAllSelectors];
	self.contentViewController = nil;
	self.popoverController = nil;
	self.entityView = nil;
	self.hideTimer = nil;
	
	[self deregisterForDeviceRotation];
	[super dealloc];
}

- (void) onDeviceRotated
{
	// NOTE: we don't want cocos2D to handle rotation.
	/*if (UIInterfaceOrientationIsPortrait(self.interfaceOrientation) !=
			UIInterfaceOrientationIsPortrait(
				[UIApplication sharedApplication].statusBarOrientation ))
	{
		[self adjustLayout];
		self.interfaceOrientation =
			[UIApplication sharedApplication].statusBarOrientation;
	}*/
}

- (CCLayer*) findWorldLayer
{
	CCNode * p = self.parent;
	
	for (CCLayer * w in [p children])
	{
		if ([w isKindOfClass:[WorldLayer class]])
		{
			return w;
		}
	}
	
	return nil;
}

- (void) adjustLayout
{
	if ((self.entityView != nil) && self.entityView->pEntity()->isDestroyed())
	{
		[self hide];
	}
	else if (self.popoverController != nil)
	{
		CCLayer * w = [self findWorldLayer];
		BW::Position3D position = self.entityView->pEntity()->position();
		CGPoint pos = [[CCDirector sharedDirector]
			convertToUI: [w convertToWorldSpace:
				CGPointMake( position.x, position.z )]];

		// This is workaround for convertToWorldSpace not working (or seemingly
		// working) properly on retina devices
		pos.x *= CC_CONTENT_SCALE_FACTOR();
		pos.y *= CC_CONTENT_SCALE_FACTOR();
		
		CGRect frame = [[[CCDirector sharedDirector] openGLView] bounds];
		CGRect posRect = CGRectMake( pos.x - 6, pos.y - 11, 22, 22 );

		if (!CGRectContainsRect( frame, posRect ))
		{
			[self hide];
		}
		else
		{
			if (self.shouldUpdateMessage)
			{
				[self.contentViewController
				 updateInfo: [NSString stringWithUTF8String: self.entityView->popoverString().c_str()]];
			}

			// remove the width so the arrow follow entity exactly
			posRect = CGRectMake( posRect.origin.x, posRect.origin.y, 0,
					posRect.size.height );
			[self.popoverController
				repositionPopoverFromRect: posRect
				permittedArrowDirections:
					UIPopoverArrowDirectionDown|UIPopoverArrowDirectionUp];
		}
	}
}

- (void) updateForEntityView: (EntityView *) anEntityView
				 withMessage: (NSString *) msg
				shouldUpdate: (BOOL) shouldUpdate
{
	self.shouldUpdateMessage = shouldUpdate;
	self.entityView = anEntityView;

	NSString * icon = [NSString stringWithFormat: @"%s_icon", 
					   self.entityView->spriteName().c_str()];
	NSString * entityTypeName =
		[NSString stringWithUTF8String: self.entityView->pEntity()->entityTypeName().c_str()];
	
	self.contentViewController =
	[[[EntityInfoViewController alloc] initWithIconNameAndInfo: icon  
														  name: entityTypeName
														  info: msg]
	 autorelease];
}

- (void) updateForEntityView: (EntityView *) anEntityView
				 withMessage: (NSString *) msg
{
	[self updateForEntityView: anEntityView
				  withMessage: msg
				 shouldUpdate: NO];
}


- (void) updateForEntityView: (EntityView *) anEntityView
{
	const BW::string popoverString = anEntityView->popoverString();
	[self updateForEntityView: anEntityView
				  withMessage: [NSString stringWithUTF8String: popoverString.c_str()]
				 shouldUpdate: YES];
}


- (void) show
{
	if (self.popoverController != nil)
	{
		[self.popoverController dismissPopoverAnimated:NO];
	}
	
	CCLayer * w = [self findWorldLayer];
	BW::Position3D position = self.entityView->pEntity()->position();
	
	CCDirector * director = [CCDirector sharedDirector];
	CGPoint pos = [director convertToUI: [w convertToWorldSpace:CGPointMake( position.x, position.z )]];
	
	self.popoverController = [[WEPopoverController alloc]
			initWithContentViewController:self.contentViewController];
	
	[self.popoverController presentPopoverFromRect: CGRectMake( pos.x-6, pos.y-11, 0, 22 )
											inView: [director openGLView]
						  permittedArrowDirections: UIPopoverArrowDirectionDown | UIPopoverArrowDirectionUp
										  animated: YES];
	
	[self schedule: @selector( adjustLayout )];
	
	if (self.hideTimer != nil)
	{
		[self.hideTimer invalidate];
	}
	
	self.hideTimer = [NSTimer scheduledTimerWithTimeInterval: HIDE_TIMEOUT
													  target: self 
													selector: @selector( hide ) 
													userInfo: nil 
													 repeats: NO];
}


- (void) hide
{
	if (self.popoverController != nil) 
	{
		if (self.hideTimer != nil)
		{
			[self.hideTimer invalidate];
		}
		
		self.hideTimer = nil;
		[self unschedule: @selector( adjustLayout )];

		[self.popoverController dismissPopoverAnimated: YES];
		self.popoverController = nil;
		self.contentViewController = nil;
		self.entityView = nil;
	}
}

@end
