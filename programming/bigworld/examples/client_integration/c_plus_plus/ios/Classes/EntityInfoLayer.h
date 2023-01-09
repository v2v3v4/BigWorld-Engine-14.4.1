//
//  EntityInfoLayer.h
//  bigworld
//
//  Created by Thomas Cowell on 15/06/11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "cocos2d.h"

#import "EntityInfoViewController.h"

#import "WEPopoverController.h"

class EntityView;

@interface EntityInfoLayer : CCLayer 
{
}

@property (nonatomic, retain) EntityInfoViewController* contentViewController;
@property (nonatomic, retain) WEPopoverController* popoverController;
@property (nonatomic, assign) EntityView * entityView;

@property (nonatomic, retain) NSTimer* hideTimer;

@property (nonatomic, assign) UIInterfaceOrientation interfaceOrientation;

@property (nonatomic, assign) BOOL shouldUpdateMessage;

- (void) adjustLayout;
- (void) updateForEntityView: (EntityView *) anEntityView
				 withMessage: (NSString *) msg;
- (void) updateForEntityView: (EntityView *) anEntityView;
- (void) show;
- (void) hide;

@end
