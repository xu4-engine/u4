//
//  U4SuperButtonBreakdown.h
//  xu4
//
//  Created by Trenton Schulz on 2/23/12.
//  Copyright (c) 2012 Norwegian Computing Center. All rights reserved.
//

#import <UIKit/UIKit.h>

@class U4Button;
@class U4GameController;

@interface U4SuperButtonBreakdown : UIViewController
{
    U4Button *exitButton;
    U4Button *raiseBalloon;
    U4GameController *gameController;
}
- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil gameController:(U4GameController *)controller;

@property (retain, nonatomic) IBOutlet U4GameController *gameController;
@property (retain, nonatomic) IBOutlet U4Button *exitButton;
@property (retain, nonatomic) IBOutlet U4Button *raiseBalloon;
- (IBAction)buttonPressed:(id)sender;

@end
