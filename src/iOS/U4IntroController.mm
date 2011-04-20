//
//  U4IntroController.mm
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

#import "U4IntroController.h"
#import "U4AppDelegate.h"
#import "U4RootController.h"
#include "intro.h"
#include "settings.h"


@implementation U4IntroController

@synthesize startButton;
@synthesize loadButton;
@synthesize continueButton;
@synthesize choiceAButton;
@synthesize choiceBButton;

/*
 // The designated initializer.  Override if you create the controller programmatically and want to perform customization that is not appropriate for viewDidLoad.
- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil {
    if ((self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil])) {
        // Custom initialization
    }
    return self;
}
*/

// Implement viewDidLoad to do additional setup after loading the view, typically from a nib.
- (void)viewDidLoad {
    [super viewDidLoad];
    continueButton.hidden = YES;
    choiceAButton.hidden = YES;
    choiceBButton.hidden = YES;
    loadButton.hidden = !([[NSFileManager defaultManager] 
                           fileExistsAtPath:[NSString stringWithUTF8String:
                           (settings.getUserPath() + PARTY_SAV_BASE_FILENAME).c_str()]]);
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
    // Release any retained subviews of the main view.
    // e.g. self.myOutlet = nil;
    self.startButton = nil;
    self.loadButton = nil;
    self.continueButton = nil;
    self.choiceAButton = nil;
    self.choiceBButton = nil;
}


- (void)dealloc {
    [super dealloc];
}

- (IBAction)startGame:(id)sender {
    intro->skipTitles();
//    oldBounds = self.view.frame;
    startGame = [[[U4StartDialogController alloc] initWithNibName:@"U4StartDialog" bundle:nil] autorelease];
    startGame.delegate = self;
    U4AppDelegate *ourDelegate = [UIApplication sharedApplication].delegate;
    [ourDelegate.viewController presentModalViewController:startGame animated:YES];
}

- (IBAction)loadGame:(id)sender {
    intro->skipTitles();
    // reset the timer to the pre-titles granularity, this ensures the timer is running correctly.
    eventHandler->getTimer()->reset(eventTimerGranularity);
    intro->journeyOnward();
    [self launchGameController];
}

- (IBAction)continuePressed:(id)sender {
    EventHandler::getInstance()->getController()->notifyKeyPressed('\n');
}

- (void)startDialogControllerDidFinish:(U4StartDialogController *)controller {
    U4AppDelegate *ourDelegate = [UIApplication sharedApplication].delegate;
    [ourDelegate.viewController dismissModalViewControllerAnimated:YES];
    if (controller.success) {
        [self switchToContinueButtons];
        string namebuffer = [controller.avatarName UTF8String];
        NSInteger sexTypeThing = controller.avatarGender;
        SexType sex = sexTypeThing == 0 ? SEX_MALE : SEX_FEMALE;
        intro->finishInitiateGame(namebuffer, sex);
        [self launchGameController];
    }
}

- (void)launchGameController {
    U4AppDelegate *appDelegate = [UIApplication sharedApplication].delegate;
    U4RootController *rootController = appDelegate.viewController;
    [rootController performSelector:@selector(startGameController)
                         withObject:nil afterDelay:.125];    
}

- (IBAction)choiceAClicked:(id)sender {
    EventHandler::getInstance()->getController()->notifyKeyPressed('a');
}

- (IBAction)choiceBClicked:(id)sender {
    EventHandler::getInstance()->getController()->notifyKeyPressed('b');
}

- (void)switchToContinueButtons {
    static const  NSTimeInterval ALPHA_DURATION = 0.40;
    [UIView beginAnimations:nil context:NULL];
    [UIView setAnimationDuration:ALPHA_DURATION];
    self.startButton.alpha = 0.0;
    self.loadButton.alpha = 0.0;
    self.choiceAButton.alpha = 0.0;
    self.choiceBButton.alpha = 0.0;
    self.continueButton.hidden = NO;
    self.continueButton.alpha = 1.0;
    [UIView commitAnimations];
}

- (void)switchToChoiceButtons {
    static const NSTimeInterval ALPHA_DURATION = 0.40;
    [UIView beginAnimations:nil context:NULL];
    [UIView setAnimationDuration:ALPHA_DURATION];
    self.startButton.alpha = 0.0;
    self.loadButton.alpha = 0.0;
    self.choiceAButton.hidden = NO;
    self.choiceBButton.hidden = NO;
    self.choiceAButton.alpha = 1.0;
    self.choiceBButton.alpha = 1.0;
    self.continueButton.alpha = 0.0;
    [UIView commitAnimations];
}

@end
