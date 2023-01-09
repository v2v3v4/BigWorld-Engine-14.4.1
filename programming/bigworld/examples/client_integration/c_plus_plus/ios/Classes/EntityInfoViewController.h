//
//  EntityInfoViewController.h
//  FantasyDemo
//
//  Created by Daniel Giddings on 4/07/11.
//  Copyright 2011 MF. All rights reserved.
//

#import <UIKit/UIKit.h>


@interface EntityInfoViewController : UIViewController {

}

@property (nonatomic, retain) NSString* icon;
@property (nonatomic, retain) NSString* info;
@property (nonatomic, retain) NSString* name;

@property (nonatomic, retain) UILabel* infoView;

- (id)initWithIconNameAndInfo:(NSString*)iconS name:(NSString*)nameS info:(NSString*)infoS;

- (void)updateInfo:(NSString*)infoS;

@end
