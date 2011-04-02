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

@interface CastSpellController : UIViewController {
    U4GameController *gameController;
    IBOutlet UIButton *awakenButton;
    IBOutlet UIButton *blinkButton;
    IBOutlet UIButton *cureButton;
    IBOutlet UIButton *dispelButton;
    IBOutlet UIButton *energyFieldButton;
    IBOutlet UIButton *fireballButton;
    IBOutlet UIButton *gateTravelButton;
    IBOutlet UIButton *healButton;
    IBOutlet UIButton *iceballButton;
    IBOutlet UIButton *jinxButton;
    IBOutlet UIButton *killButton;
    IBOutlet UIButton *lightButton;
    IBOutlet UIButton *magicMissleButton;
    IBOutlet UIButton *negateButton;
    IBOutlet UIButton *openButton;
    IBOutlet UIButton *protectionButton;
    IBOutlet UIButton *quicknessButton;
    IBOutlet UIButton *resurrectButton;
    IBOutlet UIButton *sleepButton;
    IBOutlet UIButton *tremorButton;
    IBOutlet UIButton *undeadButton;
    IBOutlet UIButton *viewButton;
    IBOutlet UIButton *windChangeButton;
    IBOutlet UIButton *xitButton;
    IBOutlet UIButton *yupPressed;
    IBOutlet UIButton *zdownPressed;
    IBOutlet UIButton *cancelButton;
    IBOutlet UILabel *noSpellLabel;
    NSDictionary *spellDict;
}
@property (nonatomic, retain) U4GameController *gameController;
@property (nonatomic, retain) IBOutlet UIButton *awakenButton;
@property (nonatomic, retain) IBOutlet UIButton *blinkButton;
@property (nonatomic, retain) IBOutlet UIButton *cureButton;
@property (nonatomic, retain) IBOutlet UIButton *dispelButton;
@property (nonatomic, retain) IBOutlet UIButton *energyFieldButton;
@property (nonatomic, retain) IBOutlet UIButton *fireballButton;
@property (nonatomic, retain) IBOutlet UIButton *gateTravelButton;
@property (nonatomic, retain) IBOutlet UIButton *healButton;
@property (nonatomic, retain) IBOutlet UIButton *iceballButton;
@property (nonatomic, retain) IBOutlet UIButton *jinxButton;
@property (nonatomic, retain) IBOutlet UIButton *killButton;
@property (nonatomic, retain) IBOutlet UIButton *lightButton;
@property (nonatomic, retain) IBOutlet UIButton *magicMissleButton;
@property (nonatomic, retain) IBOutlet UIButton *negateButton;
@property (nonatomic, retain) IBOutlet UIButton *openButton;
@property (nonatomic, retain) IBOutlet UIButton *protectionButton;
@property (nonatomic, retain) IBOutlet UIButton *quicknessButton;
@property (nonatomic, retain) IBOutlet UIButton *resurrectButton;
@property (nonatomic, retain) IBOutlet UIButton *sleepButton;
@property (nonatomic, retain) IBOutlet UIButton *tremorButton;
@property (nonatomic, retain) IBOutlet UIButton *undeadButton;
@property (nonatomic, retain) IBOutlet UIButton *viewButton;
@property (nonatomic, retain) IBOutlet UIButton *windChangeButton;
@property (nonatomic, retain) IBOutlet UIButton *xitButton;
@property (nonatomic, retain) IBOutlet UIButton *yupButton;
@property (nonatomic, retain) IBOutlet UIButton *zdownButton;
@property (nonatomic, retain) IBOutlet UIButton *cancelButton;
@property (nonatomic, retain) IBOutlet UILabel *noSpellLabel;
@property (nonatomic, retain) IBOutlet NSDictionary *spellDict;
- (IBAction)buttonPressed:(id)sender;
@end
