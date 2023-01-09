//
//  DeviceRotation.h
//  bigworld
//
//  Created by Thomas Cowell on 17/06/11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#import "cocos2d.h"


@interface CCLayer (RotationNotification)

- (void) registerForDeviceRotation;
- (void) deregisterForDeviceRotation;
- (void) onDeviceRotated;

@end