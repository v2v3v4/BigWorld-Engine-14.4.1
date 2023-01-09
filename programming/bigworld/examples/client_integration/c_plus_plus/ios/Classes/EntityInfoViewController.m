//
//  EntityInfoViewController.m
//  FantasyDemo
//
//  Created by Daniel Giddings on 4/07/11.
//  Copyright 2011 MF. All rights reserved.
//

#import "EntityInfoViewController.h"


@implementation EntityInfoViewController

@synthesize icon;
@synthesize info;
@synthesize name;

@synthesize infoView;

- (id)initWithIconNameAndInfo:(NSString*)iconS name:(NSString*)nameS info:(NSString*)infoS
{
    self = [super init];
    if (self) {

		self.icon = iconS;
		self.name = nameS;
		self.info = infoS;
		
		self.contentSizeForViewInPopover = CGSizeMake(300,44);
    }

    return self;
}
    
// The designated initializer.  Override if you create the controller programmatically and want to perform customization that is not appropriate for viewDidLoad.
/*
- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil {
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        // Custom initialization.
    }
    return self;
}
*/


// Implement loadView to create a view hierarchy programmatically, without using a nib.
- (void)loadView {
	UIView* view = [[[UIView alloc] initWithFrame:CGRectMake(0,0,300,44)] autorelease];
	view.backgroundColor = [UIColor colorWithRed:0 green:0 blue:0 alpha:0];
	UIImageView* iconView = [[[UIImageView alloc] initWithFrame:CGRectMake(0,0,44,44)] autorelease];
	[iconView setImage:[UIImage imageWithContentsOfFile:[[NSBundle mainBundle] pathForResource:[self.icon stringByDeletingPathExtension] ofType:@"png"]]];
	[view addSubview:iconView];

	UILabel* nameView = [[[UILabel alloc] initWithFrame:CGRectMake(50,2,256,24)] autorelease];
	nameView.text = self.name;
	nameView.textColor = [UIColor whiteColor];
	nameView.font = [UIFont boldSystemFontOfSize:16];

	nameView.backgroundColor = [UIColor colorWithRed:0 green:0 blue:0 alpha:0];
	
	[view addSubview:nameView];
	
	self.infoView = [[[UILabel alloc] initWithFrame:CGRectMake(50,23,256,20)] autorelease];
	self.infoView.text = self.info;
	self.infoView.textColor = [UIColor whiteColor];
	self.infoView.backgroundColor = [UIColor colorWithRed:0 green:0 blue:0 alpha:0];
	self.infoView.font = [UIFont systemFontOfSize:14];
	
	[view addSubview:self.infoView];
	
	self.view = view;
}

- (void)updateInfo:(NSString*)infoS
{
	self.info = infoS;
	
	self.infoView.text = self.info;
}

/*
// Implement viewDidLoad to do additional setup after loading the view, typically from a nib.
- (void)viewDidLoad {
    [super viewDidLoad];
}
*/

/*
// Override to allow orientations other than the default portrait orientation.
- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
    // Return YES for supported orientations.
    return (interfaceOrientation == UIInterfaceOrientationPortrait);
}
*/

- (void)didReceiveMemoryWarning {
    // Releases the view if it doesn't have a superview.
    [super didReceiveMemoryWarning];
    
    // Release any cached data, images, etc. that aren't in use.
}

- (void)viewDidUnload {
    [super viewDidUnload];
    // Release any retained subviews of the main view.
    // e.g. self.myOutlet = nil;
	
	self.infoView = nil;
}


- (void)dealloc {
	self.icon = nil;
	self.name = nil;
	self.info = nil;

    [super dealloc];
}


@end
