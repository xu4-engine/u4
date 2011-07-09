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

#include <CoreFoundation/CoreFoundation.h>
#import "CastSpellController.h"
#import "U4GameController.h"
#import "U4Button.h"
#include "event.h"
#include "context.h"
#include "spell.h"
#include "ios_helpers.h"
#include "U4CFHelper.h"
#include <vector>

@implementation CastSpellController
@synthesize gameController;
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
    
    const SaveGame * const saveGame = c->saveGame;
    NSArray *spellButtons = [NSArray arrayWithObjects:spellButton1, spellButton2, spellButton3,
                             spellButton4, spellButton5, spellButton6, spellButton7, spellButton8,
                             spellButton9, spellButton10, spellButton11, spellButton12,
                             spellButton13, spellButton14, spellButton15, spellButton16,
                             spellButton17, spellButton18, spellButton19, spellButton20,
                             spellButton21, spellButton22, nil];

    std::vector<const Spell *> castableSpells;
    castableSpells.reserve(SPELL_MAX);
    
    std::vector<char> castableLetters;
    castableLetters.reserve(SPELL_MAX);
    
    // Build the list of castable spells that the party can cast based on mixtures and context.
    for (int i = 0; i < SPELL_MAX; ++i) {
        short mixtureCount = saveGame->mixtures[i];
        if (mixtureCount == 0)
            continue;
        const Spell *spell = getSpell(i);
        if ((c->location->context & spell->context) 
            && (c->transportContext & spell->transportContext)) {
            castableSpells.push_back(spell);
            castableLetters.push_back(spell->name[0]);
        }
    }
    
    boost::intrusive_ptr<CFString> letters = cftypeFromCreateOrCopy(CFStringCreateWithBytes(kCFAllocatorDefault, 
                                                                                            reinterpret_cast<const UInt8 *>(castableLetters.data()),
                                                                                            castableLetters.size(), kCFStringEncodingUTF8, false));
    spellDict
        = const_cast<NSDictionary *>(reinterpret_cast<const NSDictionary *>(U4IOS::createDictionaryForButtons(reinterpret_cast<CFArrayRef>(spellButtons),
                                                                                                           letters.get(),
                                                                                                           cancelButton)));
    const int TotalSpells = castableSpells.size();
    for (int i = 0; i < TotalSpells; ++i) {
        const Spell *spell = castableSpells.at(i);
        U4Button *button = [spellButtons objectAtIndex:i];
        button.hidden = NO;
        [button setTitle:[NSString stringWithUTF8String:spell->name] forState:UIControlStateNormal];
    }

    self.noSpellLabel.hidden = (TotalSpells > 0);
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
    [spellButton1 release];
    spellButton1 = nil;
    [spellButton2 release];
    spellButton2 = nil;
    [spellButton3 release];
    spellButton3 = nil;
    [spellButton4 release];
    spellButton4 = nil;
    [spellButton5 release];
    spellButton5 = nil;
    [spellButton6 release];
    spellButton6 = nil;
    [spellButton7 release];
    spellButton7 = nil;
    [spellButton8 release];
    spellButton8 = nil;
    [spellButton9 release];
    spellButton9 = nil;
    [spellButton10 release];
    spellButton10 = nil;
    [spellButton11 release];
    spellButton11 = nil;
    [spellButton12 release];
    spellButton12 = nil;
    [spellButton13 release];
    spellButton13 = nil;
    [spellButton14 release];
    spellButton14 = nil;
    [spellButton15 release];
    spellButton15 = nil;
    [spellButton16 release];
    spellButton16 = nil;
    [spellButton17 release];
    spellButton17 = nil;
    [spellButton18 release];
    spellButton18 = nil;
    [spellButton19 release];
    spellButton19 = nil;
    [spellButton20 release];
    spellButton20 = nil;
    [spellButton21 release];
    spellButton21 = nil;
    [spellButton22 release];
    spellButton22 = nil;
    [super viewDidUnload];
    self.gameController.currentPressedButton = nil;
    // Release any retained subviews of the main view.
    // e.g. self.myOutlet = nil;
    self.gameController = nil;

    self.noSpellLabel = nil;
    self.cancelButton = nil;
    self.spellDict = nil;
}


- (void)dealloc {
    [gameController release];
    [noSpellLabel release];
    [cancelButton release];
    [spellDict release];
    [spellButton1 release];
    [spellButton2 release];
    [spellButton3 release];
    [spellButton4 release];
    [spellButton5 release];
    [spellButton6 release];
    [spellButton7 release];
    [spellButton8 release];
    [spellButton9 release];
    [spellButton10 release];
    [spellButton11 release];
    [spellButton12 release];
    [spellButton13 release];
    [spellButton14 release];
    [spellButton15 release];
    [spellButton16 release];
    [spellButton17 release];
    [spellButton18 release];
    [spellButton19 release];
    [spellButton20 release];
    [spellButton21 release];
    [spellButton22 release];
    [super dealloc];
}


- (IBAction)buttonPressed:(id)sender {
    gameController.currentPressedButton = sender;
    NSString *spellLetter = static_cast<NSString *>([spellDict objectForKey:sender]);
    assert(spellLetter != nil);
    EventHandler::getInstance()->getController()->notifyKeyPressed(char([spellLetter characterAtIndex:0]));    
}

@end
