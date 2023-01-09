// When you import this file, you import all the cocos2d classes
#import "cocos2d.h"

#import "TouchHandler.h"

@class WorldLayerTouchHandler;
@class WorldUIModel;

class EntityView;
class WorldLayerBWView;

namespace BW
{
	class BWSpaceDataListener;
}


// WorldLayer
@interface WorldLayer : CCLayer <TouchHandlerOwner>
{
	CCSprite * backgroundImage;

	WorldLayerTouchHandler * touchHandler;
	BW::BWSpaceDataListener * spaceDataListener;
	WorldUIModel * uiModel;
}

@property (nonatomic, retain) CCSprite * backgroundImage;
@property (nonatomic, retain) WorldLayerTouchHandler * touchHandler;
@property (nonatomic, readonly) WorldUIModel * uiModel;
@property (nonatomic, readonly) WorldLayerBWView * bwView;

- (id) initWithUIModel: (WorldUIModel *) model;

- (void)movePlayerTo: (CGPoint) location;

- (EntityView *) entityViewFromLocation: (CGPoint) location;

- (void) centreOn: (CGPoint) centrePosition;

@end
