/*
 *  ios_helpers.cpp
 *  xu4
 * Copyright 2011 Trenton Schulz. All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without modification, are
 * permitted provided that the following conditions are met:
 * 
 *    1. Redistributions of source code must retain the above copyright notice, this list of
 *       conditions and the following disclaimer.
 * 
 *    2. Redistributions in binary form must reproduce the above copyright notice, this list
 *       of conditions and the following disclaimer in the documentation and/or other materials
 *       provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY TRENTON SCHULZ ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * The views and conclusions contained in the software and documentation are those of the
 * authors and should not be interpreted as representing official policies, either expressed
 * or implied, of Trenton Schulz.
 *
 *
 */

#include "ios_helpers.h"
#import "U4View.h"
#import "U4AppDelegate.h"
#import "U4IntroController.h"
#import "U4GameController.h"
#import "U4ViewController.h"
#import <UIKit/UIKit.h>
#include <sys/time.h>
#include "U4CFHelper.h"
#include "imagemgr.h"
#include "view.h"
#include <CoreFoundation/CoreFoundation.h>

struct timeval start;
void startTicks()
{
    /* Set first ticks value */
    gettimeofday(&start, NULL);
}

int getTicks()
{
    int ticks;
    struct timeval now;
    gettimeofday(&now, NULL);
    ticks=(now.tv_sec-start.tv_sec)*1000+(now.tv_usec-start.tv_usec)/1000;
    return(ticks);
}

namespace U4IOS {
    
std::string IOSConversationHelper::introString;
    
CFDictionaryRef createDictionaryForButtons(CFArrayRef buttonArray,
                                           CFStringRef arrayOfLetters, void * uibuttonCancelButton) {
    
    int smallestItems = std::min(CFArrayGetCount(buttonArray), CFStringGetLength(arrayOfLetters));
    boost::intrusive_ptr<CFMutableDictionary> mutableDict 
                = cftypeFromCreateOrCopy(CFDictionaryCreateMutable(kCFAllocatorDefault,
                                                                   CFArrayGetCount(buttonArray) + 1,
                                                                   &kCFTypeDictionaryKeyCallBacks,
                                                                   &kCFTypeDictionaryValueCallBacks));
    const NSArray *allButtons = reinterpret_cast<const NSArray *>(buttonArray);
    int i = 0;
    for (UIButton *button in allButtons) {
        if (i >= smallestItems)
            break;
        unichar unilet = CFStringGetCharacterAtIndex(arrayOfLetters, i);
        NSString *letter = [NSString stringWithCharacters:&unilet length:1];
        CFDictionaryAddValue(mutableDict.get(), button, letter);     
        ++i;
    }
    CFDictionaryAddValue(mutableDict.get(), uibuttonCancelButton, @" ");
    CFRetain(mutableDict.get());
    return mutableDict.get();
}

void updateView() {
    [frontU4View() setNeedsDisplay];
}

void updateRectInView(int x, int y, int w, int h) {
    [frontU4View() setNeedsDisplayInRect:CGRectMake(x, y, w, h)];    
}

std::string getFileLocation(const std::string &dir, const std::string &filename, const std::string &ext) {
    NSBundle *bundle = [NSBundle mainBundle];
    NSString *path = [bundle pathForResource:[NSString stringWithUTF8String:filename.c_str()] 
                                      ofType:[NSString stringWithUTF8String:ext.c_str()]
                                 inDirectory:[NSString stringWithUTF8String:dir.c_str()]];
    return path == nil ? "" :  [path UTF8String];
}

CFURLRef copyAppSupportDirectoryLocation()
{
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory,
                                                         NSUserDomainMask, YES);
    // Just take the first one, if I can.
    NSString *finalPath = nil;
    for (NSString *path in paths) {
        finalPath = path;
        break;
    }
    if (finalPath != nil)
        return CFURLCreateWithFileSystemPath(kCFAllocatorDefault,
                                             reinterpret_cast<CFStringRef>(finalPath),
                                             kCFURLPOSIXPathStyle, YES);
    return 0;
}

static U4IntroController *introController() {
    U4AppDelegate *appDelegate = static_cast<U4AppDelegate *>([[UIApplication sharedApplication] delegate]);
    return appDelegate.introController;
}

static U4GameController *gameController() {
    U4AppDelegate *appDelegate = static_cast<U4AppDelegate *>([[UIApplication sharedApplication] delegate]);
    return appDelegate.gameController;
}

U4View *frontU4View() {
    U4AppDelegate *appDelegate = static_cast<U4AppDelegate *>([[UIApplication sharedApplication] delegate]);
    return [appDelegate frontU4View];
}

void drawMessageOnLabel(const std::string &message) {
    [gameController() showMessage:[NSString stringWithUTF8String:message.c_str()]];
}

void switchU4IntroControllerToContinueButton() {
    [introController() switchToContinueButtons];
}

void switchU4IntroControllerToABButtons() {
    [introController() switchToChoiceButtons];    
}
    
void hideGameButtons() {
    [gameController() hideAllButtons];
}

void showGameButtons() {
    [gameController() showAllButtons];
}

void IOSConversationHelper::beginConversation(U4IOS::IPadKeyboardType conversationType,
                                                 const std::string &intro) {
    conversationBegan = true;
    NSString *nsIntroString = nil;
    const std::string &tmpString = (intro.size() == 0 && introString.size() != 0) ? introString : intro;
    
    if (tmpString.size() != 0)
        nsIntroString = [NSString stringWithUTF8String:tmpString.c_str()];
    
    // Reset the introString because it wasn't locally overridden.
    if (intro.size() == 0)
        introString = "";
    [gameController() beginConversation:(UIKeyboardType)conversationType withGreeting:nsIntroString];
}
    
void IOSConversationHelper::endConversation() {
    [gameController() endConversation];
}

IOSHideActionKeysHelper::IOSHideActionKeysHelper() {
    [gameController() hideAllButtonsMinusDirections];
}

IOSHideActionKeysHelper::~IOSHideActionKeysHelper() {
    [gameController() showAllButtonsMinusDirections];     
}

void maximizeChoicePanel() {
    [gameController() fullSizeChoicePanel];
}

void restoreChoicePanel() {
    [gameController() halfSizeChoicePanel];
}
    
void updateChoicesInDialog(const std::string &choices, const std::string &target, int npcType) {
    [gameController() updateChoices:[NSString stringWithUTF8String:choices.c_str()]
                         withTarget:[NSString stringWithUTF8String:target.c_str()]
                        npcType:npcType];
}
    
    
void beginChoiceConversation() {
    [gameController() bringUpChoicePanel];
}
    
void endChoiceConversation() {
    [gameController() endChoiceConversation];
}
    
void beginCharacterChoiceDialog() {
    [gameController() bringUpCharacterPanel];
}

void endCharacterChoiceDialog() {
    [gameController() endCharacterPanel];
}

void beginCastSpellController() {
    [gameController() bringUpCastSpellController];
}

void endCastSpellController() {
    [gameController() endCastSpellController];
}

void bringUpDirectionPopup(bool climbMode) {
    [gameController() bringUpDirectionPopupWithClimbMode:climbMode];
}

void dismissDirectionPop() {
    [gameController() dismissDirectionPopup];
}

void showWeaponDialog() {
    [gameController() bringUpWeaponChoicePanel];
}

void hideWeaponDialog() {
    [gameController() dismissWeaponChoicePanel];
}

void showArmorDialog() {
    [gameController() bringUpArmorChoicePanel];
}

void hideArmorDialog() {
    [gameController() dismissArmorChoicePanel];
}
    
void beginMixSpellController() {
    [gameController() bringUpMixReagentsController];
}

void updateScreenView() {
    View::screen = imageMgr->get("screen")->image;
}
    
void disableGameButtons() {
    [gameController() disableGameButtons];
}

void enableGameButtons() {
    [gameController() enableGameButtons];        
}

void updateGameControllerContext(LocationContext context) {
    [gameController() updateGameControllerLocationContext:context];
}

}

