//
//  InfoView.h
//  FantasyDemo
//
//  Created by Daniel Giddings on 30/06/11.
//  Copyright 2011 MF. All rights reserved.
//

#import <UIKit/UIKit.h>


@interface InfoView : UIView<UIWebViewDelegate> {

}

@property (nonatomic, retain) UINavigationBar* navigationBar;
@property (nonatomic, retain) UIWebView* webview;
@property (nonatomic, retain) UIImageView* background;

-(IBAction)handleDoneButton:(id)sender;

@end
