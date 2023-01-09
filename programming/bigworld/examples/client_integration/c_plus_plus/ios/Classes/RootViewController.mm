#import "cocos2d.h"

#import "RootViewController.h"

#import "ApplicationDelegate.h"

@implementation RootViewController

// Override to allow orientations other than the default portrait orientation.
- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
	//
	// We use the uiViewController to handle rotation
	// The view will rotate to all possible orientations
	//
	
	return YES;
}


// This rotates the view's elements correctly.
-(void)willRotateToInterfaceOrientation:(UIInterfaceOrientation)toInterfaceOrientation duration:(NSTimeInterval)duration
{
	//
	// Assuming that the main window has the size of the screen
	// BUG: This won't work if the EAGLView is not fullscreen
	///
	CGRect screenRect = [[UIScreen mainScreen] bounds];
	CGRect rect = screenRect;
	
	if(toInterfaceOrientation == UIInterfaceOrientationLandscapeLeft ||
	   toInterfaceOrientation == UIInterfaceOrientationLandscapeRight)
		rect.size = CGSizeMake( screenRect.size.height, screenRect.size.width );
	
	// rect in points before scaling based on content scale
	CGRect pointRect = rect;
	
	CCDirector *director = [CCDirector sharedDirector];
	EAGLView *glView = [director openGLView];
	float contentScaleFactor = [director contentScaleFactor];
	
	if( contentScaleFactor != 1 ) {
		rect.size.width *= contentScaleFactor;
		rect.size.height *= contentScaleFactor;
	}
	glView.frame = rect;
	
	ApplicationDelegate * ad = [ApplicationDelegate sharedDelegate];
	
	// use points rect when laying out UI elements
	ad.infoView.frame = pointRect;
	[ad.infoView layoutSubviews];
	[ad layoutInfoButton:pointRect];
}


- (void)didReceiveMemoryWarning
{
    // Releases the view if it doesn't have a superview.
    [super didReceiveMemoryWarning];
    
    // Release any cached data, images, etc that aren't in use.
}

- (void)viewDidUnload
{
    [super viewDidUnload];
    // Release any retained subviews of the main view.
    // e.g. self.myOutlet = nil;
}


- (void)dealloc
{
    [super dealloc];
}

@end

