//
//  U4ArmorChoiceDialog.mm
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

#import "U4ArmorChoiceDialog.h"
#include "context.h"
#include "event.h"
#include "ios_helpers.h"

@implementation U4ArmorChoiceDialog
@synthesize noneButton;
@synthesize clothButton;
@synthesize leatherButton;
@synthesize chainButton;
@synthesize plateButton;
@synthesize magicChainButton;
@synthesize magicPlateButton;
@synthesize mysticRobesButton;
@synthesize cancelButton;
@synthesize armorDict;

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
    const SaveGame * const savegame = c->saveGame;
    NSArray *allButtons = [NSArray arrayWithObjects:noneButton, clothButton, leatherButton, 
                                                    chainButton, plateButton, magicChainButton,
                                                    magicPlateButton, mysticRobesButton, nil];
    armorDict
        = const_cast<NSDictionary *>(reinterpret_cast<const NSDictionary *>(U4IOS::createDictionaryForButtons(reinterpret_cast<CFArrayRef>(allButtons),
                                                                                                           CFSTR("ABCDEFGH"), cancelButton)));    
    int i = 0;
    for (UIButton *button in allButtons) {
        if (i != 0) { // // You always have your skin, don't bother with this count stuff.
            short armorCount = savegame->armor[i];
            [button setTitle:[NSString stringWithFormat:@"%@ (%d)", button.currentTitle, armorCount]
                    forState:UIControlStateNormal];
            button.hidden = (armorCount == 0);
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
    // Release any retained subviews of the main view.
    self.noneButton = nil;
    self.clothButton = nil;
    self.leatherButton = nil;
    self.chainButton = nil;
    self.plateButton = nil;
    self.magicChainButton = nil;
    self.magicPlateButton = nil;
    self.mysticRobesButton = nil;
    self.cancelButton = nil;
    self.armorDict = nil;
    
}


- (void)dealloc {
    [noneButton release];
    [clothButton release];
    [leatherButton release];
    [chainButton release];
    [plateButton release];
    [magicChainButton release];
    [magicPlateButton release];
    [mysticRobesButton release];
    [cancelButton release];
    [armorDict release];
    [super dealloc];
}

-(IBAction) buttonPressed:(id)sender {
    NSString *armorLetter = static_cast<NSString *>([armorDict objectForKey:sender]);
    assert(armorLetter != nil);
    EventHandler::getInstance()->getController()->notifyKeyPressed(char([armorLetter characterAtIndex:0]));
}

@end
