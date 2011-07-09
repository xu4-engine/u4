//
//  CastSpellController.h
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

#import <UIKit/UIKit.h>
@class U4GameController;
@class U4Button;

@interface CastSpellController : UIViewController {
    U4GameController *gameController;
    IBOutlet U4Button *spellButton1;
    IBOutlet U4Button *spellButton2;
    IBOutlet U4Button *spellButton3;
    IBOutlet U4Button *spellButton4;
    IBOutlet U4Button *spellButton5;
    IBOutlet U4Button *spellButton6;
    IBOutlet U4Button *spellButton7;
    IBOutlet U4Button *spellButton8;
    IBOutlet U4Button *spellButton9;
    IBOutlet U4Button *spellButton10;
    IBOutlet U4Button *spellButton11;
    IBOutlet U4Button *spellButton12;
    IBOutlet U4Button *spellButton13;
    IBOutlet U4Button *spellButton14;
    IBOutlet U4Button *spellButton15;
    IBOutlet U4Button *spellButton16;
    IBOutlet U4Button *spellButton17;
    IBOutlet U4Button *spellButton18;
    IBOutlet U4Button *spellButton19;
    IBOutlet U4Button *spellButton20;
    IBOutlet U4Button *spellButton21;
    IBOutlet U4Button *spellButton22;
    
    IBOutlet UIButton *cancelButton;
    IBOutlet UILabel *noSpellLabel;
    NSDictionary *spellDict;
}
@property (nonatomic, retain) U4GameController *gameController;
@property (nonatomic, retain) IBOutlet UIButton *cancelButton;
@property (nonatomic, retain) IBOutlet UILabel *noSpellLabel;
@property (nonatomic, retain) IBOutlet NSDictionary *spellDict;
- (IBAction)buttonPressed:(id)sender;
@end
