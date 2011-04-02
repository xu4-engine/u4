//
//  ConversationChoiceController.mm
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

#import "ConversationChoiceController.h"
#import "U4GameController.h"
#include "event.h"
#include <algorithm>

@implementation ConversationChoiceController
@synthesize choice8Button;
@synthesize choice7Button;
@synthesize choice6Button;
@synthesize choice5Button;
@synthesize choice4Button;
@synthesize choice1Button;
@synthesize choice3Button;
@synthesize choice2Button;
@synthesize choice9Button;
@synthesize choice10Button;
@synthesize choice11Button;
@synthesize choice12Button;
@synthesize choice13Button;
@synthesize choice14Button;
@synthesize choice15Button;
@synthesize choice16Button;
@synthesize noThanksButton;
@synthesize gameController;
@synthesize choices;

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
    [self updateChoiceButtons];
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
    [choiceButtonToStringDict release];
    self.choice1Button = nil;
    self.choice2Button = nil;
    self.choice3Button = nil;
    self.choice4Button = nil;
    self.choice5Button = nil;
    self.choice6Button = nil;
    self.choice7Button = nil;
    self.choice8Button = nil;
    self.choice9Button = nil;
    self.choice10Button = nil;
    self.choice11Button = nil;
    self.choice12Button = nil;
    self.choice13Button = nil;
    self.choice14Button = nil;
    self.choice15Button = nil;
    self.choice16Button = nil;
    self.noThanksButton = nil;
    self.gameController.currentPressedButton = nil;
}

- (void)dealloc {
    [choices release];
    [gameController release];
    // The other items were freed when the view unloaded.
    [super dealloc];
}

- (IBAction)choiceButtonPressed:(id)sender {
    UIButton *button = static_cast<UIButton *>(sender);
    self.gameController.currentPressedButton = button;
    NSString *choice = [choiceButtonToStringDict objectForKey:button.currentTitle];
    if (choice != nil) {
        unichar theLetter = [choice characterAtIndex:0];
        EventHandler::getInstance()->getController()->notifyKeyPressed(theLetter);
    }
}

- (IBAction)noThanksButtonPressed:(id)sender {
    EventHandler::getInstance()->getController()->notifyKeyPressed('\r');
}

-(void)updateChoiceButtons {
    if (choices == nil)
        return;
    [choiceButtonToStringDict release];
    choiceButtonToStringDict = [[NSMutableDictionary alloc] initWithCapacity:17];
    NSArray *buttonArray = [NSArray arrayWithObjects:self.choice1Button, self.choice2Button,
                            self.choice3Button, self.choice4Button, self.choice5Button,
                            self.choice6Button, self.choice7Button, self.choice8Button,
                            self.choice9Button, self.choice10Button, self.choice11Button,
                            self.choice12Button, self.choice13Button, self.choice14Button,
                            self.choice15Button, self.choice16Button, nil];

    for (UIButton *button in buttonArray)
        button.hidden = YES;
    
    // Reset the No Thanks button (just in case)
    self.noThanksButton.hidden = NO;
    [self.noThanksButton setTitle:@"No Thanks" forState:UIControlStateNormal];

    // Walk through the list of choices and put one on each of the buttons
    if (choices == nil || [choices hasPrefix:@" "]) {
        // Special case, just make the middle button a continue button
        [self.noThanksButton setTitle:@"Continue" forState:UIControlStateNormal];
    } else if ([choices hasPrefix:@"yn"]) {
        [self joinButton:self.choice1Button withString:@"y" buttonText:@"Yes"];
        [self joinButton:self.choice2Button withString:@"n" buttonText:@"No"];
        self.noThanksButton.hidden = YES;
    } else if ([choices hasPrefix:@"bs"]) {
        [self joinButton:self.choice1Button withString:@"b" buttonText:@"Buy"];
        [self joinButton:self.choice2Button withString:@"s" buttonText:@"Sell"];
    } else {
        NSRange totalChoices = [choices rangeOfString:@" " options:NSBackwardsSearch];
        NSUInteger maxWalk = std::min((totalChoices.location == NSNotFound) ? NSUInteger(0)
                                                                            : totalChoices.location,
                                      [buttonArray count]);
        for (NSUInteger i = 0; i < maxWalk; ++i) {
            unichar charArray[1];
            charArray[0] = [choices characterAtIndex:i];
            NSString *onlyOneCharacter = [NSString stringWithCharacters:charArray length:1];
            [self joinButton:[buttonArray objectAtIndex:i] withString:onlyOneCharacter
                  buttonText:[NSString stringWithFormat:@"Choice %@", onlyOneCharacter]];
        }
    }
}

-(void)joinButton:(UIButton *)button withString:(NSString *)string buttonText:(NSString *)buttonText {
    [choiceButtonToStringDict setObject:string forKey:buttonText];
    [button setTitle:buttonText forState:UIControlStateNormal];
    button.hidden = NO;
}

-(void)setChoices:(NSString *)newChoices {
    [newChoices retain];
    NSString *oldChoices = choices;
    choices = newChoices;
    [oldChoices release];
    if (choice1Button != nil) {
        [self updateChoiceButtons];
    }
}

@end
