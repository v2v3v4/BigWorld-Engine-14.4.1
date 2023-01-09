#import "HUDLayer.h"
#import "WorldUIModel.h"

@interface HUDLayer (Private)

- (void) onCompassClick: (id) sender;

@end


@implementation HUDLayer

- (id) initWithUIModel: (WorldUIModel *)model
{
	if ((self = [super init]))
	{
		uiModel_ = [model retain];

		CCSprite * unselectedSprite = [CCSprite spriteWithFile:@"compasspin.png"];
		CCSprite * selectedSprite = [CCSprite spriteWithFile:@"compasspinsel.png"];
		CCSprite * overlay = [CCSprite spriteWithFile:@"compassstatic.png"];
		overlay.position = ccp( 35, 35 );
	
		compass_ = [CCMenu menuWithItems:[CCMenuItemImage itemFromNormalSprite:unselectedSprite selectedSprite:selectedSprite target:self selector:@selector(onCompassClick:)], nil];

		compass_.contentSize = CGSizeZero;
		compass_.position = ccp( 35, 35 );

		[self addChild:compass_];
		[self addChild:overlay];
		
		[uiModel_
         addObserver:self 
         forKeyPath:@"transform" 
         options:NSKeyValueObservingOptionNew
         context:NULL
         ];
	}

	return self;
}

- (void) dealloc
{
    [uiModel_ removeObserver:self forKeyPath:@"transform"];
	[uiModel_ release];
	[super dealloc];
}

- (void) onCompassClick: (id) sender
{
    // Extract scale, then build an unrotated and translated transform.
    float scale = uiModel_.scale;
    CGPoint pos = uiModel_.position;
    
    uiModel_.transform = CGAffineTransformMake( scale, 0, 0, scale, pos.x, pos.y );
    uiModel_.followPlayer = YES;
}

- (void) observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context
{
	if (object == uiModel_ && [keyPath isEqualToString:@"transform"])
	{
		compass_.rotation = CC_RADIANS_TO_DEGREES( uiModel_.rotation );
	}
}

@end
