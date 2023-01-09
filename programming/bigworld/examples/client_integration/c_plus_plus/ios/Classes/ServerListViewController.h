
#import <UIKit/UIKit.h>

@interface ServerListViewController : UIViewController <UITableViewDelegate, UITableViewDataSource>
{
	UIScrollView * serverListScrollView;
	UITableView * serverListTableView;
	UILabel * selectAServerText;

	NSDictionary * localServerList;
	BOOL	fetchingServers;
	BOOL	isIpad;
}

@property (nonatomic, retain) IBOutlet UIScrollView * serverListScrollView;
@property (nonatomic, retain) IBOutlet UITableView * serverListTableView;
@property (nonatomic, retain) IBOutlet UILabel * selectAServerText;

- (void) initView;

- (void) onFoundServer: (NSString *) newServer 
			  withName: (NSString *) serverName 
			   andUser: (NSString *) username;

- (void) onFindServersDone;

- (void) showServerList;
- (void) hideServerList;

// function to let the menu know we have finised attempting to logon
- (void) onEndLogin;

@end
