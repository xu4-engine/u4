//
//  CastSpellController.mm
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

#import "CastSpellController.h"
#import "U4GameController.h"
#include "game.h"
#include "ios_helpers.h"

@implementation CastSpellController
@synthesize gameController;
@synthesize awakenButton;
@synthesize blinkButton;
@synthesize cureButton;
@synthesize dispelButton;
@synthesize energyFieldButton;
@synthesize fireballButton;
@synthesize gateTravelButton;
@synthesize healButton;
@synthesize iceballButton;
@synthesize jinxButton;
@synthesize killButton;
@synthesize lightButton;
@synthesize magicMissleButton;
@synthesize negateButton;
@synthesize openButton;
@synthesize protectionButton;
@synthesize quicknessButton;
@synthesize resurrectButton;
@synthesize sleepButton;
@synthesize tremorButton;
@synthesize undeadButton;
@synthesize viewButton;
@synthesize windChangeButton;
@synthesize xitButton;
@synthesize yupButton;
@synthesize zdownButton;
@synthesize cancelButton;
@synthesize noSpellLabel;
@synthesize spellDict;

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
    const SaveGame * const saveGame = game->currentGame();
    NSArray *spellButtons = [NSArray arrayWithObjects:awakenButton, blinkButton, cureButton, dispelButton,
                                                      energyFieldButton, fireballButton, gateTravelButton,
                                                      healButton, iceballButton, jinxButton, killButton,
                                                      lightButton, magicMissleButton, negateButton,
                                                      openButton, protectionButton, quicknessButton,
                                                      resurrectButton, sleepButton, tremorButton,
                                                      undeadButton, viewButton, windChangeButton,
                                                      xitButton, yupButton, zdownButton, nil];

    spellDict
        = const_cast<NSDictionary *>(reinterpret_cast<const NSDictionary *>(U4IOS::createDictionaryForButtons(reinterpret_cast<CFArrayRef>(spellButtons),
                                                                                                           CFSTR("abcdefghijklmnopqrstuvwxyz"),
                                                                                                           cancelButton)));
    self.noSpellLabel.hidden = YES;
    int hideCount = 0;
    int i = 0;
    for (UIButton *button in spellButtons) {
        short mixtureCount = saveGame->mixtures[i];
        [button setTitle:[NSString stringWithFormat:@"%@ (%d)", button.currentTitle, mixtureCount] forState:UIControlStateNormal];
        BOOL hidden = (mixtureCount == 0);
        button.hidden = hidden;
        if (hidden)
            ++hideCount;
        ++i;
    }
    if (hideCount == [spellButtons count])
        self.noSpellLabel.hidden = NO;
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
    self.gameController.currentPressedButton = nil;
    // Release any retained subviews of the main view.
    // e.g. self.myOutlet = nil;
    self.gameController = nil;
    self.awakenButton = nil;
    self.blinkButton = nil;
    self.cureButton = nil;
    self.dispelButton = nil;
    self.energyFieldButton = nil;
    self.fireballButton = nil;
    self.gateTravelButton = nil;
    self.healButton = nil;
    self.iceballButton = nil;
    self.jinxButton = nil;
    self.killButton = nil;
    self.lightButton = nil;
    self.magicMissleButton = nil;
    self.negateButton = nil;
    self.openButton = nil;
    self.protectionButton = nil;
    self.quicknessButton = nil;
    self.resurrectButton = nil;
    self.sleepButton = nil;
    self.tremorButton = nil;
    self.undeadButton = nil;
    self.viewButton = nil;
    self.windChangeButton = nil;
    self.xitButton = nil;
    self.yupButton = nil;
    self.zdownButton = nil;
    self.noSpellLabel = nil;
    self.cancelButton = nil;
    self.spellDict = nil;
}


- (void)dealloc {
    [awakenButton release];
    [blinkButton release];
    [cureButton release];
    [dispelButton release];
    [energyFieldButton release];
    [fireballButton release];
    [gateTravelButton release];
    [healButton release];
    [iceballButton release];
    [jinxButton release];
    [killButton release];
    [lightButton release];
    [magicMissleButton release];
    [negateButton release];
    [openButton release];
    [protectionButton release];
    [quicknessButton release];
    [resurrectButton release];
    [sleepButton release];
    [tremorButton release];
    [undeadButton release];
    [viewButton release];
    [windChangeButton release];
    [xitButton release];
    [yupButton release];
    [zdownButton release];
    [gameController release];
    [noSpellLabel release];
    [cancelButton release];
    [spellDict release];
    [super dealloc];
}


- (IBAction)buttonPressed:(id)sender {
    gameController.currentPressedButton = sender;
    NSString *spellLetter = static_cast<NSString *>([spellDict objectForKey:sender]);
    assert(spellLetter != nil);
    EventHandler::getInstance()->getController()->notifyKeyPressed(char([spellLetter characterAtIndex:0]));    
}

@end
