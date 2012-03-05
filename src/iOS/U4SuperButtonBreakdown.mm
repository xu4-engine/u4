//
//  U4SuperButtonBreakdown.m
//  xu4
//
//  Created by Trenton Schulz on 2/23/12.
//  Copyright (c) 2012 Norwegian Computing Center. All rights reserved.
//

#import "U4SuperButtonBreakdown.h"
#import "U4GameController.h"
#import "U4Button.h"
#include "context.h"
#include "player.h"

@implementation U4SuperButtonBreakdown
@synthesize exitButton;
@synthesize raiseBalloon;
@synthesize gameController;

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil gameController:(U4GameController *)controller;
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        self.gameController = controller;
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
    if (c->party->isFlying()) {
        self.exitButton.hidden = YES;
        self.raiseBalloon.hidden = YES;
    } else {
        self.exitButton.hidden = NO;
        self.raiseBalloon.hidden = NO;
    }
    self.contentSizeForViewInPopover = self.view.frame.size;
}

- (void)viewDidUnload
{
    [self setExitButton:nil];
    [self setRaiseBalloon:nil];
    [self setGameController:nil];
    [super viewDidUnload];
    // Release any retained subviews of the main view.
    // e.g. self.myOutlet = nil;
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
    // Return YES for supported orientations
	return YES;
}

- (void)dealloc {
    [exitButton release];
    [raiseBalloon release];
    [gameController release];
    [super dealloc];
}

- (IBAction)buttonPressed:(id)balloonButton {
    if (balloonButton == exitButton) {
        [gameController exitPressed:exitButton];
    } else { 
        assert(balloonButton == raiseBalloon);
        [gameController klimbPressed:raiseBalloon];
    }
}
@end
