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
#ifndef IOS_HELPERS_H
#define IOS_HELPERS_H
#include <string>
#include "observer.h"
#include "location.h"


#define XU4_DISABLE_COPY(Class) \
Class(const Class &); \
Class &operator=(const Class &);


typedef const struct __CFURL *CFURLRef;
typedef const struct __CFDictionary *CFDictionaryRef;
typedef const struct __CFArray *CFArrayRef;
typedef const struct __CFString *CFStringRef;
typedef struct CGContext * CGContextRef;
typedef struct CGColorSpace * CGColorSpaceRef;
typedef struct CGImage *CGImageRef;
typedef double NSTimeInterval;
struct CGRect;
void startTicks();
int getTicks();
class Aura;
class Party;
class PartyEvent;


#if __OBJC__
@class U4View;
#else
typedef void U4View;
#endif

namespace U4IOS {
    static const NSTimeInterval ALPHA_DURATION = 0.40;
    static const NSTimeInterval SLIDE_DURATION = 0.40;
    
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
    void updateScreenView();
    void testFlightPassCheckPoint(const std::string &checkPointName);
    
    void beginChoiceConversation();
    void maximizeChoicePanel();
    void restoreChoicePanel();
    void updateChoicesInDialog(const std::string &choices, const std::string &target, int npcType);
    void buildGateSpellChoices();
    void buildEnergyFieldSpellChoices();
    void endChoiceConversation();
    
    void beginCharacterChoiceDialog();
    void endCharacterChoiceDialog();

    void beginMixSpellController();

    void beginCastSpellController();
    void endCastSpellController();
    
    void bringUpDirectionPopup(bool climbMode = false);
    void dismissDirectionPop();
    
    void bringUpSuperButtonPopup();
    
    void hideGameButtons();
    void showGameButtons();
    
    void showWeaponDialog();
    void hideWeaponDialog();
    
    void showArmorDialog();
    void hideArmorDialog();

    void drawMessageOnLabel(const std::string &message);
    
    void disableGameButtons();
    void enableGameButtons();
    void updateGameControllerContext(LocationContext context);

    
    void updatePartyMemberData(const SaveGamePlayerRecord *partyMember);
    void reloadPartyMembers();
    void updateActivePartyMember(int row);
    void HIViewDrawCGImage(CGContextRef inContext, const CGRect *inBounds, CGImageRef inImage);
    
    class IOSObserver : public Observer<Aura *>, public Observer<Party *, PartyEvent &> {
    public:
        void update(Aura *aura);
        void update(Party *party, PartyEvent &event);
        static IOSObserver *sharedInstance();
    private:
        IOSObserver();
        static IOSObserver *instance;
        XU4_DISABLE_COPY(IOSObserver)
    };
    
    class IOSDisableGameControllerHelper {
    public:
        IOSDisableGameControllerHelper() {
            disableGameButtons();
        }
        ~IOSDisableGameControllerHelper() {
            enableGameButtons();
        }
    private:
        XU4_DISABLE_COPY(IOSDisableGameControllerHelper)
    };

    class IOSHideGameControllerHelper {
    public:
        IOSHideGameControllerHelper() {
            hideGameButtons();
        }
        ~IOSHideGameControllerHelper() {
            showGameButtons();
        }
    private:
        XU4_DISABLE_COPY(IOSHideGameControllerHelper)
    };
    
    void incrementConversationCount();
    void decrementConversationCount();

    class IOSHideActionKeysHelper {
    public:
        IOSHideActionKeysHelper();
        ~IOSHideActionKeysHelper();
    private:
        XU4_DISABLE_COPY(IOSHideActionKeysHelper)
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
        XU4_DISABLE_COPY(IOSConversationHelper)
    };
    
    class IOSConversationChoiceHelper {
    public:
        IOSConversationChoiceHelper() {
            beginChoiceConversation();
        }
        ~IOSConversationChoiceHelper() {
            endChoiceConversation();
        }
        void updateChoices(const std::string &choices, const std::string &target = "", int npcType = -1) {
            updateChoicesInDialog(choices, target, npcType);
        }
        void updateGateSpellChoices() {
            buildGateSpellChoices();
        }
        void updateEnergyFieldSpellChoices() {
            buildEnergyFieldSpellChoices();
        }
        void fullSizeChoicePanel() {
            maximizeChoicePanel();
        }
        
        void halfSizeControlPanel() {
            restoreChoicePanel();
        }
    private:
        XU4_DISABLE_COPY(IOSConversationChoiceHelper)
    };
    
    class IOSCastSpellHelper {
    public:
        IOSCastSpellHelper() {
            beginCastSpellController();
        }
        ~IOSCastSpellHelper() {
            endCastSpellController();
        }
    private:
        XU4_DISABLE_COPY(IOSCastSpellHelper)
    };
    
    class IOSDirectionHelper {
    public:
        IOSDirectionHelper() {
            bringUpDirectionPopup();
        }
        ~IOSDirectionHelper() {
            dismissDirectionPop();
        }
    private:
        XU4_DISABLE_COPY(IOSDirectionHelper)
    };
    
    class IOSClimbHelper {
    public:
        IOSClimbHelper() {
            bringUpDirectionPopup(true);
        }
        ~IOSClimbHelper() {
            dismissDirectionPop();
        }
    private:
        XU4_DISABLE_COPY(IOSClimbHelper)
    };

    class IOSSuperButtonHelper {
    public:
        IOSSuperButtonHelper() {
            bringUpSuperButtonPopup();
        }
        ~IOSSuperButtonHelper() {
            dismissDirectionPop(); // ### This dismisses the "super" popup as well.
        }
    private:
        XU4_DISABLE_COPY(IOSSuperButtonHelper)
    };

    CGColorSpaceRef u4colorSpace();
    CFStringRef playerStatusAsString(StatusType status);
    CFStringRef playerClassAsString(ClassType cclass);
    CFStringRef weaponAsString(WeaponType weapon);
    CFStringRef armorAsString(ArmorType armor);
    CFStringRef reagentAsString(Reagent reagent);
    void updateOtherPartyStats();
    void syncPartyMembersWithSaveGame();
}
#endif // IOS_HELPERS_H
#endif // IOS

