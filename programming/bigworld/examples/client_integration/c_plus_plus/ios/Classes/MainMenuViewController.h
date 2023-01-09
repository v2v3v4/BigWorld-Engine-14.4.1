//
//  MainMenuController.h
//  FantasyDemo
//
//  Created by Evan Zachariadis on 18/04/12.
//  Copyright BigWorld Pty, Ltd. All rights reserved.
//
//	NOTE: 
//

#import <UIKit/UIKit.h>

@interface MainMenuViewController : UIViewController <UITextFieldDelegate>
{
	UIView * mainLoginView;
	UIButton * mainLogonButton;
	UIButton * mainLogonInfoButton;
	UITextField * mainLogonNameTextField;
	UIActivityIndicatorView * mainLogonIndicator;
	
	UILabel * usernameText;
	
	UIToolbar * backToolbar;
}

// these are for the main menu.
@property (nonatomic, retain) IBOutlet UIView * mainLoginView;
@property (nonatomic, retain) IBOutlet UIButton * mainLogonButton;
@property (nonatomic, retain) IBOutlet UIButton * mainLogonInfoButton;
@property (nonatomic, retain) IBOutlet UITextField * mainLogonNameTextField;
@property (nonatomic, retain) IBOutlet UIActivityIndicatorView * mainLogonIndicator;

@property (nonatomic, retain) IBOutlet UILabel * usernameText;

// these are for the server list view.
@property (nonatomic, retain) IBOutlet UIToolbar * backToolbar;

// action functions for the buttons etc.
- (IBAction) onLogonPress;
- (IBAction) onInfoPress;
- (IBAction) onServerBackPress;

// our functionality calls
- (void) initView;

- (void) showMainMenu;
- (void) hideMainMenu;

// function to let the menu know we have finised attempting to logon
- (void) onEndLogin;

@end
