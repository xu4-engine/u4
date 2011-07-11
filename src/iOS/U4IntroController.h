//
//  U4IntroController.h
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

#import "U4StartDialog.h"
#import "CreditsViewController.h"

@class U4View;
@interface U4IntroController : UIViewController  <StartGameDialogDelegate, CreditsViewControllerDelegate> {
    U4StartDialogController *startGame;
    UIButton *startButton;
    UIButton *loadButton;
    UIButton *continueButton;
    UIButton *choiceAButton;
    UIButton *choiceBButton;
    UIButton *creditsButton;
    U4View *u4view;
    BOOL finishFirstTimeLoad;
}
- (IBAction)choiceAClicked:(id)sender;
- (IBAction)choiceBClicked:(id)sender;
- (IBAction)startGame:(id)sender;
- (IBAction)loadGame:(id)sender;
- (IBAction)continuePressed:(id)sender;
- (void)switchToContinueButtons;
- (void)switchToChoiceButtons;
- (void)launchGameController;
- (IBAction)showCredits:(id)sender;

@property (nonatomic, retain) IBOutlet UIButton *startButton;
@property (nonatomic, retain) IBOutlet UIButton *loadButton;
@property (nonatomic, retain) IBOutlet UIButton *continueButton;
@property (nonatomic, retain) IBOutlet UIButton *choiceAButton;
@property (nonatomic, retain) IBOutlet UIButton *choiceBButton;
@property (nonatomic, retain) IBOutlet UIButton *creditsButton;
@property (nonatomic, retain) IBOutlet U4View *u4view;

@end
