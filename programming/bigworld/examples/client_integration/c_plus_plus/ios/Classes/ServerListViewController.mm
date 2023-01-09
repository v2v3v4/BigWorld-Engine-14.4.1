
#import "ApplicationDelegate.h"
#import "cocos2d.h"
#import "LocalServerDiscovery.h"

#import "ServerListViewController.h"

#define MIN_SERVER_LIST_ROWS_IPAD 10
#define MIN_SERVER_LIST_ROWS_IPHONE 3

@implementation ServerListViewController

@synthesize serverListScrollView;
@synthesize serverListTableView;
@synthesize selectAServerText;

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self)
	{
        // Custom initialization
    }
    return self;
}

- (void)didReceiveMemoryWarning
{
    // Releases the view if it doesn't have a superview.
    [super didReceiveMemoryWarning];
    
    // Release any cached data, images, etc that aren't in use.
}

#pragma mark - View lifecycle

- (void)viewDidLoad
{
    [super viewDidLoad];
	
	// NOTE: we have to do this, as something strange happens to the view size.
	// the elements stay the same (size/position), but the view doesn't. it's very strange.
	// this programmatically resizes the view
	[self.view setFrame:[ApplicationDelegate sharedDelegate].window.bounds];
	self.view.bounds = [ApplicationDelegate sharedDelegate].window.bounds;
	isIpad = (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad);

	[self initView];
}

- (void)viewDidUnload
{
    [super viewDidUnload];
    // Release any retained subviews of the main view.
    // e.g. self.myOutlet = nil;
}

- (void) fetchLocalServers
{
	ApplicationDelegate * appDelegate = [ApplicationDelegate sharedDelegate];
	BW::BWServerConnection * connection = appDelegate.connection;
	
	// NOTE: if we havn't init'd bigworld yet, our connection will be null, so put this current call on a callback.
	if (connection == NULL)
	{
		[self performSelector:@selector(fetchLocalServers)
				   withObject:nil
				   afterDelay:0.2];
		return;
	}
	// don't bother fetching servers if we are already in the process of doing so.
	else if (fetchingServers)
	{
		return;
	}
	
	fetchingServers = YES;
	
	localServerList = [[NSMutableDictionary alloc] init];
	
	BW::ServerFinder * pServerFinder = new DiscoverMenuServerFinder( self );
	connection->findServers( *pServerFinder );
}

- (void) onFoundServer: (NSString *) newServer 
			  withName: (NSString *) serverName 
			   andUser: (NSString *) username
{
	NSLog( @"onFoundServer: %@ withName: %@ andUser: %@", 
		  newServer, serverName, username );

	NSArray * components = [newServer componentsSeparatedByString: @":"];
	if (components.count < 2)
	{
		NSLog( @"Invalid server address: %@", newServer );
		return;
	}
	int port = [[components objectAtIndex: 1] intValue];
	if (port <= 0)
	{
		NSLog( @"Invalid port: %d", port );
		return;
	}

	NSString * label = [NSString stringWithFormat: @"%@:%d - %@",
						serverName, port, username];
	
	[localServerList setValue:newServer forKey:label];
}

- (void) onFindServersDone
{
	NSLog( @"onFindServersDone" );
	[serverListTableView reloadData];
	fetchingServers = NO;
}

- (void) initView
{
	localServerList = [[NSMutableDictionary alloc] init];
	
	// this puts the rounded edges on the scolling view and table.
	serverListTableView.layer.cornerRadius = 15;
	serverListScrollView.layer.cornerRadius = 15;
}

- (void) showServerList
{
	[serverListTableView reloadData];
	[self fetchLocalServers];
	[self.view setHidden:NO];
}

- (void) hideServerList
{
	[self.view setHidden:YES];
}

- (void) onEndLogin
{
	// Do anything that needs to be done when finishing logging on here
	// currently that is nothing.
}

// XIB actions
- (IBAction) onInfoPress
{
	NSLog(@"MainMenuDelegate::onInfoPress");
	[[ApplicationDelegate sharedDelegate] handleInfoButton:nil];
}

// uiTableView delegate functions
- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
	// we only have one section in the table.
	// All the following functions assume this, and will print error messages if
	// this is changed.
    return 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
	// if our section does NOT equal 0 then something is wrong. Skip out.
	if (section != 0)
	{
		NSLog(@"tableView:numberOfRowsInSection: - Trying to get rows for invalid section.");
		return 0;
	}
	
	int numberOfRows = 0;
	
	if (isIpad)
	{
		numberOfRows = std::max( MIN_SERVER_LIST_ROWS_IPAD,
								1 + (int)[localServerList count]);
		
	}
	else
	{
		numberOfRows = std::max( MIN_SERVER_LIST_ROWS_IPHONE,
								1 + (int)[localServerList count] );
	}
	
	// resize our table view to match the number of rows
	[tableView setFrame:CGRectMake(tableView.frame.origin.x,
								   tableView.frame.origin.y,
								   tableView.frame.size.width,
								   tableView.rowHeight * numberOfRows)];
	
	// let the scroll view know how long our list is so it scrolls correctly.
	[serverListScrollView setContentSize:CGSizeMake(tableView.frame.size.width,
													tableView.rowHeight * numberOfRows)];

	return numberOfRows;
}

- (UITableViewCell *) createDefaultServerCellForTable:(UITableView*)tableView
{
    UITableViewCell * cell = [tableView dequeueReusableCellWithIdentifier:@"MainServerCell"];
	if (cell == nil)
	{
		cell = [[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault
									   reuseIdentifier:@"MainServerCell"]
				autorelease];
	}
	
	cell.textLabel.text = [ApplicationDelegate sharedDelegate].defaultServerName;
	cell.accessoryType = UITableViewCellAccessoryNone;
	return cell;
}

- (UITableViewCell *) createLocalServerCell:(NSIndexPath *)index forTable:(UITableView*)tableView
{
    UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:@"ServerCell"];
    if (cell == nil)
	{
        cell = [[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault
									   reuseIdentifier:@"ServerCell"]
				autorelease];
    }
    
	// TODO: FIXME currently -1 as default server is at the top of the local server list
	cell.textLabel.text = [[localServerList allKeys] objectAtIndex:index.row-1];
	cell.accessoryType = UITableViewCellAccessoryNone;
	
	return cell;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
	// if our section does NOT equal 0 then something is wrong. Skip out.
	if (indexPath.section != 0)
	{
		NSLog(@"tableView:cellForRowAtIndexPath: - Trying to create cell for invalid section.");
		return nil;
	}
	
	UITableViewCell * cell = nil;
	
	// if this is meant to be a blank cell, return one.
	if (indexPath.row > [localServerList count])
	{
		cell = [tableView dequeueReusableCellWithIdentifier:@"BlankServerCell"];
		if (cell == nil)
		{
			cell = [[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault
										   reuseIdentifier:@"BlankServerCell"]
					autorelease];
		}
		
		cell.textLabel.text = @"";
		cell.accessoryType = UITableViewCellAccessoryNone;
		return cell;
	}
	else if (indexPath.row == 0)
	{
		// we assume that this only gets called once. this is our default server.
		cell = [self createDefaultServerCellForTable:tableView];
	}
	else
	{
		cell = [self createLocalServerCell:indexPath forTable:tableView];;
	}
	
    return cell;
}

// this is the actual callback when we hit an item.
- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
	[tableView deselectRowAtIndexPath:indexPath animated:YES];
	
	// if our section does NOT equal 0 then something is wrong. Skip out.
	if (indexPath.section != 0)
	{
		NSLog(@"tableView:didSelectRowAtIndexPath: - Trying to select invalid section.");
		return;
	}
	
	// HACK: this is to make blank rows do nothing.
	if ([localServerList count] < indexPath.row)
		return;
	
	if (indexPath.row == 0)
	{
		[ApplicationDelegate sharedDelegate].serverName = [ApplicationDelegate sharedDelegate].defaultServerName;
	}
	else
	{
		// HACK: again, the -1 is due to the default server being on the local list
		[ApplicationDelegate sharedDelegate].serverName = [localServerList objectForKey:
														   [[localServerList allKeys] objectAtIndex:indexPath.row-1]];
	}
	
	// hide ourselfs and show the login screen
	[self hideServerList];
	[[ApplicationDelegate sharedDelegate] showLoginMenu];
}

@end
