//
//  SpriteSheetManager.m
//  Balloons
//
//  Created by license on 30/11/10.
//  Copyright 2010 __MyCompanyName__. All rights reserved.
//

#import "SpriteSheetManager.h"

@interface SpriteSheetManager ()
@property (retain, nonatomic) NSMutableDictionary* animFrames_;
@end

@implementation SpriteSheetManager

@synthesize animFrames_;

//
// singleton stuff
//
static SpriteSheetManager *_sharedManager = nil;

+(SpriteSheetManager *)sharedManager
{
	if (!_sharedManager) 
	{		
		_sharedManager = [[self alloc] init];
	}
	
	return _sharedManager;
}

-(id)init
{
	if( (self = [super init]) )
	{
		self.animFrames_ = [[[NSMutableDictionary alloc] init] autorelease];
	}
	return self;
}

-(void)dealloc
{
	self.animFrames_ = nil;
	[super dealloc];
}

-(void)addSpriteSheet:(NSString*)plist
{
	[[CCSpriteFrameCache sharedSpriteFrameCache] addSpriteFramesWithFile:plist];
}

// NOTE: we are assuming that we're starting at frame 0000.
-(BOOL) loadAnimation:(NSString*)animName
{
	// if the animation already exists, then we don't want to re-add it.
	// return YES as it is already sucessful.
	if ([self.animFrames_ objectForKey:animName])
		return YES;
	
	NSMutableArray* frames = [NSMutableArray array];
	int frameNum = 0;
	while(YES)
	{
		// keep grabbing frames for this animation 
		CCSpriteFrame* frame =
		[[CCSpriteFrameCache sharedSpriteFrameCache] spriteFrameByName:
		 [NSString stringWithFormat:@"%@_%04.04d.png", animName, frameNum]
		 ];
		
		if (frame == nil)
		{
			break;
		}
		else
		{
			[frames addObject:frame];
			frameNum += 1;
		}
	}
	
	if ([frames count] == 0)
	{
		// if we can't find the animation  throw up a log msg.
		CCLOG( @"ERROR: Found no animation frames for animation '%@'", animName );
		return NO;
	}
	
	[self.animFrames_ setObject:frames forKey:animName];
	
	return YES;
}

-(NSString*) loadAnimationForEntity:(NSString*)entityName withAnimationName:(NSString*)aniName inDirection:(float)rotation
{
	// add the offset and make sure it's within the positive range still
	rotation -= 125.0f;
	if (rotation < 0)
		rotation += 360.0f;
	
	// used to snap to one of the four directions (x needs to be positive for this to work co)
	float direction = fmodf(rotation, 90.0f);
	
	// make the sprite one of 4 directions.
	uint newDirection = (uint)((int)(rotation + (direction > 45.0f ? 90.0f-direction : -direction)) / 90);
	
	// NOTE: frames start at 1 and 360 is the same as 0
	if (newDirection >= 4)
		newDirection = 0;
	
	NSString* animationString = [NSString stringWithFormat:@"%@/%@_%i", entityName, aniName, newDirection];
	
	BOOL success = [self loadAnimation:animationString];
	
	if (!success)
	{
		NSString* defaultKey = [NSString stringWithFormat:@"%@/%@_0", entityName, aniName];
		success = [self loadAnimation:defaultKey];
		if (success)
			CCLOG( @"ERROR: Using default animation '%@' for '%@'", defaultKey, animationString );
		else
			CCLOG( @"ERROR: Unable to load default animation '%@' for '%@'!!!!", defaultKey, animationString );
		
		// note we're adding a reference to the default animation's frames under the current animation name so it doesn't fail everytime
		// and will grab a usable animation
		NSMutableArray* frames = [self.animFrames_ objectForKey:defaultKey];
		if (frames)
			[self.animFrames_ setObject:frames forKey:animationString];
	}
	
	return animationString;
}

- (void) purgeUnused
{
	// remove all animations which only have a reference of one. basically copypaste of the CCSpriteFrameCache removeUnusedSpriteFrames function.
	// NOTE: this will NOT remove any default animations which may be used for more than one animation.
	// TODO: possibly search through the dictionary and find all instances and remove all of them if the ref count is the same as the ani count
	NSArray *keys = [self.animFrames_ allKeys];
	for( NSString* aniKey in keys )
	{
		NSMutableArray* value = [self.animFrames_ objectForKey:aniKey];		
		if( [value retainCount] == 1 )
		{
			CCLOG(@"SpriteManager: removing unused animation: %@", aniKey);
			[self.animFrames_ removeObjectForKey:aniKey];
		}
	}
	
	[[CCSpriteFrameCache sharedSpriteFrameCache] removeUnusedSpriteFrames];
}

-(CCAnimation*) makeAnimation:(NSString*)animName fps:(float)fps
{
	NSMutableArray* animFrames = [self.animFrames_ objectForKey:animName];
	if (!animFrames)
	{
		CCLOG( @"SpriteSheetManager: animation '%@' not defined.", animName );
		return nil;
	}
	
	return [CCAnimation animationWithFrames:animFrames delay:(1.0f/fps)];
}

@end
