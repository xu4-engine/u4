//
//  U4ChooseDirectionPopup.mm
//  xu4
//
// Copyright 2011 Trenton Schulz. All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
// 
//    1. Redistributions of source code must retain the above copyright notice, this list of
//       conditions and the following disclaimer.
// 
//    2. Redistributions in binary form must reproduce the above copyright notice, this list
//       of conditions and the following disclaimer in the documentation and/or other materials
//       provided with the distribution.
// 
// THIS SOFTWARE IS PROVIDED BY TRENTON SCHULZ ``AS IS'' AND ANY EXPRESS OR IMPLIED
// WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
// FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
// ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// 
// The views and conclusions contained in the software and documentation are those of the
// authors and should not be interpreted as representing official policies, either expressed
// or implied, of Trenton Schulz.
//

#include "game.h"
#import "U4ChooseDirectionPopup.h"
#import "U4GameController.h"

@implementation U4ChooseDirectionPopup
@synthesize upButton;
@synthesize rightButton;
@synthesize downButton;
@synthesize leftButton;
@synthesize gameController;


- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil gameController:(U4GameController *)controller {
    climbMode = NO;
    if ((self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil])) {
        self.gameController = controller;
    }
    return self;
}

- (BOOL)climbMode {
    return climbMode;
}

- (void)setClimbMode:(BOOL)newMode {
    climbMode = newMode;
    [self transformMode];
}


- (void)transformMode {
    if (leftButton == nil)
        return;

    if (climbMode) {
        self.leftButton.hidden = YES;
        self.rightButton.hidden = YES;
    } else {
        self.leftButton.hidden = NO;
        self.rightButton.hidden = NO;
    }
}

// Implement viewDidLoad to do additional setup after loading the view, typically from a nib.
- (void)viewDidLoad {
    [super viewDidLoad];
    self.contentSizeForViewInPopover = self.view.frame.size;
    [self transformMode];
}


- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
    // Overriden to allow any orientation.
    return YES;
}


- (void)didReceiveMemoryWarning {
    // Releases the view if it doesn't have a superview.
    [super didReceiveMemoryWarning];
    
    // Release any cached data, images, etc that aren't in use.
}


- (void)viewDidUnload {
    [super viewDidUnload];
    self.upButton = nil;
    self.leftButton = nil;
    self.downButton = nil;
    self.rightButton = nil;
    // Release any retained subviews of the main view.
    // e.g. self.myOutlet = nil;
}


- (void)dealloc {
    [rightButton release];
    [upButton release];
    [downButton release];
    [leftButton release];
    [gameController release];
    [super dealloc];
}


- (IBAction)directionButtonPressed:(id)directionButton {
    if (directionButton == upButton) {
        if (climbMode)
            [gameController klimbPressed:directionButton];
        else
            [gameController goUpPressed:directionButton];
    } else if (directionButton == downButton) {
        if (climbMode)
            [gameController descendPressed:directionButton];
        else        
            [gameController goDownPressed:directionButton];
    } else if (directionButton == leftButton) {
        [gameController goLeftPressed:directionButton];
    } else if (directionButton == rightButton) {
        [gameController goRightPressed:directionButton];
    }
}
@end
