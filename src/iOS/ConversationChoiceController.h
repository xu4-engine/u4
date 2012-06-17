//
//  ConversationChoiceController.h
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

@interface ConversationChoiceController : UIViewController {
@private    
    UIButton *choice1Button;
    UIButton *choice3Button;
    UIButton *choice2Button;
    UIButton *choice4Button;
    UIButton *choice5Button;
    UIButton *choice6Button;
    UIButton *choice7Button;
    UIButton *choice8Button;
    UIButton *choice9Button;
    UIButton *choice10Button;
    UIButton *choice11Button;
    UIButton *choice12Button;
    UIButton *choice13Button;
    UIButton *choice14Button;
    UIButton *choice15Button;
    UIButton *choice16Button;
    UIButton *noThanksButton;
    NSString *choices;
    NSString *target;
    int npcType;
    NSMutableDictionary *choiceButtonToStringDict;
    U4GameController *gameController;
}
@property (nonatomic, retain) IBOutlet UIButton *noThanksButton;
@property (nonatomic, retain) IBOutlet UIButton *choice16Button;
@property (nonatomic, retain) IBOutlet UIButton *choice15Button;
@property (nonatomic, retain) IBOutlet UIButton *choice14Button;
@property (nonatomic, retain) IBOutlet UIButton *choice13Button;
@property (nonatomic, retain) IBOutlet UIButton *choice12Button;
@property (nonatomic, retain) IBOutlet UIButton *choice11Button;
@property (nonatomic, retain) IBOutlet UIButton *choice10Button;
@property (nonatomic, retain) IBOutlet UIButton *choice9Button;
@property (nonatomic, retain) IBOutlet UIButton *choice8Button;
@property (nonatomic, retain) IBOutlet UIButton *choice7Button;
@property (nonatomic, retain) IBOutlet UIButton *choice6Button;
@property (nonatomic, retain) IBOutlet UIButton *choice5Button;
@property (nonatomic, retain) IBOutlet UIButton *choice4Button;
@property (nonatomic, retain) IBOutlet UIButton *choice1Button;
@property (nonatomic, retain) IBOutlet UIButton *choice3Button;
@property (nonatomic, retain) IBOutlet UIButton *choice2Button;
@property (nonatomic, retain) U4GameController *gameController;
- (IBAction)choiceButtonPressed:(id)sender;
- (IBAction)noThanksButtonPressed:(id)sender;
- (void)updateChoiceButtons;
- (NSArray *)allChoiceButtons;
- (void)setChoices:(NSString *)choices withTarget:(NSString *)target npcType:(int)npcType;
- (void)joinButton:(UIButton *)button withString:(NSString *)string buttonText:(NSString *)buttonText;
- (void)buildChoicesFromInventory;
- (void)buildGateSpellChoices;
- (void)buildEnergyFieldSpellChoices;
@end
