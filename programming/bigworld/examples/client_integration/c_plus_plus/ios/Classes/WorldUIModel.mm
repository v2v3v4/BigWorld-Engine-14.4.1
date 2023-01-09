#import "WorldUIModel.h"
#import "cocos2d.h"

float clamp( float minValue, float value, float maxValue )
{
	return MIN( MAX( value, minValue ), maxValue );
}


@implementation WorldUIModel

@synthesize entityScale=entityScale_;
@synthesize followPlayer=followPlayer_;
@synthesize transform=transform_;
@synthesize position;
@synthesize scale;
@synthesize rotation;

- (id) init
{
	if ((self = [super init]))
	{
		entityScale_ = 0.2f;    // initial size
        followPlayer_ = YES;
        transform_ = CGAffineTransformIdentity;
	}

	return self;
}

// how big the entity sprites are
- (void) setEntityScale:(float)newScale
{
	const float MIN_ENTITY_SCALE = 0.03f;
	const float MAX_ENTITY_SCALE = 0.5f;
	
	entityScale_ = clamp( newScale, MIN_ENTITY_SCALE, MAX_ENTITY_SCALE );
}

- (CGPoint) position
{
    return CGPointApplyAffineTransform( ccp(0,0), transform_ );
}

- (float) scale
{
    return ccpLength( ccp( transform_.a, transform_.b ) );
}

- (float) rotation
{
    return -atan2( transform_.b, transform_.a );
}

@end
