#import "cocos2d.h"

#import "ApplicationDelegate.h"
#import "MainMenuViewController.h"
#import "SpriteSheetManager.h"

#import "BlankMenuLayer.h"
#import "WorldScene.h"

#import "RootViewController.h"

#include "DefsDigest.hpp"

#include "EntityGameLogicFactory.hpp"
#include "EntityViewExtensionFactory.hpp"
#include "EntityView.hpp"
#include "IOSClientFilterEnvironment.h"

#include "connection/condemned_interfaces.hpp"
#include "connection/login_challenge_factory.hpp"
#include "connection/server_connection_handler.hpp"
#include "connection_model/bw_entities.hpp"
#include "connection_model/bw_connection_helper.hpp"
#include "connection_model/simple_space_data_storage.hpp"
#include "network/space_data_mappings.hpp"


NSString * kDefaultServer = @"eval.bigworldtech.com:23201";


@interface ApplicationDelegate()
{
	BW::BWServerConnection *		pConnection;
	EntityFactory *					pEntityFactory;
	BW::SimpleSpaceDataStorage *	pSpaceDataStorage;
	EntityGameLogicFactory *		pEntityGameLogicFactory;
	EntityViewExtensionFactory *	pEntityViewFactory;
	BW::ServerConnectionHandler *	pConnectionHandler;
	ConnectionUpdater *				connectionUpdater;
	id								playerController;
	std::set< EntityView * >		entityViews;
	IOSClientFilterEnvironment *	pFilterEnvironment;
	BW::BWConnectionHelper *		pConnectionHelper;
}

- (void) initCocos;
- (void) initBigWorld;

- (void) onLoggedOn;
- (void) onLoggedOff;
- (void) onLogOnFailure: (NSString *) msg;
- (void) update: (ccTime) deltaTime;

- (IBAction) handleInfoButton: (id) sender;

@end


namespace // anonymous
{

class AppDelegateBWConnectionHandler : public BW::ServerConnectionHandler
{
public:
	AppDelegateBWConnectionHandler(): 
		ServerConnectionHandler()
	{}
	
	virtual ~AppDelegateBWConnectionHandler() {}
	
	// Overrides from ServerConnectionHandler
	
	virtual void onLoggedOff();
	virtual void onLoggedOn();
	virtual void onLogOnFailure( const BW::LogOnStatus & status,
		const BW::string & message );
private:
};


void AppDelegateBWConnectionHandler::onLoggedOff()
{
	[[ApplicationDelegate sharedDelegate] onLoggedOff];
}


void AppDelegateBWConnectionHandler::onLoggedOn()
{
	[[ApplicationDelegate sharedDelegate] onLoggedOn];
}


void AppDelegateBWConnectionHandler::onLogOnFailure(
		const BW::LogOnStatus & status,
		const BW::string & messageString )
{
	NSString * message = [NSString stringWithFormat: @"%s (%d)",
		messageString.c_str(), (int)status.value()];
	
	[[ApplicationDelegate sharedDelegate] onLogOnFailure: message];
}


} // end namespace (anonymous)


@interface ConnectionUpdater : NSObject 
{
}

- (id) init;
- (void) update: (ccTime) dt;

@end

@implementation ConnectionUpdater

- (id) init
{
	if ((self = [super init]))
	{
		[[CCScheduler sharedScheduler] scheduleUpdateForTarget: self 
													  priority: -1000 
														paused: NO];
	}
	
	return self;
}

- (void) cancel
{
	[[CCScheduler sharedScheduler] unscheduleUpdateForTarget: self];
}

- (void) update: (ccTime) deltaTime
{
	[[ApplicationDelegate sharedDelegate] update: deltaTime];
}

@end



@implementation ApplicationDelegate

@synthesize serverName;
@synthesize defaultServerName;
@synthesize window;
@synthesize shouldShowResetServer;
@synthesize infoView;
@synthesize infoButton;

- (BW::BWServerConnection *)connection
{
	return pConnection;
}


- (BW::FilterEnvironment *) filterEnvironment
{
	return pFilterEnvironment;
}


+ (ApplicationDelegate *) sharedDelegate
{
	return (ApplicationDelegate *)[[UIApplication sharedApplication] delegate];
}


- (void) applicationDidFinishLaunching: (UIApplication*) application
{
	// We want notifications on device rotation
	[[UIDevice currentDevice] beginGeneratingDeviceOrientationNotifications];
	[[UIApplication sharedApplication] setIdleTimerDisabled:YES];
	
	// used to check to see if we're running an iPad
	isIpad = (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad);
	
	NSUserDefaults * defaults = [NSUserDefaults standardUserDefaults];

	self.serverName = [defaults stringForKey: @"default_server"];
	self.defaultServerName = [defaults stringForKey: @"default_server"];
	NSString * lastDefaultServer = [defaults stringForKey:@"last_default_server"];

	// if this is the first time the app has been launched, set the value of the
	// last default server.
	if (lastDefaultServer == nil)
	{
		lastDefaultServer = kDefaultServer;
		[defaults setObject:kDefaultServer forKey:@"last_default_server"];
		[defaults synchronize];
	}
	
	if (self.serverName == nil ||
		[defaults boolForKey: @"reset_default_server"] || 
		[lastDefaultServer caseInsensitiveCompare:kDefaultServer] != NSOrderedSame)
	{
		[self resetDefaultServer];
		// set the current Default server
		[defaults setObject:kDefaultServer forKey:@"last_default_server"];
		[defaults synchronize];
	}
	
	[self initCocos];

	[self initBigWorld];

}

- (void) initCocos
{
	
	// Init the window
	window = [[UIWindow alloc] initWithFrame: [[UIScreen mainScreen] bounds]];
	
	// Try to use CADisplayLink director
	// if it fails (SDK < 3.1) use the default director
	if( ![CCDirector setDirectorType: kCCDirectorTypeDisplayLink])
	{
		[CCDirector setDirectorType: kCCDirectorTypeDefault];
	}
	
	CCDirector * director = [CCDirector sharedDirector];
	
	// Init the View Controller
	viewController = [[RootViewController alloc] initWithNibName: nil 
														  bundle: nil];
	viewController.wantsFullScreenLayout = YES;
	
	//
	// Create the EAGLView manually
	//  1. Create a RGB565 format. Alternative: RGBA8
	//	2. depth format of 0 bit. Use 16 or 24 bit for 3d effects, like 
	//	   CCPageTurnTransition
	//
	//
	EAGLView *glView = 
	[EAGLView viewWithFrame: [window bounds]
				pixelFormat: kEAGLColorFormatRGB565 // kEAGLColorFormatRGBA8
				depthFormat: 0	// GL_DEPTH_COMPONENT16_OES
	 ];
	
	[glView setMultipleTouchEnabled: YES];
	// attach the openglView to the director
	[director setOpenGLView: glView];
	
	// Enables High Res mode (Retina Display) on iPhone 4 and maintains low res 
	// on all other devices
	if (![director enableRetinaDisplay: YES])
	{
		CCLOG(@"Retina Display Not supported");
	}
	
	// init the spritesheet manager with our sprites.
	[[SpriteSheetManager sharedManager] addSpriteSheet: @"EntitySprites.plist"];
	
	[director setAnimationInterval: 1.0 / 60];
	[director setDisplayFPS: YES];
	
	
	// Default texture format for PNG/BMP/TIFF/JPEG/GIF images
	// It can be RGBA8888, RGBA4444, RGB5_A1, RGB565
	// You can change anytime.
	[CCTexture2D setDefaultAlphaPixelFormat: kCCTexture2DPixelFormat_RGBA8888];

	// Run the intro Scene
	[[CCDirector sharedDirector] runWithScene: [BlankMenuLayer scene]];
	
	self.infoButton = [UIButton buttonWithType: UIButtonTypeInfoLight];
	
	[self.infoButton addTarget: self 
						action: @selector( handleInfoButton: ) 
			  forControlEvents: UIControlEventTouchUpInside];
	
	// make the OpenGLView a child of the view controller
	viewController.view = glView;
	
	// setup our views.
	menuViewController = [[MainMenuViewController alloc]
						  initWithNibName:(isIpad ? @"MainMenuiPadView" : @"MainMenuView")
						  bundle:nil];
	serverListViewController = [[ServerListViewController alloc]
								initWithNibName:(isIpad ? @"ServerListiPadView" : @"ServerListView")
								bundle:nil];
	
	[viewController.view addSubview: self.infoButton];
	
	// swap the bounds to landscape
	CGRect boundsRect = CGRectMake( viewController.view.bounds.origin.y,
								   viewController.view.bounds.origin.x,
								   viewController.view.bounds.size.height,
								   viewController.view.bounds.size.width );
	
	[self layoutInfoButton: boundsRect];
	
	self.infoView = [[[InfoView alloc] 
					  initWithFrame: boundsRect] 
					 autorelease];
	
	// add the menu view controllers
	[viewController.view addSubview: menuViewController.view];
	[viewController.view addSubview: serverListViewController.view];
	// use this to show the relevant view.
	[self showMenuScreen];
	// make the View Controller a child of the main window
	[window addSubview: viewController.view];
	
	[window makeKeyAndVisible];

}


- (void) initBigWorld
{
	pEntityFactory = new EntityFactory();
	pSpaceDataStorage = new BW::SimpleSpaceDataStorage( new BW::SpaceDataMappings() );

	pConnectionHelper = new BW::BWConnectionHelper(
		*pEntityFactory,
		*pSpaceDataStorage,
		DefsDigest::constants() );
		
	pEntityGameLogicFactory = new EntityGameLogicFactory();
	pEntityFactory->addExtensionFactory( 
		pEntityGameLogicFactory );

	pEntityViewFactory = new EntityViewExtensionFactory();
	pEntityFactory->addExtensionFactory( 
		pEntityViewFactory );

	pConnection = pConnectionHelper->createServerConnection( 0.0 );
	
	NSString * loginAppKeyPath = [[NSBundle mainBundle] pathForResource: @"loginapp" 
																 ofType: @"pubkey"];
	
	pConnection->setLoginAppPublicKeyPath( [loginAppKeyPath UTF8String] );
	
	pConnectionHandler = new AppDelegateBWConnectionHandler();
	
	pConnection->setHandler( pConnectionHandler );	

	connectionUpdater = [[ConnectionUpdater alloc] init];
	
	pFilterEnvironment = new IOSClientFilterEnvironment( pConnection->entities() );
}


- (void) addEntityView:(EntityView *)pView
{
	entityViews.insert( pView );
}


- (void) removeEntityView:(EntityView *)pView
{
	entityViews.erase( pView );
}


- (void) setPlayerController: (id) aController
{
	if (playerController != nil)
	{
		[playerController release];
	}
	
	[aController retain];
	playerController = aController;
}


- (NSArray *) entityViewsFromLocation: (CGPoint) location
{
	NSMutableArray * hitViews = [NSMutableArray array];
	for (std::set< EntityView * >::const_iterator iView = entityViews.begin();
		 iView != entityViews.end();
		 ++iView)
	{
		if ((*iView)->hitTest( location ))
		{
			[hitViews addObject: [NSValue valueWithPointer: *iView]];
		}
	}
	
	return hitViews;
}


- (void) entityViewWasClicked: (EntityView *) pEntityView
{
	pEntityView->handleClick();

	WorldScene * worldScene = [WorldScene sharedWorldScene];
	[worldScene.entityInfoLayer updateForEntityView: pEntityView];
	[worldScene.entityInfoLayer show];
}


- (void) update: (ccTime) deltaTime
{
	pConnectionHelper->update( pConnection, deltaTime );

	if ([playerController updateForEntity: pConnection->pPlayer()
									   by: deltaTime])
	{
		playerController = nil;
	}
	
	std::set< EntityView * >::const_iterator iView = entityViews.begin();
	while (iView != entityViews.end())
	{
		(*iView)->update();
		++iView;
	}

	pConnectionHelper->updateServer( pConnection );
}


- (void) resetDefaultServer
{
	NSUserDefaults * defaults = [NSUserDefaults standardUserDefaults];
	[defaults setValue: kDefaultServer 
				forKey: @"default_server"];
	
	[defaults setObject: [NSNumber numberWithBool: NO]
				 forKey: @"reset_default_server"];
	[defaults synchronize];
	self.serverName = kDefaultServer;
	self.defaultServerName = kDefaultServer;
}

- (void) showMenuScreen
{
	[menuViewController.view setHidden:YES];
	[serverListViewController.view setHidden:YES];
	
	// show the relevent view
	if ([[NSUserDefaults standardUserDefaults] boolForKey:@"show_local_servers"])
	{
		[serverListViewController showServerList];
	}
	else
	{
		[menuViewController showMainMenu];
	}
}

- (void) hideMenuScreen
{
	[menuViewController hideMainMenu];
	[serverListViewController hideServerList];
}

- (void) showLoginMenu
{
	[menuViewController showMainMenu];
}

- (void) hideLoginMenu
{
	[menuViewController hideMainMenu];
}

- (void) showServerListMenu
{
	[serverListViewController showServerList];
}

- (void) hideServerListMenu
{
	[serverListViewController hideServerList];
}


- (void) onEndLogin
{
	[[CCDirector sharedDirector] replaceScene: [WorldScene sharedWorldScene]];

	[menuViewController onEndLogin];
	[serverListViewController onEndLogin];
}

- (void) beginBackgroundLogoff
{
	UIApplication *app = [UIApplication sharedApplication];
	
	self->logoffTask = [app beginBackgroundTaskWithExpirationHandler:^{ 
		[app endBackgroundTask:logoffTask]; 
		logoffTask = UIBackgroundTaskInvalid;
	}];
	
	pConnection->logOff();
}

- (void) applicationWillResignActive: (UIApplication *) application
{
	[[CCDirector sharedDirector] pause];
	[self beginBackgroundLogoff];
}

- (void) resetConnection
{
}


- (void) applicationDidBecomeActive: (UIApplication *) application 
{
	[[CCDirector sharedDirector] resume];
}


- (void) applicationDidReceiveMemoryWarning: (UIApplication *) application 
{
	[[CCDirector sharedDirector] purgeCachedData];
}


- (void) applicationDidEnterBackground: (UIApplication *) application 
{
	[[CCDirector sharedDirector] stopAnimation];
}


- (void) applicationWillEnterForeground: (UIApplication *) application 
{
	pConnection->logOff();
	[[CCDirector sharedDirector] startAnimation];
	
	NSUserDefaults * defaults = [NSUserDefaults standardUserDefaults];
	
	[defaults synchronize];
	
	// set the server back to the initial server, if we swaped from local servers
	if (![defaults boolForKey: @"show_local_servers"])
	{
		self.serverName = [defaults stringForKey: @"default_server"];
	}
	self.defaultServerName = [defaults stringForKey: @"default_server"];
	
	if (self.serverName == nil || [defaults boolForKey: @"reset_default_server"])
	{
		[self resetDefaultServer];
	}
	
	// reload the menu when we re-enter the app if needed.
	if (!menuViewController.view.hidden || !serverListViewController.view.hidden)
	{
		[self onEndLogin];
		[self showMenuScreen];
	}
}


- (void) applicationWillTerminate: (UIApplication *) application 
{
	pConnection->logOff();
	
	CCDirector * director = [CCDirector sharedDirector];
	
	[[director openGLView] removeFromSuperview];
	
	[viewController release];
	
	[window release];
	
	[menuViewController release];
	[serverListViewController release];
	
	[director end];	
}


- (void) applicationSignificantTimeChange: (UIApplication *) application 
{
	[[CCDirector sharedDirector] setNextDeltaTimeZero: YES];
}


- (void) dealloc 
{
	self.infoView = nil;
	self.infoButton = nil;
	
	[[CCDirector sharedDirector] release];
	[window release];
	
	delete pFilterEnvironment;
	delete pEntityFactory;
	delete pSpaceDataStorage;
	delete pConnectionHandler;
	delete pConnection;
	delete pConnectionHelper;

	[connectionUpdater cancel];
	[connectionUpdater release];
	
	[super dealloc];
}

- (void) onLoggedOn
{
	NSLog( @"onLoggedOn" );
}



- (void) onLoggedOff
{
	NSLog(@"loggedOff");
	[[[[UIAlertView alloc] initWithTitle: @"Connection Status"
								 message: @"Connection Lost"
								delegate: nil
					   cancelButtonTitle: nil
					   otherButtonTitles: @"OK", nil] 
	  autorelease] 
	 show];
	
	[self resetConnection];
	[BlankMenuLayer activate];
	[self onEndLogin];
	[self showMenuScreen];
	
	// this is here to force the background task to be invalid if it isn't already.
	UIApplication *app = [UIApplication sharedApplication];
	if (logoffTask != UIBackgroundTaskInvalid) {
		[app endBackgroundTask:logoffTask]; 
		logoffTask = UIBackgroundTaskInvalid;
	}
}


- (void) onLogOnFailure: (NSString *) msg
{
	[[[[UIAlertView alloc] initWithTitle: @"Failed to Log On"
								 message: msg
								delegate: nil
					   cancelButtonTitle: nil
					   otherButtonTitles: @"OK", nil] 
	  autorelease] 
	 show];
	
	[self onEndLogin];
	[self showLoginMenu];
}


- (IBAction) handleInfoButton: (id) sender
{
	CCScene * scene = [[CCDirector sharedDirector] runningScene];
	
	if ([scene isKindOfClass: [WorldScene class]])
	{
		[((WorldScene*) scene).entityInfoLayer hide];
	}
	
	[UIView beginAnimations: nil 
					context: nil];
	[UIView setAnimationDuration: 1.0];
	[UIView setAnimationTransition: UIViewAnimationTransitionFlipFromRight
						   forView: viewController.view
							 cache: YES];
	[viewController.view addSubview: self.infoView];
	
	[UIView commitAnimations];
}

- (void) layoutInfoButton: (CGRect) bounds
{
	CGSize frameSize = self.infoButton.frame.size;
	
	self.infoButton.frame = CGRectMake( 
		bounds.origin.x + bounds.size.width - frameSize.width - 15,
		bounds.origin.y + bounds.size.height - frameSize.height - 15,
		frameSize.width, frameSize.height );	
}


@end
