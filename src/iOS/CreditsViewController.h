//
//  CreditsViewController.h
//  xu4
//
//  Created by Trenton Schulz on 7/10/11.
//  Copyright 2011 Norwegian Computing Center. All rights reserved.
//

#import <UIKit/UIKit.h>

@class CreditsViewController;
@protocol CreditsViewControllerDelegate
- (void)CreditsViewControllerDidFinish:(CreditsViewController *)controller;
@end



@interface CreditsViewController : UIViewController {
    
    UIWebView *webView;
    UINavigationBar *navBar;
    id<CreditsViewControllerDelegate> delegate;
}

@property (nonatomic, retain) IBOutlet UIWebView *webView;
@property (nonatomic, retain) IBOutlet UINavigationBar *navBar;
@property (nonatomic, assign) id<CreditsViewControllerDelegate> delegate;
@end
