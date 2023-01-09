#import "WorldScene.h"

#import "WorldUIModel.h"
#import "WorldLayer.h"
#import "HUDLayer.h"
#import "EntityInfoLayer.h"

static WorldScene * _sharedWorldScene;

@interface WorldScene ()
@property (nonatomic, retain, readwrite) WorldLayer * worldLayer;
@property (nonatomic, retain, readwrite) HUDLayer * hudLayer;
@property (nonatomic, retain, readwrite) EntityInfoLayer * entityInfoLayer;
@property (nonatomic, retain, readwrite) WorldUIModel * uiModel;
@end

@implementation WorldScene

@synthesize worldLayer;
@synthesize hudLayer;
@synthesize entityInfoLayer;
@synthesize uiModel;


+ (CCScene *) scene
{
	return [self node];
}


+ (WorldScene *) sharedWorldScene
{
	if (!_sharedWorldScene)
	{
		[[WorldScene alloc] init];
	}
	return _sharedWorldScene;
}

- (id) init
{
	if ((self = [super init]))
	{
		if (_sharedWorldScene && _sharedWorldScene != self)
		{
			[_sharedWorldScene release];
		}
		
		_sharedWorldScene = self;

		self.uiModel = [[[WorldUIModel alloc] init] autorelease];

		self.worldLayer = [[[WorldLayer alloc] initWithUIModel: uiModel] autorelease];
		self.hudLayer = [[[HUDLayer alloc] initWithUIModel: uiModel] autorelease];
		self.entityInfoLayer = [[[EntityInfoLayer alloc] init] autorelease];

		[self addChild:self.worldLayer z:0];
		[self addChild:self.hudLayer z:10];
		[self addChild:self.entityInfoLayer z:20];
	}

	return self;
}


- (void) dealloc
{
	self.uiModel = nil;

	self.worldLayer = nil;
	self.hudLayer = nil;
	self.entityInfoLayer = nil;

	if (_sharedWorldScene == self)
	{
		_sharedWorldScene = nil;
	}

	[super dealloc];
}

@end
