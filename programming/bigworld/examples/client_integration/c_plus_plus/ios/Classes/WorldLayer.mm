#import "ApplicationDelegate.h"
#import "WorldLayer.h"
#import "EntityInfoLayer.h"
#import "EntityController.h"
#import "EntityView.hpp"
#import "MoveEntityController.h"
#import "TouchHandler.h"
#import "WorldLayerNoTouchHandler.h"
#import "WorldUIModel.h"

//extern "C" void CGAffineToGL( const CGAffineTransform * t, GLfloat * m );
#import "TransformUtils.h"

#include "connection_model/bw_entities.hpp"
#include "connection_model/bw_server_connection.hpp"
#include "connection_model/bw_space_data_listener.hpp"


@interface WorldLayer()
{
}

- (BW::BWConnection *) connection;

- (void) onGeometryMappingForSpace: (BW::SpaceID) aSpaceID 
						  withPath: (NSString *) aPath;

- (void) onTimeOfDayForSpace: (BW::SpaceID) aSpaceID 
			 withInitialTime: (float) anInitialTime
			withGameTimeRate: (float) theGameSecondsPerSecond;

- (void) onUserSpaceData: (BW::SpaceID) aSpaceID 
				 withKey: (BW::uint16) aKey 
				 andData: (NSData *) theData;
@end


class WorldLayerSpaceDataListener : public BW::BWSpaceDataListener
{
public:
	
	/**
	 *	Constructor.
	 */
	WorldLayerSpaceDataListener( WorldLayer * worldLayer ):
		worldLayer_( worldLayer )
	{
		[worldLayer_ retain];
	}
	
	
	/**
	 *	Destructor.
	 */
	virtual ~WorldLayerSpaceDataListener()
	{
		[worldLayer_ release];
	}
	
	
	// Overrides from BWSpaceDataListener.
	
	
	virtual void onGeometryMapping( BW::SpaceID spaceID,
			BW::Matrix mappingMatrix, const BW::string & mappingName )
	{
		NSString * mappingNameString =
			[NSString stringWithUTF8String: mappingName.c_str()];

		[worldLayer_ onGeometryMappingForSpace: spaceID
									  withPath: mappingNameString];
	}
	
	virtual void onTimeOfDay( BW::SpaceID spaceID, 
							 float initialTime, float gameSecondsPerSecond )
	{
		[worldLayer_ onTimeOfDayForSpace: spaceID
						 withInitialTime: initialTime
						withGameTimeRate: gameSecondsPerSecond];
	}
	
	virtual void onUserSpaceData( BW::SpaceID spaceID, BW::uint16 key,
		bool isInsertion, const BW::string & data )
	{}
	
	
private:
	WorldLayer * worldLayer_;
};




@interface BackgroundLoader : NSObject
{
	NSString * mapping;
	WorldLayer * layer;
}

+ (void) loadBackground: (NSString *) mapping 
			   forLayer: (WorldLayer *) worldLayer;

- (id) initWithBackground: (NSString *) aMapping 
				 forLayer: (WorldLayer *) aLayer;

- (void) onTextureLoaded: (CCTexture2D *) texture;
- (void) onDetailTextureLoaded: (CCTexture2D *) texture;

@end


@implementation BackgroundLoader

+ (void) loadBackground: (NSString *) mapping 
			   forLayer: (WorldLayer *) worldLayer
{
	BackgroundLoader * loader = [[self alloc] initWithBackground: mapping
														forLayer: worldLayer];
	
	CCTextureCache * cache = [CCTextureCache sharedTextureCache];
	[cache addImageAsync: @"minspec.pvr" 
				  target: loader 
				selector: @selector( onTextureLoaded: )];
	
    [cache addImageAsync: @"minspec_detail.pvr" 
				  target: loader 
				selector: @selector( onDetailTextureLoaded: )];
}


- (id) initWithBackground: (NSString *) aMapping 
				 forLayer: (WorldLayer *) aLayer
{
	if ((self = [super init]))
	{
		mapping = [aMapping retain];
		layer = [aLayer retain];
	}

	return self;
}


- (void) dealloc
{
	[mapping release];
	[layer release];
	
	[super dealloc];
}


- (void) onTextureLoaded: (CCTexture2D *) texture
{
    CCSprite * image = [CCSprite spriteWithTexture: texture];
	CGRect spaceBound = CGRectMake( -1200.f, -1200.f, 2500.f, 2500.f );
    image.anchorPoint = ccp( 0, 0 );
	image.scaleX = spaceBound.size.width / image.contentSize.width;
	image.scaleY = spaceBound.size.height / image.contentSize.height;
	CGPoint offset = ccpAdd( spaceBound.origin,
							ccpMult( ccpFromSize( spaceBound.size ), 0.5 ) );

    image.position = ccp( -spaceBound.size.width / 2 + offset.x, 
						 -spaceBound.size.height / 2 + offset.y );
    
	[layer addChild: image];
}

- (void) onDetailTextureLoaded: (CCTexture2D *) texture
{
    CCSprite * image = [CCSprite spriteWithTexture: texture];
	CGRect spaceBound = CGRectMake( -300.f, -400.f, 800.f, 800.f );
	image.scaleX = spaceBound.size.width / image.contentSize.width;
	image.scaleY = spaceBound.size.height / image.contentSize.height;
	image.position = ccpAdd( spaceBound.origin,
							ccpMult( ccpFromSize( spaceBound.size ), 0.5 ) );
    
    image.position = ccpAdd( image.position, ccp(-200, 200) );
    [layer addChild: image 
				  z: 1];
}

@end



// WorldLayer implementation
@implementation WorldLayer

@synthesize backgroundImage;
@synthesize touchHandler;
@synthesize uiModel;



- (id) initWithUIModel: (WorldUIModel *) aModel
{
	if ((self = [super init]))
	{
		uiModel = [aModel retain];
		
		BW::BWConnection * connection = [self connection];
		
		touchHandler = [[WorldLayerNoTouchHandler alloc] initWithLayer: self];
		self.isTouchEnabled = YES;
		
        [self setContentSize: CGSizeMake( 2500.0f / 2, 2500.0f / 2 )];
		self.anchorPoint = ccp( 0.f, 0.f );
        
        [self scheduleUpdate];


		spaceDataListener = new WorldLayerSpaceDataListener( self );
		connection->addSpaceDataListener( *spaceDataListener );
        
        [uiModel addObserver: self 
				  forKeyPath: @"transform" 
					 options: NSKeyValueObservingOptionNew
					 context: NULL];

    }

	return self;
}


- (void) onEnter
{
	// register to receive targeted touch events
	[[CCTouchDispatcher sharedDispatcher] addTargetedDelegate: self
													 priority: 0
											  swallowsTouches: YES];
	[super onEnter];
}

- (void) onExit
{
	[super onExit];
	
	// register to receive targeted touch events
	[[CCTouchDispatcher sharedDispatcher] removeDelegate: self];

}

- (void) observeValueForKeyPath: (NSString *) keyPath
					   ofObject: (id) object 
						 change: (NSDictionary *) change 
						context: (void *) context
{
	if ((object == uiModel) && [keyPath isEqualToString: @"transform"])
	{
		self.position = uiModel.position;
        self.scale = uiModel.scale;
        self.rotation = CC_RADIANS_TO_DEGREES( uiModel.rotation );
	}
}


- (void) dealloc
{
    [uiModel removeObserver: self
				 forKeyPath: @"transform"];
    
	[uiModel release];
	
	self.backgroundImage = nil;

	[self connection]->removeSpaceDataListener( *spaceDataListener );
	delete spaceDataListener;

	[touchHandler release];

	[super dealloc];
}


- (void) centreOn: (CGPoint) centrePosition
{
    float scale = ccpLength( ccp(uiModel.transform.a, uiModel.transform.b) );

    CGSize size = [[CCDirector sharedDirector] winSize];

    CGPoint viewOffset = ccpMult( ccp( size.width, size.height ), 
								 0.5f / scale );
    centrePosition = ccpSub( centrePosition, viewOffset );

    CGAffineTransform transform = CGAffineTransformIdentity;
    transform = CGAffineTransformScale( transform, scale, scale );
    transform.tx = -centrePosition.x*scale;
    transform.ty = -centrePosition.y*scale;

    uiModel.transform = transform;
}


- (void) update: (ccTime) dt
{
	BW::BWConnection * connection = [self connection];

	const BW::BWEntities & entities = connection->entities();

	BW::BWEntity * pPlayer = entities.pPlayer();

    if (pPlayer)
	{
		const BW::Position3D position = pPlayer->position();
		
		if (uiModel.followPlayer)
		{
			[self centreOn: ccp( position.x, position.z )];
			
			// TODO: Re-enable rotating the player somehow so it fits in with the controls.
			//[player.controller onTurnTo:CC_DEGREES_TO_RADIANS(-self.rotation)];
		}
	}
}


- (BOOL)ccTouchBegan: (UITouch *) touch 
		   withEvent: (UIEvent *) event
{
	return [touchHandler touchBegan: touch 
						  withEvent: event 
						   andOwner: self];
}


- (void)ccTouchMoved: (UITouch *) touch 
		   withEvent: (UIEvent *) event
{
	[touchHandler touchMoved: touch 
				   withEvent: event 
					andOwner: self];
}


- (void)ccTouchEnded: (UITouch *) touch 
		   withEvent: (UIEvent *) event
{
	[touchHandler touchEnded: touch 
				   withEvent: event 
					andOwner: self];
}


- (void)movePlayerTo: (CGPoint) destination
{
	BW::BWConnection * connection = [self connection];

	destination = [self convertToNodeSpace: destination];
	BW::BWEntityPtr pPlayer = connection->entities().pPlayer();

	if (!pPlayer.exists())
	{
		return;
	}
	
	const BW::Position3D position = pPlayer->position();
	BW::Vector3 currentLocation( position.x, 0.f, position.z );
	BW::Vector3 destinationLocation( destination.x, 0.f, destination.y );
	
	BW::Vector3 dir = destinationLocation - currentLocation;
	
	MoveEntityController * controller = 
		[[MoveEntityController alloc] initWithPosition: destinationLocation
												   yaw: dir.yaw()];
	
	[[ApplicationDelegate sharedDelegate] setPlayerController: controller];
}


- (EntityView *) entityViewFromLocation: (CGPoint)location
{
    location = [self convertToNodeSpace: location];
	
	EntityView * closestEntityView = NULL;
	float closestDist = FLT_MAX;
	//int highestZ = -1000;
	
	NSArray * hitViews =
		[[ApplicationDelegate sharedDelegate]
			entityViewsFromLocation: location];
	
	for (NSValue * value in hitViews)
	{
		EntityView * pEntityView = (EntityView *)[value pointerValue];
		const BW::BWEntity * pEntity = pEntityView->pEntity();
		const BW::Vector3 & position = pEntity->position();
		// TODO: Get an actual Z value from sprite (currently all == 0)
		CGPoint cgPos = ccp( position.x, position.z );
		float dist = ccpDistance( location, cgPos );

		if (dist < closestDist)
		{
			closestDist = dist;
			closestEntityView = pEntityView;
		}
	}

	return closestEntityView;
}


- (BW::BWConnection *) connection
{
	return [[ApplicationDelegate sharedDelegate] connection];
}


// -----------------------------------------------------------------------------
// Section: BWSpaceDataListener protocol
// -----------------------------------------------------------------------------

- (void) onGeometryMappingForSpace: (BW::SpaceID) aSpaceID 
						  withPath: (NSString *) aPath
{
	NSLog( @"onGeometryMapping %@\n", aPath );
	[BackgroundLoader loadBackground: aPath 
							forLayer: self];
}


- (void) onTimeOfDayForSpace: (BW::SpaceID) aSpaceID 
			 withInitialTime: (float) anInitialTime
			withGameTimeRate: (float) theGameSecondsPerSecond;
{
	NSLog( @"onTimeOfDay: %.2f, %.2f", anInitialTime, theGameSecondsPerSecond );
}


- (void) onUserSpaceData: (BW::SpaceID) aSpaceID 
				 withKey: (BW::uint16) aKey 
				 andData: (NSData *) theData
{
	NSLog( @"onUserSpaceData: key = %hu", aKey );
}


@end
