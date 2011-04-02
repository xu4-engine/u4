//
//  U4WeaponChoiceDialog.h
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


@interface U4WeaponChoiceDialog : UIViewController {
    IBOutlet UIButton *handsButton;
    IBOutlet UIButton *staffButton;
    IBOutlet UIButton *daggerButton;
    IBOutlet UIButton *slingButton;
    IBOutlet UIButton *maceButton;
    IBOutlet UIButton *axeButton;
    IBOutlet UIButton *swordButton;
    IBOutlet UIButton *bowButton;
    IBOutlet UIButton *crossBowButton;
    IBOutlet UIButton *oilButton;
    IBOutlet UIButton *halberdButton;
    IBOutlet UIButton *magicAxeButton;
    IBOutlet UIButton *magicSwordButton;
    IBOutlet UIButton *magicBowButton;
    IBOutlet UIButton *magicWandButton;
    IBOutlet UIButton *mysticSwordButton;
    IBOutlet UIButton *cancelButton;
    NSDictionary *weaponDict;
}
@property (nonatomic, retain) IBOutlet UIButton *handsButton;
@property (nonatomic, retain) IBOutlet UIButton *staffButton;
@property (nonatomic, retain) IBOutlet UIButton *daggerButton;
@property (nonatomic, retain) IBOutlet UIButton *slingButton;
@property (nonatomic, retain) IBOutlet UIButton *maceButton;
@property (nonatomic, retain) IBOutlet UIButton *axeButton;
@property (nonatomic, retain) IBOutlet UIButton *swordButton;
@property (nonatomic, retain) IBOutlet UIButton *bowButton;
@property (nonatomic, retain) IBOutlet UIButton *crossBowButton;
@property (nonatomic, retain) IBOutlet UIButton *oilButton;
@property (nonatomic, retain) IBOutlet UIButton *halberdButton;
@property (nonatomic, retain) IBOutlet UIButton *magicAxeButton;
@property (nonatomic, retain) IBOutlet UIButton *magicSwordButton;
@property (nonatomic, retain) IBOutlet UIButton *magicBowButton;
@property (nonatomic, retain) IBOutlet UIButton *magicWandButton;
@property (nonatomic, retain) IBOutlet UIButton *mysticSwordButton;
@property (nonatomic, retain) IBOutlet UIButton *cancelButton;
@property (nonatomic, retain) NSDictionary *weaponDict;

-(IBAction) buttonPressed:(id) sender;
@end
