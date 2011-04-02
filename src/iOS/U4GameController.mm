//
//  U4GameController.mm
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
#include "game.h"
#include "screen.h"
#include "tileset.h"
#import "CastSpellController.h"
#import "CharacterChoiceController.h"
#import "ConversationChoiceController.h"
#import "U4AppDelegate.h"
#import "U4ArmorChoiceDialog.h"
#import "U4ChooseDirectionPopup.h"
#import "U4GameController.h"
#import "U4WeaponChoiceDialog.h"
#include "U4CFHelper.h"
#include "ios_helpers.h"


static const  NSTimeInterval ALPHA_DURATION = 0.40;
static void hideButtonHelper(NSArray *buttons) {
    [UIView beginAnimations:nil context:NULL];
    [UIView setAnimationDuration:ALPHA_DURATION];
    for (UIButton *button in buttons)
        button.alpha = 0.0;
        [UIView commitAnimations];
    
}

static void showButtonHelper(NSArray *buttons) {
    [UIView beginAnimations:nil context:NULL];
    [UIView setAnimationDuration:ALPHA_DURATION];        
    for (UIButton *button in buttons)
        button.alpha = 1.0;
        [UIView commitAnimations];
    
}



extern bool gameSpellMixHowMany(int spell, int num, Ingredients *ingredients); // game.cpp

@implementation U4GameController
@synthesize conversationEdit;
@synthesize choiceController;
@synthesize actionDirectionController;
@synthesize castSpellController;
@synthesize characterChoiceController;
@synthesize currentPressedButton;
@synthesize gameButtonDict;
@synthesize attackButton;
@synthesize makeCampButton;
@synthesize talkButton;
@synthesize castButton;
@synthesize openDoorButton;
@synthesize searchButton;
@synthesize peerAtGemButton;
@synthesize superButton;
@synthesize useWeaponButton;
@synthesize fireCannonButton;
@synthesize lightTorchButton;
@synthesize partyOrderButton;
@synthesize wearArmorButton;
@synthesize currentPosButton;
@synthesize pickLockButton;
@synthesize statsButton;
@synthesize mixSpellButton;
@synthesize yellButton;
@synthesize useItemButton;
@synthesize saveButton;
@synthesize passButton;
@synthesize helpButton;
@synthesize upButton;
@synthesize leftButton;
@synthesize downButton;
@synthesize rightBUtton;
@synthesize weaponPanel;
@synthesize armorPanel;
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
    eventHandler->setControllerDone(false);
    conversationEdit.delegate = self;
    
    NSArray *gameButtons = [self allButtons];

    UInt8 keysForButtons[] = { 'a', 'c', U4_ENTER, 'f', 'h', 'i', 'j', 'l', 'm', 'n', 'o', 'p', 'q',
                               'r', 's', 't', 'u', 'w', 'y', 'z', U4_UP, U4_DOWN, U4_LEFT,
                               U4_RIGHT };
    boost::intrusive_ptr<CFString> allKeys = cftypeFromCreateOrCopy(
                                                CFStringCreateWithBytesNoCopy(kCFAllocatorDefault,
                                                                              keysForButtons, 24,
                                                                              kCFStringEncodingUTF8,
                                                                              false,
                                                                              kCFAllocatorNull));
    gameButtonDict = const_cast<NSDictionary *>(reinterpret_cast<const NSDictionary *>(
                                            U4IOS::createDictionaryForButtons(
                                                        reinterpret_cast<CFArrayRef>(gameButtons),
                                                        allKeys.get(), passButton)));
}

- (NSArray *)allButtons {
    NSArray *buttons = [self allButtonsButDirectionButtons];
    buttons = [buttons arrayByAddingObjectsFromArray:[NSArray arrayWithObjects:upButton, downButton,
                                                                               leftButton,
                                                                               rightBUtton,
                                                                               passButton,
                                                                               helpButton,
                                                                               nil]];
    return buttons;
}

- (NSArray *)allButtonsButDirectionButtons {
    return [NSArray arrayWithObjects:attackButton, castButton, superButton,
                     fireCannonButton, makeCampButton, lightTorchButton, pickLockButton,
                     currentPosButton, mixSpellButton, partyOrderButton, openDoorButton,
                     peerAtGemButton, saveButton, useWeaponButton, searchButton, talkButton,
                     useItemButton, wearArmorButton, yellButton, statsButton, nil];
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
    // Overriden to allow any orientation.
    return YES;
}

- (void)didReceiveMemoryWarning {
    // Releases the view if it doesn't have a superview.
    [super didReceiveMemoryWarning];
    Tileset::unloadAllImages();
    // Release any cached data, images, etc that aren't in use.
}

- (void)viewDidUnload {
    [self cleanUp];
    [super viewDidUnload];
    if (viewsReadytoFadeOutSet != nil) {
        [viewsReadytoFadeOutSet removeAllObjects];
        [viewsReadytoFadeOutSet release];
        viewsReadytoFadeOutSet = nil;
    }
    self.actionDirectionController = nil;
    self.conversationEdit = nil;
    self.choiceController = nil;
}

- (void)dealloc {
    [makeCampButton release];
    [attackButton release];
    [talkButton release];
    [castButton release];
    [openDoorButton release];
    [searchButton release];
    [peerAtGemButton release];
    [superButton release];
    [useWeaponButton release];
    [fireCannonButton release];
    [lightTorchButton release];
    [partyOrderButton release];
    [wearArmorButton release];
    [currentPosButton release];
    [pickLockButton release];
    [statsButton release];
    [mixSpellButton release];
    [yellButton release];
    [useItemButton release];
    [saveButton release];
    [passButton release];
    [helpButton release];
    [upButton release];
    [leftButton release];
    [downButton release];
    [rightBUtton release];
    [super dealloc];
}

- (void)cleanUp {
    // ### Double check, this may be called twice.
    eventHandler->popController();
    delete game;
    game = 0;
    Tileset::unloadAll();
    
    delete musicMgr;
    soundDelete();
    screenDelete();    
}

- (IBAction)buttonPressed:(id)sender {
    self.currentPressedButton = sender;
    NSString *gameLetter = static_cast<NSString *>([gameButtonDict objectForKey:sender]);
    assert(gameLetter != nil || sender == helpButton);
    if (sender == helpButton)
        EventHandler::getInstance()->getController()->notifyKeyPressed('h' + U4_ALT);
    else
        EventHandler::getInstance()->getController()->notifyKeyPressed(char([gameLetter characterAtIndex:0]));
}

- (IBAction)goUpPressed:(id)sender {
    [self buttonPressed:upButton];
}

- (IBAction)goLeftPressed:(id)sender {
    [self buttonPressed:leftButton];
}

- (IBAction)goDownPressed:(id)sender {
    [self buttonPressed:downButton];
}

- (IBAction)goRightPressed:(id)sender {
    [self buttonPressed:rightBUtton];
}

- (IBAction)klimbPressed:(id)sender {
    self.currentPressedButton = sender;
    EventHandler::getInstance()->getController()->notifyKeyPressed('k');
}

- (IBAction)descendPressed:(id)sender {
    self.currentPressedButton = sender;
    EventHandler::getInstance()->getController()->notifyKeyPressed('d');
}

- (void)bringUpMixReagentsController {
    MixSpellDialog *mixSpellDialog = [[MixSpellDialog alloc] initWithNibName:@"MixSpellDialog" bundle:nil];
    mixSpellDialog.delegate = self;
    U4AppDelegate *ourDelegate = [UIApplication sharedApplication].delegate;
    [ourDelegate.viewController presentModalViewController:mixSpellDialog animated:YES];
    [mixSpellDialog release];
}

-(void)mixSpellDialog:(MixSpellDialog *)dialog chooseSpell:(NSInteger)spellIndex reagents:(Ingredients *)reagents numberOfSpells:(NSInteger)amount {
    U4AppDelegate *ourDelegate = [UIApplication sharedApplication].delegate;
    [ourDelegate.viewController dismissModalViewControllerAnimated:YES];
    gameSpellMixHowMany(spellIndex, amount, reagents);
    [self buttonPressed:passButton];
}

-(void)mixWasCanceled:(MixSpellDialog *)dialog {
    U4AppDelegate *ourDelegate = [UIApplication sharedApplication].delegate;
    [ourDelegate.viewController dismissModalViewControllerAnimated:YES];
    [self buttonPressed:passButton];
}

- (void)popoverControllerDidDismissPopover:(UIPopoverController *)popoverController {
    [self buttonPressed:passButton];
}

- (void)bringUpDirectionPopupWithClimbMode:(BOOL)useClimbMode {
    [self bringUpDirectionPopupForButton:self.currentPressedButton climbMode:useClimbMode];
}

- (void)bringUpDirectionPopupForButton:(UIButton *)button climbMode:(BOOL)useClimbMode {
    U4ChooseDirectionPopup *directionPopup = [[[U4ChooseDirectionPopup alloc] initWithNibName:@"U4ChooseDirectionPopup" bundle:nil gameController:self] autorelease];
    directionPopup.climbMode = useClimbMode;
    UIPopoverController *popoverController = [[[UIPopoverController alloc] initWithContentViewController:directionPopup] autorelease];
    popoverController.delegate = self;
    self.actionDirectionController = popoverController;
    [self.actionDirectionController presentPopoverFromRect:button.frame inView:self.view permittedArrowDirections:UIPopoverArrowDirectionAny animated:YES];
}

- (void)dismissDirectionPopup {
    if (self.actionDirectionController) {
        [self.actionDirectionController dismissPopoverAnimated:YES];
        self.actionDirectionController = nil;
    }
    self.currentPressedButton = nil;
}

- (void)beginConversation:(UIKeyboardType)conversationType withGreeting:(NSString *)greeting {
    if (conversationEdit.hidden == YES) {
        conversationEdit.hidden = NO;
        if (greeting == nil)
            greeting = @"Ask about…";
        conversationEdit.placeholder = greeting;
        conversationEdit.keyboardType = conversationType;
    }
    [self hideAllButtons];
    [conversationEdit becomeFirstResponder];
    
}

- (void)hideAllButtons {
    buttonHideCount++;
    if (buttonHideCount == 1) {
        hideButtonHelper([self allButtons]);
    }
}

- (void)showAllButtons {
    buttonHideCount--;
    assert(buttonHideCount > -1);
    if (buttonHideCount == 0) {
        showButtonHelper([self allButtons]);
    }
}

- (void)hideAllButtonsMinusDirections {
    // No stacking, we shouldn't get in a case where that's necessary.
    hideButtonHelper([self allButtonsButDirectionButtons]);
    [self.passButton setTitle:@"Done" forState:UIControlStateNormal];
}


- (void)showAllButtonsMinusDirections {
    showButtonHelper([self allButtonsButDirectionButtons]);
    [self.passButton setTitle:@"Pass" forState:UIControlStateNormal];
}

- (void)endConversation {
    if (conversationEdit.hidden == NO) {
        [self showAllButtons];
        conversationEdit.text = nil;
        [conversationEdit resignFirstResponder];
        conversationEdit.hidden = YES;
    }
}

- (BOOL)textFieldShouldReturn:(UITextField *)textField {
    Controller *controller = EventHandler::getInstance()->getController();
    
    if (ReadStringController *rsController = dynamic_cast<ReadStringController *>(controller)) {
        NSString *text = textField.text;
        rsController->setValue([text UTF8String]);
        rsController->keyPressed('\r');
        textField.text = nil;
    } else if (ReadChoiceController *readChoiceController
               = dynamic_cast<ReadChoiceController *>(controller)) {
        readChoiceController->keyPressed('\n');
    }
    return YES;
}

- (void)bringUpChoicePanel {
    if (self.choiceController == nil) {
        choiceController = [[ConversationChoiceController alloc] initWithNibName:@"ConversationChoiceController" bundle:nil];
        choiceController.gameController = self;
    }
    [self finishBringUpPanel:choiceController.view halfSized:YES];

    /*
    UIView *choiceDialog = self.choiceController.view;
    [choiceDialog removeFromSuperview];
    [self.view addSubview:choiceDialog];
    [self halfSizeChoicePanel];
    [self hideAllButtons];
     */

    if (conversationEdit.hidden == YES) {
        inMidConversation = NO;
    } else {
        inMidConversation = YES;
        [conversationEdit resignFirstResponder];
        conversationEdit.hidden = YES;
    }
    [self updateChoices:nil];
}

- (void)fullSizeChoicePanel {
    if (!self.choiceController)
        return;
    [self slideViewFullIn:self.choiceController.view];
}

- (void)halfSizeChoicePanel {
    if (!self.choiceController)
        return;
    [self slideViewHalfwayIn:self.choiceController.view];
}

- (void)slideViewHalfwayIn:(UIView *)view {
    CGRect mySize = self.view.frame;
    [self slideViewIn:view 
           finalFrame:CGRectMake(0, conversationEdit.frame.size.height + 24,
                                 mySize.size.width, 
                                 mySize.size.height - conversationEdit.frame.size.height + 24)];
}

- (void)slideViewFullIn:(UIView *)view {
    CGRect mySize = self.view.frame;
    [self slideViewIn:view finalFrame:CGRectMake(0, 0, mySize.size.width, mySize.size.height)];    
}

- (void)updateChoices:(NSString *)choices {
    self.choiceController.choices = choices;
    self.choiceController.view.hidden = NO;
}

- (void)endChoiceConversation {
    if (inMidConversation == YES) {
        conversationEdit.hidden = NO;
        [conversationEdit becomeFirstResponder];
        inMidConversation = NO;
    }
    [self finishDismissPanel:self.choiceController.view];
}

- (void)bringUpCharacterPanel {
    if (self.characterChoiceController == nil) {
        characterChoiceController = [[CharacterChoiceController alloc]
                                     initWithNibName:@"CharacterChoiceController" bundle:nil];
    }
    [self finishBringUpPanel:self.characterChoiceController.view halfSized:NO];
}

- (void)endCharacterPanel {
    [self finishDismissPanel:self.characterChoiceController.view];
}

- (void)bringUpCastSpellController {
    if (self.castSpellController == nil) {
        castSpellController = [[CastSpellController alloc] initWithNibName:@"CastSpellController" bundle:nil];
        castSpellController.gameController = self;
    }
    [self finishBringUpPanel:castSpellController.view halfSized:NO];
}

- (void)endCastSpellController {
    self.currentPressedButton = nil;
    [self finishDismissPanel:self.castSpellController.view];
}

- (void)slideViewIn:(UIView *)view finalFrame:(CGRect)newFrame {
    CGRect mySize = self.view.frame;
    view.frame = CGRectMake(0, mySize.size.height, mySize.size.width, mySize.size.height);    
    [UIView beginAnimations:nil context:NULL];
    view.frame = newFrame;
    [UIView commitAnimations];
}

- (void)slideViewOut:(UIView *)view {
    [UIView beginAnimations:nil context:NULL];
    CGRect mySize = self.view.frame;
    view.frame = CGRectMake(0, mySize.size.height, mySize.size.width, mySize.size.height);    
    [UIView commitAnimations];
    if (viewsReadytoFadeOutSet == nil)
        viewsReadytoFadeOutSet = [[NSMutableSet alloc] initWithCapacity:16];
    [viewsReadytoFadeOutSet addObject:view];
    [NSTimer scheduledTimerWithTimeInterval:0.400 target:self 
                          selector:@selector(finishSlideOut:) userInfo:view repeats:NO];
}

- (void)finishSlideOut:(NSTimer *)theTimer {
    UIView *view = static_cast<UIView *>([theTimer userInfo]);
    if ([viewsReadytoFadeOutSet containsObject:view]) { // Don't do this unless we actually are in the set.
        [view removeFromSuperview];
        if (self.choiceController != nil && self.choiceController.view == view) {
            self.choiceController = nil;
        } else if (self.castSpellController != nil && self.castSpellController.view == view) {
            self.castSpellController = nil;
        } else if (self.characterChoiceController != nil
                   && self.characterChoiceController.view == view) {
            self.characterChoiceController = nil;
        } else if (self.weaponPanel != nil && self.weaponPanel.view == view) {
            self.weaponPanel = nil;
        } else if (self.armorPanel != nil && self.armorPanel.view == view) {
            self.armorPanel = nil;
        }
        [viewsReadytoFadeOutSet removeObject:view];
    }
    [theTimer invalidate];
}

- (void)finishBringUpPanel:(UIView *)view halfSized:(BOOL)halfSize {
    [view removeFromSuperview]; // This is overprotective, in case the controller wasn't killed before being invoked again.
    if (viewsReadytoFadeOutSet != nil && [viewsReadytoFadeOutSet containsObject:view])
        [viewsReadytoFadeOutSet removeObject:view];
        
    [self.view addSubview:view];
    [self hideAllButtons];
    if (halfSize) {
        [self slideViewHalfwayIn:view];
    } else {
        [self slideViewFullIn:view];
    }
    view.hidden = NO;    
}

- (void)finishDismissPanel:(UIView *)view {
    [self slideViewOut:view];
    [self showAllButtons];
}

- (void)bringUpWeaponChoicePanel {
    if (self.weaponPanel == nil) {
        weaponPanel = [[U4WeaponChoiceDialog alloc] initWithNibName:@"U4WeaponChoiceDialog" bundle:nil];
    }
    [self finishBringUpPanel:self.weaponPanel.view halfSized:NO];
}

- (void)dismissWeaponChoicePanel {
    [self finishDismissPanel:self.weaponPanel.view];
}

- (void)bringUpArmorChoicePanel {
    if (self.armorPanel == nil) {
        armorPanel = [[U4ArmorChoiceDialog alloc] initWithNibName:@"U4ArmorChoiceDialog" bundle:nil];
    }
    [self finishBringUpPanel:self.armorPanel.view halfSized:NO];
}

- (void)dismissArmorChoicePanel {
    [self finishDismissPanel:self.armorPanel.view];
}

@end
