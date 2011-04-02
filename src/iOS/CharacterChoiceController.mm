//
//  CharacterChoiceController.mm
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

#import "CharacterChoiceController.h"
#include "game.h"
#include "ios_helpers.h"

@implementation CharacterChoiceController
@synthesize avatarButton;
@synthesize character2Button;
@synthesize character3Button;
@synthesize character4Button;
@synthesize character5Button;
@synthesize character6Button;
@synthesize character7Button;
@synthesize character8Button;
@synthesize cancelButton;
@synthesize characterDict;
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
    const SaveGame *saveGame = game->currentGame();
    NSArray *allButtons = [NSArray arrayWithObjects:avatarButton, character2Button,
                                                   character3Button, character4Button,
                                                   character5Button, character6Button,
                                                   character7Button, character8Button, nil];
    characterDict
        = const_cast<NSDictionary *>(reinterpret_cast<const NSDictionary *>(U4IOS::createDictionaryForButtons(reinterpret_cast<CFArrayRef>(allButtons),
                                                            CFSTR("12345678"), cancelButton)));
    int i = 0;
    for (UIButton *button in allButtons) {
        if (i >= saveGame->members) {
            button.hidden = YES;
        } else {
            const SaveGamePlayerRecord &partyMember = saveGame->players[i];
            [button setTitle:[NSString stringWithUTF8String:partyMember.name] forState:UIControlStateNormal];
            button.hidden = NO;
        }
        ++i;
    }
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
    self.avatarButton = nil;
    self.character2Button = nil;
    self.character3Button = nil;
    self.character4Button = nil;
    self.character5Button = nil;
    self.character6Button = nil;
    self.character7Button = nil;
    self.character8Button = nil;
    self.cancelButton = nil;
    self.characterDict = nil;
}


- (void)dealloc {
    [avatarButton release];
    [character2Button release];
    [character3Button release];
    [character4Button release];
    [character5Button release];
    [character6Button release];
    [character7Button release];
    [character8Button release];
    [cancelButton release];
    [characterDict release];
    [super dealloc];
}

-(IBAction) buttonPressed:(id)sender {
    NSString *characterLetter = static_cast<NSString *>([characterDict objectForKey:sender]);
    assert(characterLetter != nil);
    EventHandler::getInstance()->getController()->notifyKeyPressed(char([characterLetter characterAtIndex:0]));
}

@end
