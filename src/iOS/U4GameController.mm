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
#include <QuartzCore/QuartzCore.h>
#include "context.h"
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
#import "U4PlayerTableController.h"
#import "U4View.h"
#include "U4CFHelper.h"
#include "ios_helpers.h"

static CGRect computeFrameRect(const CGRect &rect, UIInterfaceOrientation orientation) {
    CGRect retRect = rect;
    if (UIInterfaceOrientationIsLandscape(orientation)) {
        // Swap the size around.
        CGSize size = rect.size;
        retRect.size.height = size.width;
        retRect.size.width = size.height;
    }
    CGFloat oneThirdHeight = 243;
    retRect.origin.y = retRect.size.height - oneThirdHeight;
    retRect.size.height = oneThirdHeight;
    return retRect;
}

static NSString *shrinkOldText(NSString *currentText, NSUInteger lineThreshold,
                               NSUInteger currentNumberOfLines) {
    // This function tries to be a bit smart by limiting allocations.
    // Calculate the number lines we need to skip.
    // Skip ahead that many lines
    // Create a substring of the remaining lines.
    NSUInteger numberOfLinesToSkip = currentNumberOfLines - lineThreshold;
    NSUInteger linesTraversed = 0;
    NSUInteger textLength = [currentText length];
    NSRange range = NSMakeRange(0, textLength);
    while (range.location != NSNotFound && linesTraversed < numberOfLinesToSkip) {
        range = [currentText rangeOfString:@"\n" options:0 range:range];
        if (range.location != NSNotFound) {
            range = NSMakeRange(range.location + range.length, textLength - (range.location + range.length));
            ++linesTraversed;
        }
    }
    
    // The cool thing is that the range should be correct to create the substring.
    return [currentText substringWithRange:range];    
}

static NSUInteger computeLineCount(NSString *lines) {
    NSUInteger count = 0;
    NSUInteger textLength = [lines length];
    NSRange range = NSMakeRange(0, textLength); 
    while (range.location != NSNotFound) {
        range = [lines rangeOfString:@"\n" options:0 range:range];
        if (range.location != NSNotFound) {
            range = NSMakeRange(range.location + range.length, textLength - (range.location + range.length));
            ++count; 
        }
    }
    return count;
}

@implementation U4GameController(NavControllerDelegate)

- (void)navigationController:(UINavigationController *)navigationController willShowViewController:(UIViewController *)viewController animated:(BOOL)animated {
    if (viewController == playerTableController) {
        [navigationController setNavigationBarHidden:YES animated:YES];
        if (game) // The first time we are called the game hasn't been created yet, guard against that.
            game->paused = false;
    } else {
        [navigationController setNavigationBarHidden:NO animated:YES];
        game->paused = true;
    }
}

@end


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
@synthesize fireCannonButton;
@synthesize lightTorchButton;
@synthesize currentPosButton;
@synthesize pickLockButton;
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
@synthesize u4view;
@synthesize gameText;
@synthesize scrollImage;
@synthesize playerTableController;
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

    UInt8 keysForButtons[] = { 'a', 'c', U4_ENTER, 'f', 'h', 'i', 'j', 'l', 'm', 'o', 'p', 'q',
                               's', 't', 'u', 'y', U4_UP, U4_DOWN, U4_LEFT,
                               U4_RIGHT };
    boost::intrusive_ptr<CFString> allKeys = cftypeFromCreateOrCopy(
                                                CFStringCreateWithBytesNoCopy(kCFAllocatorDefault,
                                                                              keysForButtons, 20,
                                                                              kCFStringEncodingUTF8,
                                                                              false,
                                                                              kCFAllocatorNull));
    gameButtonDict = const_cast<NSDictionary *>(reinterpret_cast<const NSDictionary *>(
                                            U4IOS::createDictionaryForButtons(
                                                        reinterpret_cast<CFArrayRef>(gameButtons),
                                                        allKeys.get(), passButton)));
    U4AppDelegate *appDelegate = static_cast<U4AppDelegate *>([UIApplication sharedApplication].delegate);
    playerNavController = [[UINavigationController alloc] initWithRootViewController:self.playerTableController];
    playerNavController.navigationBar.hidden = YES;
    playerNavController.delegate = self;
    playerNavController.view.frame = CGRectMake(468, 21, 300, 410);
    playerNavController.view.layer.borderWidth = 2.;
    playerNavController.view.layer.borderColor = [UIColor lightGrayColor].CGColor;
    
    [self.view addSubview:playerNavController.view];
    [appDelegate pushU4View:self.u4view];
}

- (void)viewDidAppear:(BOOL)animated
{
    [super viewDidAppear:animated];
    [self.gameText flashScrollIndicators];
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
                     currentPosButton, mixSpellButton, openDoorButton,
                     peerAtGemButton, saveButton, searchButton, talkButton,
                     useItemButton, yellButton, nil];
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
    // Overriden to allow any orientation.
    return YES;
}

- (void)didReceiveMemoryWarning {
    // Releases the view if it doesn't have a superview.
    [super didReceiveMemoryWarning];
    // Release any cached data, images, etc that aren't in use.
    Tileset::unloadAllImages();
    [self clearText];
}

- (void)clearText {
    // This could probably be a little nice and not delete the current visible text...
    self.gameText.text = @"";
    lineCount = 0;
}

- (void)viewDidUnload {
    [self cleanUp];
    [self setU4view:nil];
    [self setGameText:nil];
    [self setScrollImage:nil];
    [playerTableController release];
    playerTableController = nil;
    [self setPlayerTableController:nil];
    [playerNavController release];
    playerNavController = nil;
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
    [playerNavController release];
    [makeCampButton release];
    [attackButton release];
    [talkButton release];
    [castButton release];
    [openDoorButton release];
    [searchButton release];
    [peerAtGemButton release];
    [superButton release];
    [fireCannonButton release];
    [lightTorchButton release];
    [currentPosButton release];
    [pickLockButton release];
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
    [u4view release];
    [gameText release];
    [scrollImage release];
    [playerTableController release];
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

- (void)willRotateToInterfaceOrientation:(UIInterfaceOrientation)toInterfaceOrientation
                                duration:(NSTimeInterval)duration {
    [super willRotateToInterfaceOrientation:toInterfaceOrientation duration:duration];
    
    // Only really change things if our orientation really is different, not just "upside down."
    if (UIInterfaceOrientationIsPortrait(self.interfaceOrientation) && UIInterfaceOrientationIsPortrait(toInterfaceOrientation))
        return;

    static const CGPoint PortaitPoint = CGPointMake(106., 12.);
    static const CGPoint LandscapePoint = CGPointMake(5, 10.);
    
    static const CGRect PortraitLogRect = CGRectMake(216., 332., 649., 229.);
    static const CGRect LandscapeLogRect = CGRectMake(435., 88., 318., 230.);

    static const CGRect PortraitScrollRect = CGRectMake(145., 266., 728., 351.);
    static const CGRect LandscapeScrollRect = CGRectMake(417., 10., 343., 351.);

    static const CGRect PortraitEditRect = CGRectMake(360., 540., 728, 31);
    static const CGRect LandscapeEditRect = CGRectMake(435, 415, 320, 31);
    
    static const CGRect PortraitPartyTableRect = CGRectMake(468, 21, 556, 153);
    static const CGRect LandscapePartyTableRect = CGRectMake(400, 10, 10, 660);
    
    CGRect u4frame = self.u4view.frame;
    if (UIInterfaceOrientationIsLandscape(toInterfaceOrientation)) {
        u4frame.origin = LandscapePoint;
        self.gameText.frame = LandscapeLogRect;
        self.conversationEdit.frame = LandscapeEditRect;
        self.scrollImage.frame = LandscapeScrollRect;
        playerNavController.view.frame = LandscapePartyTableRect;
    } else {
        u4frame.origin = PortaitPoint;
        self.gameText.frame = PortraitLogRect;
        self.conversationEdit.frame = PortraitEditRect;
        self.scrollImage.frame = PortraitScrollRect;
        playerNavController.view.frame = PortraitPartyTableRect;
    }
    self.u4view.frame = u4frame;
}

- (IBAction)buttonPressed:(id)sender {
    if (playerNavController.topViewController != playerTableController)
        [playerNavController popToViewController:playerTableController animated:YES];
    else
        [playerTableController finishPartyOrderEditing];
    self.currentPressedButton = sender;
    NSString *gameLetter = static_cast<NSString *>([gameButtonDict objectForKey:sender]);
    assert(gameLetter != nil || sender == helpButton);
    int gameChar = (sender == helpButton) ? 'h' + U4_ALT : char([gameLetter characterAtIndex:0]);
    EventHandler::getInstance()->getController()->notifyKeyPressed(gameChar);
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
    [self presentModalViewController:mixSpellDialog animated:YES];
    [mixSpellDialog release];
}

-(void)mixSpellDialog:(MixSpellDialog *)dialog chooseSpell:(NSInteger)spellIndex reagents:(Ingredients *)reagents numberOfSpells:(NSInteger)amount {
    [self dismissModalViewControllerAnimated:YES];
    gameSpellMixHowMany(spellIndex, amount, reagents);
    [self buttonPressed:passButton];
}

-(void)mixWasCanceled:(MixSpellDialog *)dialog {
    [self dismissModalViewControllerAnimated:YES];
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
    [self.actionDirectionController presentPopoverFromRect:button.frame inView:button.superview permittedArrowDirections:UIPopoverArrowDirectionAny animated:YES];
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
        [UIView animateWithDuration:U4IOS::ALPHA_DURATION animations:^{
            for (UIButton *button in [self allButtons]) {
                button.alpha = 0.0;
            }
        }];
    }
}

- (void)showAllButtons {
    buttonHideCount--;
    assert(buttonHideCount > -1);
    if (buttonHideCount == 0) {
        [UIView animateWithDuration:U4IOS::ALPHA_DURATION animations:^{
            for (UIButton *button in [self allButtons]) {
                button.alpha = 1.0;
            }
        }];
    }
}

- (void)hideAllButtonsMinusDirections {
    // No stacking, we shouldn't get in a case where that's necessary.
    [UIView animateWithDuration:U4IOS::ALPHA_DURATION animations:^{
        for (UIButton *button in [self allButtonsButDirectionButtons]) {
            button.alpha = 0.0;
        }
    }];
    [self.passButton setTitle:@"Done" forState:UIControlStateNormal];
}


- (void)showAllButtonsMinusDirections {
    [UIView animateWithDuration:U4IOS::ALPHA_DURATION animations:^{
        for (UIButton *button in [self allButtonsButDirectionButtons]) {
            button.alpha = 1.0;
        }
    }];
    [self.passButton setTitle:@"Pass" forState:UIControlStateNormal];
}

- (void)endConversation {
    if (conversationEdit.hidden == NO) {
        [self showAllButtons];
        [self updateGameControllerLocationContext:c->location->context];
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

    if (conversationEdit.hidden == YES) {
        inMidConversation = NO;
    } else {
        inMidConversation = YES;
        [conversationEdit resignFirstResponder];
        conversationEdit.hidden = YES;
    }
    [self updateChoices:nil withTarget:nil npcType:-1];
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
    CGRect finalFrame = computeFrameRect(self.view.frame, self.interfaceOrientation);
    const CGFloat offset = 0;//conversationEdit.frame.size.height + 24;
    finalFrame.origin.y += offset;
    finalFrame.size.height -= offset;
    [self slideViewIn:view finalFrame:finalFrame];
}

- (void)slideViewFullIn:(UIView *)view {
    [self slideViewIn:view finalFrame:computeFrameRect(self.view.frame, self.interfaceOrientation)];
}

- (void)updateChoices:(NSString *)choices withTarget:(NSString *)target npcType:(int)npcType;{
    [self.choiceController setChoices:choices withTarget:target npcType:npcType];
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
    [UIView animateWithDuration:U4IOS::SLIDE_DURATION animations:^{
        view.frame = newFrame;
    }];
}

- (void)slideViewOut:(UIView *)view {
    CGRect mySize = self.view.frame;
    [UIView animateWithDuration:U4IOS::SLIDE_DURATION animations:^{
        view.frame = CGRectMake(0, mySize.size.height, mySize.size.width, mySize.size.height);
    } completion:^(BOOL) {
        if ([viewsReadytoFadeOutSet containsObject:view]) { // Don't do this unless we actually are in the set.
            [view removeFromSuperview];
            if (self.choiceController != nil && self.choiceController.view == view) {
                self.choiceController = nil;
            } else if (self.castSpellController != nil && self.castSpellController.view == view) {
                self.castSpellController = nil;
            } else if (self.characterChoiceController != nil
                       && self.characterChoiceController.view == view) {
                self.characterChoiceController = nil;
            }
            [viewsReadytoFadeOutSet removeObject:view];
        }}];
    if (viewsReadytoFadeOutSet == nil)
        viewsReadytoFadeOutSet = [[NSMutableSet alloc] initWithCapacity:16];
    [viewsReadytoFadeOutSet addObject:view];
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
    [self updateGameControllerLocationContext:c->location->context];
}

- (void)bringUpWeaponChoicePanel {
}

- (void)dismissWeaponChoicePanel {
}

- (void)bringUpArmorChoicePanel {
}

- (void)dismissArmorChoicePanel {
}

- (void)showMessage:(NSString *)message {
    static const NSUInteger LineThreshold = 256; // Don't let this grow without bounds.
    if ([message compare:@"\n"] == NSOrderedSame)
        return;
    NSString *oldText = self.gameText.text;
    NSUInteger newLineCount = computeLineCount(message);

    if (newLineCount + lineCount > LineThreshold) {
        oldText = shrinkOldText(oldText, LineThreshold - newLineCount, lineCount);
        lineCount = LineThreshold - newLineCount;
    }
    lineCount += newLineCount;
    self.gameText.text = [oldText stringByAppendingString:message];
    [self.gameText scrollRangeToVisible:NSMakeRange([self.gameText.text length] - 1, 0)];
}

- (void)disableGameButtons {
    for (UIButton *button in [self allButtons]) {
        button.enabled = NO;
    }
}

- (void)enableGameButtons {
    for (UIButton *button in [self allButtons]) {
        button.enabled = YES;
    }
}

- (void)updateGameControllerLocationContext:(LocationContext)locationContext {
    NSMutableArray *buttonsToHide = [NSMutableArray arrayWithCapacity:32];
    NSMutableArray *buttonsToShow = [NSMutableArray arrayWithCapacity:32];
    
    switch (locationContext) {
    case CTX_CITY:
        [buttonsToHide addObject:self.lightTorchButton];
        [buttonsToHide addObject:self.makeCampButton];
        [buttonsToHide addObject:self.fireCannonButton];
        [buttonsToHide addObject:self.peerAtGemButton];

        [buttonsToShow addObject:self.currentPosButton];
        [buttonsToShow addObject:self.talkButton];
        [buttonsToShow addObject:self.pickLockButton];
        [buttonsToShow addObject:self.yellButton];
        [buttonsToShow addObject:self.searchButton];
        [buttonsToShow addObject:self.mixSpellButton];
        [buttonsToShow addObject:self.openDoorButton];
        if ([self.playerTableController orderButton])
            [buttonsToShow addObject:[self.playerTableController orderButton]];
        break;
    case CTX_COMBAT:
        [buttonsToHide addObject:self.currentPosButton];
        [buttonsToHide addObject:self.lightTorchButton];
        [buttonsToHide addObject:self.makeCampButton];
        [buttonsToHide addObject:self.talkButton];
        [buttonsToHide addObject:self.fireCannonButton];
        [buttonsToHide addObject:self.pickLockButton];
        [buttonsToHide addObject:self.peerAtGemButton];
        [buttonsToHide addObject:self.yellButton];
        [buttonsToHide addObject:self.searchButton];
        [buttonsToHide addObject:self.mixSpellButton];
        [buttonsToHide addObject:self.openDoorButton];
        if ([self.playerTableController orderButton])
            [buttonsToHide addObject:[self.playerTableController orderButton]];
        break;
    case CTX_DUNGEON:
        [buttonsToHide addObject:self.currentPosButton];
        [buttonsToHide addObject:self.talkButton];
        [buttonsToHide addObject:self.fireCannonButton];
        [buttonsToHide addObject:self.pickLockButton];
        [buttonsToHide addObject:self.yellButton];
        [buttonsToHide addObject:self.openDoorButton];

        [buttonsToShow addObject:self.peerAtGemButton];
        [buttonsToShow addObject:self.searchButton];
        [buttonsToShow addObject:self.mixSpellButton];
        [buttonsToShow addObject:self.lightTorchButton];
        [buttonsToShow addObject:self.makeCampButton];
        if ([self.playerTableController orderButton])
            [buttonsToShow addObject:[self.playerTableController orderButton]];
        break;
    case CTX_WORLDMAP:
        [buttonsToHide addObject:self.lightTorchButton];
        [buttonsToHide addObject:self.talkButton];
        [buttonsToHide addObject:self.pickLockButton];
        [buttonsToHide addObject:self.peerAtGemButton];
        [buttonsToHide addObject:self.openDoorButton];

        [buttonsToShow addObject:self.currentPosButton];
        [buttonsToShow addObject:self.makeCampButton];
        [buttonsToShow addObject:self.fireCannonButton];
        [buttonsToShow addObject:self.yellButton];
        [buttonsToShow addObject:self.searchButton];
        [buttonsToShow addObject:self.mixSpellButton];
        if ([self.playerTableController orderButton])           
            [buttonsToShow addObject:[self.playerTableController orderButton]];
        break;
    case CTX_ALTAR_ROOM:
        [buttonsToShow addObject:self.currentPosButton];

        [buttonsToHide addObject:self.lightTorchButton];
        [buttonsToHide addObject:self.makeCampButton];
        [buttonsToHide addObject:self.talkButton];
        [buttonsToHide addObject:self.fireCannonButton];
        [buttonsToHide addObject:self.pickLockButton];
        [buttonsToHide addObject:self.peerAtGemButton];
        [buttonsToHide addObject:self.yellButton];
        [buttonsToHide addObject:self.searchButton];
        [buttonsToHide addObject:self.mixSpellButton];
        [buttonsToHide addObject:self.openDoorButton];
        if ([self.playerTableController orderButton])
            [buttonsToHide addObject:[self.playerTableController orderButton]];
        break;
    default:
        break;
    }
    
    if (locationContext & CTX_CAN_SAVE_GAME) {
        [buttonsToShow addObject:self.saveButton];
    } else {
        [buttonsToHide addObject:self.saveButton];
    }
    
    [UIView animateWithDuration:U4IOS::ALPHA_DURATION animations:^{
        for (UIButton *button in buttonsToHide) {
            button.alpha = 0.0;
        }
        for (UIButton *button in buttonsToShow)
            button.alpha = 1.0;
    }];
}
  
@end
