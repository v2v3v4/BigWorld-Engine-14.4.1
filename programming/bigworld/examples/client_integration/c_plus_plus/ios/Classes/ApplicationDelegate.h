#import <UIKit/UIKit.h>

@class RootViewController;

@class ConnectionUpdater;

#include "connection_model/bw_server_connection.hpp"
#include "EntityFactory.hpp"

#import "EntityController.h"
#import "MainMenuViewController.h"
#import "ServerListViewController.h"

#import "InfoView.h"

class EntityView;

@interface ApplicationDelegate : NSObject <UIApplicationDelegate>
{
	UIWindow *					window;
	RootViewController *		viewController;
	MainMenuViewController *	menuViewController;
	ServerListViewController *	serverListViewController;
	NSString *					serverName;
	NSString *					defaultServerName;
	UIBackgroundTaskIdentifier	logoffTask;
	BOOL						isIpad;
}

@property (nonatomic, retain) UIWindow * window;
@property (nonatomic, retain) NSString * serverName;
@property (nonatomic, retain) NSString * defaultServerName;
@property (nonatomic, readonly) BOOL shouldShowResetServer;

@property (nonatomic, retain) InfoView * infoView;
@property (nonatomic, retain) UIButton * infoButton;

@property (nonatomic, readonly) BW::BWServerConnection * connection;
@property (nonatomic, readonly) BW::FilterEnvironment * filterEnvironment;

+ (ApplicationDelegate *) sharedDelegate;

- (void) resetConnection;
- (void) resetDefaultServer;

- (IBAction)handleInfoButton:(id)sender;
- (void) layoutInfoButton:(CGRect)bounds;
- (void) entityViewWasClicked: (EntityView *) pEntityView;
- (void) setPlayerController: (id< EntityController >) aController;
- (void) addEntityView: (EntityView *) pView;
- (void) removeEntityView: (EntityView *) pView;
- (NSArray *) entityViewsFromLocation: (CGPoint) location;

// Menu Management
- (void) showMenuScreen;
- (void) hideMenuScreen;
- (void) showLoginMenu;
- (void) hideLoginMenu;
- (void) showServerListMenu;
- (void) hideServerListMenu;
- (void) onEndLogin;

@end
