//
//  InfoView.m
//  FantasyDemo
//
//  Created by Daniel Giddings on 30/06/11.
//  Copyright 2011 MF. All rights reserved.
//

#import "InfoView.h"
#import "ApplicationDelegate.h"

#import <QuartzCore/QuartzCore.h>

@implementation InfoView

@synthesize webview;
@synthesize navigationBar;
@synthesize background;

// When landscape
#define VOFFL 10    // vertical offset
#define HOFFL 20    // horizontal offset

// when portrait
#define VOFFP 10
#define HOFFP 8

#define VNAV  44


- (id)initWithFrame:(CGRect)frame {
    
    self = [super initWithFrame:frame];
    if (self) {
		self.backgroundColor = [UIColor blackColor];

        // Initialization code.
		self.navigationBar = [[[UINavigationBar alloc] initWithFrame:CGRectMake(frame.origin.x,frame.origin.y,frame.size.width,VNAV)] autorelease];
		UIBarButtonItem* done = [[[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemDone target:self action:@selector(handleDoneButton:)] autorelease];
		UINavigationItem* item = [[[UINavigationItem alloc] initWithTitle:@"Info"] autorelease]; 
		item.rightBarButtonItem = done;
		self.navigationBar.barStyle = UIBarStyleBlack;
		[self.navigationBar pushNavigationItem: item animated:NO];

		[self addSubview: self.navigationBar];

		self.background = [[[UIImageView alloc] initWithFrame:CGRectMake(frame.origin.x, frame.origin.y + VNAV, frame.size.width, frame.size.height-VNAV)] autorelease];
		[self.background setImage:[UIImage imageWithContentsOfFile: [[NSBundle mainBundle] pathForResource:@"background" ofType:@"png"]]];		
		self.background.userInteractionEnabled = YES;
		[self addSubview: self.background];
		
		
		self.webview = [[[UIWebView alloc] initWithFrame:CGRectMake(HOFFP, VOFFP, frame.size.width-(HOFFP*2), frame.size.height-(VOFFP+VOFFP+VNAV))] autorelease];
		[self.webview.layer setCornerRadius: 5];
		[self.webview setClipsToBounds:YES];
		self.webview.layer.backgroundColor = [UIColor whiteColor].CGColor;
		
		self.webview.delegate = self;
		
		[self.background addSubview:self.webview];
		
		[self.webview loadRequest:[NSURLRequest requestWithURL:[[NSBundle mainBundle] URLForResource:@"index" withExtension:@"html"]]];		
	}
    return self;
}


-(void)layoutSubviews
{
	CGRect f = self.frame;
    int hoff, voff;
	
    if (f.size.width > f.size.height) // landscape
    {
        hoff = HOFFL;
        voff = VOFFL;
    }
    else // portrait
    {
        hoff = HOFFP;
        voff = VOFFP;
    }
    self.navigationBar.frame = CGRectMake(f.origin.x, f.origin.y, f.size.width, VNAV);
    self.background.frame = CGRectMake(f.origin.x, f.origin.y+VNAV, f.size.width, f.size.height-VNAV);
    self.webview.frame = CGRectMake(hoff, voff, f.size.width-(hoff*2), f.size.height-(voff+voff+VNAV));
	[self.webview reload];
}

/*
// Only override drawRect: if you perform custom drawing.
// An empty implementation adversely affects performance during animation.
- (void)drawRect:(CGRect)rect {
    // Drawing code.
}
*/

- (void)dealloc {
	self.navigationBar = nil;
	self.webview = nil;
	self.background = nil;
	
    [super dealloc];
}


-(IBAction)handleDoneButton:(id)sender
{
	[UIView beginAnimations:nil context:nil];
	[UIView setAnimationDuration:1.0];
	[UIView setAnimationTransition:UIViewAnimationTransitionFlipFromLeft forView:[self superview] cache:YES];
	[self removeFromSuperview];
	[UIView commitAnimations];
}

- (BOOL)webView:(UIWebView *)webView shouldStartLoadWithRequest:(NSURLRequest *)request navigationType:(UIWebViewNavigationType)navigationType
{	
	// open http, https and mailto using UIApplication openURL, don't load anything else
    NSURL *requestURL = [[[request URL] retain] autorelease];

    if( ([[requestURL scheme] isEqualToString: @"http"] || [[requestURL scheme] isEqualToString: @"https"]) &&
	    (navigationType == UIWebViewNavigationTypeLinkClicked))
	{
        return ![[UIApplication sharedApplication] openURL:requestURL];
    }
	
	if( [[[request URL] scheme] isEqual:@"mailto"] )
	{
        return ![[UIApplication sharedApplication] openURL:[request URL]];
    }

    return YES;
}

@end
