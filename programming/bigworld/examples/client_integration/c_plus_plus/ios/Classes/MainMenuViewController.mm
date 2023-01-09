//
//  MainMenuController.m
//  FantasyDemo
//
//  Created by Evan Zachariadis on 18/04/12.
//  Copyright BigWorld Pty, Ltd. All rights reserved.
//

#import "ApplicationDelegate.h"
#import "cocos2d.h"
#import "LocalServerDiscovery.h"

#import "MainMenuViewController.h"

#define SERVER_LIST_ROWS_IPAD 10
#define SERVER_LIST_ROWS_IPHONE 3

// this is so we can hide the toolbar object and not block out the art.
@interface UITransparentToolbar : UIToolbar
@end

@implementation UITransparentToolbar

// Override draw rect to avoid
// background coloring
- (void)drawRect:(CGRect)rect {
    // do nothing in here
}

// Set properties to make background
// translucent.
- (void) applyTranslucentBackground
{
    self.backgroundColor = [UIColor clearColor];
    self.opaque = NO;
    self.translucent = YES;
}

// Override init.
- (id) init
{
    self = [super init];
    [self applyTranslucentBackground];
    return self;
}

// Override initWithFrame.
- (id) initWithFrame:(CGRect) frame
{
    self = [super initWithFrame:frame];
    [self applyTranslucentBackground];
    return self;
}

@end

@implementation MainMenuViewController

@synthesize mainLoginView;
@synthesize mainLogonButton;
@synthesize mainLogonInfoButton;
@synthesize mainLogonNameTextField;
@synthesize mainLogonIndicator;
@synthesize usernameText;

@synthesize backToolbar;

- (void)viewDidLoad
{
    [super viewDidLoad];
    
	// NOTE: we have to do this, as something strange happens to the view size.
	// the elements stay the same (size/position), but the view doesn't.
	// it's very strange. this programmatically resizes the view
	[self.view setFrame:[ApplicationDelegate sharedDelegate].window.bounds];
	self.view.bounds = [ApplicationDelegate sharedDelegate].window.bounds;
	
	[self initView];
}

// internal functions
- (void) attemptLogon
{
	ApplicationDelegate * delegate = [ApplicationDelegate sharedDelegate];
	NSUserDefaults * defaults = [NSUserDefaults standardUserDefaults];
	
	NSString * serverName = delegate.serverName;
	NSString * userName = mainLogonNameTextField.text;
	NSString * password = @"pass";
	
	if ([userName length] == 0)
	{
		userName = [defaults objectForKey: @"default_username"];
	}
	
	[defaults setObject: userName 
				 forKey: @"username"];
	[defaults synchronize];
	
	[mainLogonIndicator startAnimating];
	mainLogonButton.enabled = NO;
	mainLogonNameTextField.enabled = NO;
	
	BOOL startedLogin = delegate.connection->logOnTo( [serverName UTF8String], 
													 [userName UTF8String], 
													 [password UTF8String] );
	
	if (!startedLogin)
	{
		[self onEndLogin];
	}
	
}

// Actions implimentations
- (IBAction) onLogonPress
{
	NSLog(@"MainMenuDelegate::onLogonPress");
	[self attemptLogon];
}

- (IBAction) onInfoPress
{
	NSLog(@"MainMenuDelegate::onInfoPress");
	[[ApplicationDelegate sharedDelegate] handleInfoButton:nil];
}

- (IBAction) onServerBackPress
{
	NSLog(@"MainMenuDelegate::onServerBackPress");
	
	[self hideMainMenu];
	[[ApplicationDelegate sharedDelegate] showServerListMenu];
}

// our functionality calls
- (void) showMainMenu
{
	[backToolbar setHidden:YES];
	
	NSUserDefaults * defaults = [NSUserDefaults standardUserDefaults];
	
	NSString* userName = [defaults objectForKey: @"username"];
	[mainLogonNameTextField setText:userName];
	
	// show the back toolbar if we can view the server list
	if ([defaults boolForKey:@"show_local_servers"])
	{
		[backToolbar setHidden:NO];
	}
	
	[self.view setHidden:NO];
}

- (void) hideMainMenu
{
	[self.view setHidden:YES];
}

- (void) initView
{
	
	NSUserDefaults * defaults = [NSUserDefaults standardUserDefaults];
	
	NSString* userName = [defaults objectForKey: @"username"];
	
	// no default username? then make one.
	if ([userName length] == 0)
	{
		userName = [NSString stringWithFormat:@"User_%06d", (arc4random() % 1000000)];
		[defaults setObject: userName 
					 forKey: @"username"];
		[defaults setObject: userName 
					 forKey: @"default_username"];
		[defaults synchronize];
	}
	
	// setup the username textfield
	[mainLogonNameTextField setText:userName];
	mainLogonNameTextField.keyboardType = UIKeyboardTypeURL;
	mainLogonNameTextField.borderStyle = UITextBorderStyleRoundedRect;
	
	// now the login button
	UIImage *blueImage = [UIImage imageNamed:@"fd_ios_bluebutton.png"];
    UIImage *blueButtonImage = [blueImage stretchableImageWithLeftCapWidth:12
															  topCapHeight:0];
    [mainLogonButton setBackgroundImage:blueButtonImage
							   forState:UIControlStateSelected];
    [mainLogonButton setBackgroundImage:blueButtonImage
							   forState:UIControlStateHighlighted];
    [mainLogonButton setBackgroundImage:blueButtonImage
							   forState:UIControlStateNormal];
}

- (void) onEndLogin
{
	[mainLogonIndicator stopAnimating];
	mainLogonButton.enabled = YES;
	mainLogonNameTextField.enabled = YES;
}

// text delegate functions
-(BOOL)textFieldShouldReturn:(UITextField *)textField
{
    [textField resignFirstResponder];
    return YES;
}

@end
