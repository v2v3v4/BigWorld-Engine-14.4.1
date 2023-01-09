//
//  DeviceRotation.m
//  bigworld
//
//  Created by Thomas Cowell on 17/06/11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#import "DeviceRotation.h"


@implementation CCLayer( RotationNotification )

- (void) onDeviceRotated
{
}

- (void) registerForDeviceRotation
{
	// Use KVO to listen for when the projection property gets updated.
	// This gets done in CCDirectorIOS.layoutSubviews.
	[[CCDirector sharedDirector] 
	 addObserver:self 
	 forKeyPath:@"projection" 
	 options:NSKeyValueObservingOptionNew
	 context:NULL
	 ];
}

- (void) deregisterForDeviceRotation
{
	[[CCDirector sharedDirector] removeObserver:self forKeyPath:@"projection"];
}

- (void) observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context
{
	if ([keyPath isEqualToString:@"projection"])
	{
		[self onDeviceRotated];
	}
}

@end