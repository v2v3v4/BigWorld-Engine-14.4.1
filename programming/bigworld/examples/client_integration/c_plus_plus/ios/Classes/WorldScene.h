#import "cocos2d.h"

#import "EntityInfoLayer.h"

@class HUDLayer;
@class WorldLayer;
@class WorldUIModel;

@interface WorldScene : CCScene
{
}

@property (nonatomic, retain, readonly) WorldLayer * worldLayer;
@property (nonatomic, retain, readonly) HUDLayer * hudLayer;
@property (nonatomic, retain, readonly) EntityInfoLayer * entityInfoLayer;
@property (nonatomic, retain, readonly) WorldUIModel * uiModel;

+ (CCScene *) scene;
+ (WorldScene *) sharedWorldScene;

@end
