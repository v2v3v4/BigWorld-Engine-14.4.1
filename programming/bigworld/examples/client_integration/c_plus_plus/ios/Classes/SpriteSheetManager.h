//
//  SpriteSheetManager.h
//  Balloons
//
//  Created by license on 30/11/10.
//  Copyright 2010 __MyCompanyName__. All rights reserved.
//

// copied from the PR project to manage spritesheets.

#import <Foundation/Foundation.h>
#import "cocos2d.h"

@interface SpriteSheetManager : NSObject 

+(SpriteSheetManager*) sharedManager;

-(void) addSpriteSheet:(NSString*)plist;
-(BOOL) loadAnimation:(NSString*)animName;

-(NSString*) loadAnimationForEntity:(NSString*)entityName withAnimationName:(NSString*)aniName inDirection:(float)rotation;

-(CCAnimation*) makeAnimation:(NSString*)animName fps:(float)fps;

-(void) purgeUnused;
@end
