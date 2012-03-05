//
//  U4GameController.h
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
#import "MixSpellDialog.h"
#include "location.h"

class GameController;

@class ConversationChoiceController;
@class CastSpellController;
@class CharacterChoiceController;
@class U4View;
@class U4PlayerTableController;

@interface U4GameController : UIViewController<UITextFieldDelegate, UIPopoverControllerDelegate, MixSpellDialogDelegate, UINavigationControllerDelegate> {
@private
    GameController *gameController;
    UITextField *conversationEdit;
    ConversationChoiceController *choiceController;
    UIPopoverController *actionDirectionController;
    CastSpellController *castSpellController;
    UINavigationController *playerNavController;
    BOOL inMidConversation;
    UIButton *currentPressedButton;
    NSDictionary *gameButtonDict;
    UIButton *attackButton;
    UIButton *makeCampButton;
    UIButton *talkButton;
    UIButton *castButton;
    UIButton *openDoorButton;
    UIButton *searchButton;
    UIButton *peerAtGemButton;
    UIButton *superButton;
    UIButton *fireCannonButton;
    UIButton *lightTorchButton;
    UIButton *currentPosButton;
    UIButton *pickLockButton;
    UIButton *mixSpellButton;
    UIButton *yellButton;
    UIButton *useItemButton;
    UIButton *saveButton;
    UIButton *passButton;
    UIButton *helpButton;
    UIButton *upButton;
    UIButton *leftButton;
    UIButton *downButton;
    UIButton *rightBUtton;
    U4View *u4view;
    UITextView *gameText;
    UIImageView *scrollImage;
    U4PlayerTableController *playerTableController;
    NSMutableSet *viewsReadytoFadeOutSet;
    NSUInteger lineCount;
    NSUInteger conversationCount;
    NSTimer *repeatButtonTimer;
}
@property (nonatomic, retain) CastSpellController *castSpellController;
@property (nonatomic, retain) ConversationChoiceController *choiceController;
@property (nonatomic, retain) CharacterChoiceController *characterChoiceController;
@property (nonatomic, retain) UIPopoverController *actionDirectionController;
@property (nonatomic, retain) IBOutlet UITextField *conversationEdit;
@property (nonatomic, retain) UIButton *currentPressedButton;
@property (nonatomic, retain) NSDictionary *gameButtonDict;
@property (nonatomic, retain) IBOutlet UIButton *attackButton;
@property (nonatomic, retain) IBOutlet UIButton *makeCampButton;
@property (nonatomic, retain) IBOutlet UIButton *talkButton;
@property (nonatomic, retain) IBOutlet UIButton *castButton;
@property (nonatomic, retain) IBOutlet UIButton *openDoorButton;
@property (nonatomic, retain) IBOutlet UIButton *searchButton;
@property (nonatomic, retain) IBOutlet UIButton *peerAtGemButton;
@property (nonatomic, retain) IBOutlet UIButton *superButton;
@property (nonatomic, retain) IBOutlet UIButton *fireCannonButton;
@property (nonatomic, retain) IBOutlet UIButton *lightTorchButton;
@property (nonatomic, retain) IBOutlet UIButton *currentPosButton;
@property (nonatomic, retain) IBOutlet UIButton *pickLockButton;
@property (nonatomic, retain) IBOutlet UIButton *mixSpellButton;
@property (nonatomic, retain) IBOutlet UIButton *yellButton;
@property (nonatomic, retain) IBOutlet UIButton *useItemButton;
@property (nonatomic, retain) IBOutlet UIButton *saveButton;
@property (nonatomic, retain) IBOutlet UIButton *passButton;
@property (nonatomic, retain) IBOutlet UIButton *helpButton;
@property (nonatomic, retain) IBOutlet UIButton *upButton;
@property (nonatomic, retain) IBOutlet UIButton *leftButton;
@property (nonatomic, retain) IBOutlet UIButton *downButton;
@property (nonatomic, retain) IBOutlet UIButton *rightBUtton;
@property (nonatomic, retain) IBOutlet U4View *u4view;
@property (nonatomic, retain) IBOutlet UITextView *gameText;
@property (nonatomic, retain) IBOutlet UIImageView *scrollImage;
@property (nonatomic, retain) IBOutlet U4PlayerTableController *playerTableController;

- (IBAction)buttonPressed:(id)sender;
- (IBAction)goUpPressed:(id)sender;
- (IBAction)goLeftPressed:(id)sender;
- (IBAction)goDownPressed:(id)sender;
- (IBAction)goRightPressed:(id)sender;
- (IBAction)klimbPressed:(id)sender;
- (IBAction)descendPressed:(id)sender;
- (IBAction)exitPressed:(id)sender;
- (IBAction)repeatButtonPressed:(id)sender;
- (IBAction)repeatButtonReleased:(id)sender;
- (void)bringUpSuperButtonBreakdown;
- (void)bringUpSuperButtonBreakdownForButton:(UIButton *)button;
- (void)bringUpMixReagentsController;
- (void)cleanUp;
- (void)bringUpDirectionPopupWithClimbMode:(BOOL)climbMode;
- (void)fullSizeChoicePanel;
- (void)halfSizeChoicePanel;
- (void)slideViewHalfwayIn:(UIView *)view;
- (void)slideViewFullIn:(UIView *)view;
- (void)bringUpDirectionPopupForButton:(UIButton *)button climbMode:(BOOL)useClimbMode;
- (void)dismissDirectionPopup;
- (void)beginConversation:(UIKeyboardType)conversationType withGreeting:(NSString *)greeting;
- (void)endConversation;
- (void)hideAllButtons;
- (void)showAllButtons;
- (void)hideAllButtonsMinusDirections;
- (void)showAllButtonsMinusDirections;
- (void)bringUpChoicePanel;
- (void)updateChoices:(NSString *)choices withTarget:(NSString *)target npcType:(int)npcType;
- (void)endChoiceConversation;
- (void)bringUpCharacterPanel;
- (void)endCharacterPanel;
- (void)bringUpCastSpellController;
- (void)endCastSpellController;
- (void)finishBringUpPanel:(UIView *)view halfSized:(BOOL)halfSize;
- (void)finishDismissPanel:(UIView *)view;
- (void)slideViewIn:(UIView *)view finalFrame:(CGRect)newFrame;
- (void)slideViewOut:(UIView *)view;
- (void)showMessage:(NSString *)message;
- (void)clearText;
- (void)disableGameButtons;
- (void)enableGameButtons;
- (void)updateGameControllerLocationContext:(LocationContext)locationContext;
- (void)incrementConversationCount;
- (void)decrementConversationCount;
- (NSArray *)allButtons;
- (NSArray *)allButtonsButDirectionButtons;
@end
