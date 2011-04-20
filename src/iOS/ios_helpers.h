/*
 *  ios_helpers.h
 *  xu4
 *
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
 */

// These functions exist primarily for sealing Objective-C away from the C++ code (i.e., I don't want to change the file types).

#ifdef IOS
#include <string>
typedef const struct __CFURL *CFURLRef;
typedef const struct __CFDictionary *CFDictionaryRef;
typedef const struct __CFArray *CFArrayRef;
typedef const struct __CFString *CFStringRef;
void startTicks();
int getTicks();

#if __OBJC__
@class U4View;
#else
typedef void U4View;
#endif

namespace U4IOS {
    enum IPadKeyboardType {
        UIKeyboardTypeDefault,
        UIKeyboardTypeASCIICapable,
        UIKeyboardTypeNumbersAndPunctuation,
        UIKeyboardTypeURL,
        UIKeyboardTypeNumberPad,
        UIKeyboardTypePhonePad,
        UIKeyboardTypeNamePhonePad,
        UIKeyboardTypeEmailAddress,
        UIKeyboardTypeAlphabet = UIKeyboardTypeASCIICapable
    };

    void updateView();
    void updateRectInView(int x, int y, int w, int h);
    std::string getFileLocation(const std::string &dir, const std::string &filename, const std::string &ext);
    ::CFURLRef copyAppSupportDirectoryLocation();
    void switchU4IntroControllerToContinueButton();
    void switchU4IntroControllerToABButtons();
    ::CFDictionaryRef createDictionaryForButtons(::CFArrayRef buttonArray,
                                                 ::CFStringRef arrayOfLetters, void * uibuttonCancelButton);
    U4View *frontU4View();
    
    void beginChoiceConversation();
    void maximizeChoicePanel();
    void restoreChoicePanel();
    void updateChoicesInDialog(const std::string &choices);
    void endChoiceConversation();
    
    void beginCharacterChoiceDialog();
    void endCharacterChoiceDialog();

    void beginMixSpellController();

    void beginCastSpellController();
    void endCastSpellController();
    
    void bringUpDirectionPopup(bool climbMode = false);
    void dismissDirectionPop();
    
    void hideGameButtons();
    void showGameButtons();
    
    void showWeaponDialog();
    void hideWeaponDialog();
    
    void showArmorDialog();
    void hideArmorDialog();

    class IOSHideGameControllerHelper {
    public:
        IOSHideGameControllerHelper() {
            hideGameButtons();
        }
        ~IOSHideGameControllerHelper() {
            showGameButtons();
        }
    };

    class IOSHideActionKeysHelper {
    public:
        IOSHideActionKeysHelper();
        ~IOSHideActionKeysHelper();
    };

    class IOSConversationHelper {
        bool conversationBegan;
        static std::string introString;
    public:
        IOSConversationHelper() : conversationBegan(false) {
        }
        ~IOSConversationHelper() {
            if (conversationBegan)
                endConversation();
        }
        
        // Set the Intro string for the next invocation of begin intro.
        static void setIntroString(const std::string &intro) { introString = intro; }

        void beginConversation(U4IOS::IPadKeyboardType conversationType, const std::string &intro = "");
    private:
        void endConversation();
    };
    
    class IOSConversationChoiceHelper {
    public:
        IOSConversationChoiceHelper() {
            beginChoiceConversation();
        }
        ~IOSConversationChoiceHelper() {
            endChoiceConversation();
        }
        void updateChoices(const std::string &choices) {
            updateChoicesInDialog(choices);
        }
        void fullSizeChoicePanel() {
            maximizeChoicePanel();
        }
        
        void halfSizeControlPanel() {
            restoreChoicePanel();
        }
    };
    
    class IOSCharacterChoiceHelper {
    public:
        IOSCharacterChoiceHelper() {
            beginCharacterChoiceDialog();
        }
        ~IOSCharacterChoiceHelper() {
            endCharacterChoiceDialog();
        }
    };
    
    class IOSCastSpellHelper {
    public:
        IOSCastSpellHelper() {
            beginCastSpellController();
        }
        ~IOSCastSpellHelper() {
            endCastSpellController();
        }
    };
    
    class IOSDirectionHelper {
    public:
        IOSDirectionHelper() {
            bringUpDirectionPopup();
        }
        virtual ~IOSDirectionHelper() {
            dismissDirectionPop();
        }
    };
    
    class IOSClimbHelper : public IOSDirectionHelper {
    public:
        IOSClimbHelper() {
            bringUpDirectionPopup(true);
        }
    };
    
    class IOSWeaponDialogHelper {
    public:
        IOSWeaponDialogHelper() {
            showWeaponDialog();
        }
        ~IOSWeaponDialogHelper() {
            hideWeaponDialog();
        }
    };
    
    class IOSArmorDialogHelper {
    public:
        IOSArmorDialogHelper() {
            showArmorDialog();
        }
        ~IOSArmorDialogHelper() {
            hideArmorDialog();
        }
    };
}

#endif

