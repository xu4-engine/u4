//
//  U4WeaponChoiceDialog.mm
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

#import "U4WeaponChoiceDialog.h"
#include "game.h"
#include "ios_helpers.h"

@implementation U4WeaponChoiceDialog
@synthesize handsButton, staffButton, daggerButton, slingButton, maceButton, axeButton,
            swordButton, bowButton, crossBowButton, oilButton, halberdButton, magicAxeButton,
            magicSwordButton, magicBowButton, magicWandButton, mysticSwordButton, cancelButton,
            weaponDict;

 // The designated initializer.  Override if you create the controller programmatically and want to perform customization that is not appropriate for viewDidLoad.
/*
- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil {
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        // Custom initialization.
    }
    return self;
}
*/

// Implement viewDidLoad to do additional setup after loading the view, typically from a nib.
- (void)viewDidLoad {
    [super viewDidLoad];
    const SaveGame * const savegame = game->currentGame();
    NSArray *allButtons = [NSArray arrayWithObjects:handsButton, staffButton, daggerButton, slingButton, maceButton,
                           axeButton, swordButton, bowButton, crossBowButton, oilButton, halberdButton, magicAxeButton,
                           magicSwordButton, magicBowButton, magicWandButton, mysticSwordButton, nil];
    weaponDict
        = const_cast<NSDictionary *>(reinterpret_cast<const NSDictionary *>(U4IOS::createDictionaryForButtons(reinterpret_cast<CFArrayRef>(allButtons),
                                                                                                           CFSTR("ABCDEFGHIJKLMNOP"), cancelButton)));
    
    int i = 0;
    for (UIButton *button in allButtons) {
        if (i != 0) { // // You always have your hands, don't bother with this count stuff.
            short weaponCount = savegame->weapons[i];
            [button setTitle:[NSString stringWithFormat:@"%@ (%d)", button.currentTitle, weaponCount]
                    forState:UIControlStateNormal];
            button.hidden = (weaponCount == 0);
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
    
    // Release any cached data, images, etc. that aren't in use.
}


- (void)viewDidUnload {
    [super viewDidUnload];
    self.handsButton = nil;
    self.staffButton = nil;
    self.daggerButton = nil;
    self.slingButton = nil;
    self.maceButton = nil;
    self.axeButton = nil;
    self.swordButton = nil;
    self.bowButton = nil;
    self.crossBowButton = nil;
    self.oilButton = nil;
    self.halberdButton = nil;
    self.magicAxeButton = nil;
    self.magicSwordButton = nil;
    self.magicBowButton = nil;
    self.magicWandButton = nil;
    self.mysticSwordButton = nil;
    self.weaponDict = nil;
}

-(IBAction) buttonPressed:(id)sender {
    NSString *weaponLetter = static_cast<NSString *>([weaponDict objectForKey:sender]);
    assert(weaponLetter != nil);
    EventHandler::getInstance()->getController()->notifyKeyPressed(char([weaponLetter characterAtIndex:0]));
}

- (void)dealloc {
    [handsButton release];
    [staffButton release];
    [daggerButton release];
    [slingButton release];
    [maceButton release];
    [axeButton release];
    [swordButton release];
    [bowButton release];
    [crossBowButton release];
    [oilButton release];
    [halberdButton release];
    [magicAxeButton release];
    [magicSwordButton release];
    [magicBowButton release];
    [magicWandButton release];
    [mysticSwordButton release];
    [weaponDict release];
    [super dealloc];
}


@end
