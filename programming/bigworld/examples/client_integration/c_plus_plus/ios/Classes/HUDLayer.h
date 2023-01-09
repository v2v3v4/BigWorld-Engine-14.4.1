#import "cocos2d.h"

@class WorldUIModel;

@interface HUDLayer : CCLayer
{
	CCSprite * compass_;
	WorldUIModel * uiModel_;
}

- (id) initWithUIModel: (WorldUIModel *)model;

@end
