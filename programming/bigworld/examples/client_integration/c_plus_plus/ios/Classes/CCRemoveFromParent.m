//
//  CCRemoveFromLayer.m
//  Balloons
//
//  Created by Thomas Cowell on 18/11/10.
//  Copyright 2010 __MyCompanyName__. All rights reserved.
//

#import "CCRemoveFromParent.h"


@implementation CCRemoveFromParent

+(id) action
{
	return [[[CCRemoveFromParent alloc] init] autorelease];
}

-(void) startWithTarget:(id)aTarget
{
	[super startWithTarget:aTarget];
	[((CCNode*)aTarget).parent removeChild:aTarget cleanup:YES];
}

@end
