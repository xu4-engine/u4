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
#import "U4PlayerTableController.h"
#import "PartyStatusImageView.h"
#import "U4ViewController.h"
#import <UIKit/UIKit.h>
#include <sys/time.h>
#include "U4CFHelper.h"
#include "aura.h"
#include "context.h"
#include "player.h"
#include "imagemgr.h"
#include "view.h"
#include "CGContextGStateSaver.h"
#include <CoreFoundation/CoreFoundation.h>
#import "TestFlight.h"

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
    
void testFlightPassCheckPoint(const std::string &checkPointName) {
    [TestFlight passCheckpoint:[NSString stringWithUTF8String:checkPointName.c_str()]];
}
    
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
    
void incrementConversationCount() {
    [gameController() incrementConversationCount];
}

void decrementConversationCount() {
    [gameController() decrementConversationCount];
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

void updatePartyMemberData(const SaveGamePlayerRecord *partyMember) {
    [gameController().playerTableController updatePartyMemberData:partyMember];
}

void reloadPartyMembers() {
    [gameController().playerTableController reloadPartyMembers];
}

void updateActivePartyMember(int row) {
    [gameController().playerTableController updateActivePartyMember:row];
}
    
void updateOtherPartyStats() {
    [gameController().playerTableController updateOtherPartyStats];
}

void updateGameControllerContext(LocationContext context) {
    [gameController() updateGameControllerLocationContext:context];
}

IOSObserver *IOSObserver::instance = 0;

IOSObserver::IOSObserver() {
    
}
    
IOSObserver *IOSObserver::sharedInstance() {
    if (instance == 0) {
        instance = new IOSObserver();
    }
    return  instance;
}
    
void IOSObserver::update(Aura *aura) {
    switch (aura->getType()) {
    case Aura::NONE:
        [gameController().playerTableController.partyStatusImage drawEigths];
        break;
    case Aura::HORN:
        [gameController().playerTableController.partyStatusImage drawHorn];
        break;
    case Aura::JINX:
        [gameController().playerTableController.partyStatusImage drawJinx];
        break;
    case Aura::NEGATE:
        [gameController().playerTableController.partyStatusImage drawNegate];
        break;
    case Aura::PROTECTION:
        [gameController().playerTableController.partyStatusImage drawProtection];
        break;
    case Aura::QUICKNESS:
        [gameController().playerTableController.partyStatusImage drawQuickness];
        break;
    }
}
    
class ObservantPartyMember : public PartyMember {
public:
    explicit ObservantPartyMember(const PartyMember *pm) : PartyMember(*pm) { }
    SaveGamePlayerRecord *player() const { return PartyMember::player; }
};
    
void IOSObserver::update(Party *party, PartyEvent &event) {

    switch (event.type) {
        case PartyEvent::MEMBER_JOINED:
            reloadPartyMembers();
            break;
        case PartyEvent::GENERIC:
        case PartyEvent::ADVANCED_LEVEL:
        case PartyEvent::ACTIVE_PLAYER_CHANGED:
            if (event.player) {
                ObservantPartyMember pm(event.player);
                updatePartyMemberData(pm.player());
            }
            break;
        case PartyEvent::LOST_EIGHTH:
            update(c->aura);
            break;
        default:
            NSLog(@"Unhandled PartyEvent type %d reloading everything", event.type);
            reloadPartyMembers();
            break;
    }
    updateOtherPartyStats();
}

CFStringRef playerStatusAsString(StatusType status) {
    NSString *statusString = nil;
    switch (status) {
        case STAT_DEAD:
            statusString = NSLocalizedString(@"Dead", "Character Status");
            break;
        case STAT_GOOD:
            statusString = NSLocalizedString(@"Good", "Character Status");
            break;
        case STAT_POISONED:
            statusString = NSLocalizedString(@"Poisoned", "Character Status");
            break;
        case STAT_SLEEPING:
            statusString = NSLocalizedString(@"Sleeping", "Character Status");
            break;            
        default:
            break;
    }
    return reinterpret_cast<CFStringRef>(statusString);
}
    
CFStringRef playerClassAsString(ClassType cclass) {
    NSString *classString = nil;
    switch (cclass) {
    case CLASS_BARD:
        classString = NSLocalizedString(@"Bard", "Character Class");
        break;
    case CLASS_DRUID:
        classString = NSLocalizedString(@"Druid", "Character Class");
        break;
    case CLASS_FIGHTER:
        classString = NSLocalizedString(@"Fighter", "Character Class");
        break;
    case CLASS_MAGE:
        classString = NSLocalizedString(@"Mage", "Character Class");
        break;
    case CLASS_PALADIN:
        classString = NSLocalizedString(@"Paladin", "Character Class");
        break;
    case CLASS_RANGER:
        classString = NSLocalizedString(@"Ranger", "Character Class");
        break;
    case CLASS_SHEPHERD:
        classString = NSLocalizedString(@"Shepard", "Character Class");
        break;
    case CLASS_TINKER:
        classString = NSLocalizedString(@"Tinker", "Character Class");
        break;            
    default:
        break;
    }
    return reinterpret_cast<CFStringRef>(classString);
}
    
CFStringRef weaponAsString(WeaponType weapon) {
    NSString *weaponString = nil;
    switch (weapon) {
    case WEAP_HANDS:
        weaponString = NSLocalizedString(@"Hands", "Weapon Type");
        break;
    case WEAP_DAGGER:
        weaponString = NSLocalizedString(@"Dagger", "Weapon Type");
        break;
    case WEAP_MACE:
        weaponString = NSLocalizedString(@"Mace", "Weapon Type");
        break;
    case WEAP_AXE:
        weaponString = NSLocalizedString(@"Axe", "Weapon Type");
        break;
    case WEAP_OIL:
        weaponString = NSLocalizedString(@"Flaming Oil", "Weapon Type");
        break;
    case WEAP_STAFF:
        weaponString = NSLocalizedString(@"Staff", "Weapon Type");
        break;
    case WEAP_SLING:
        weaponString = NSLocalizedString(@"Sling", "Weapon Type");
        break;
    case WEAP_SWORD:
        weaponString = NSLocalizedString(@"Sword", "Weapon Type");
        break;
    case WEAP_BOW:
        weaponString = NSLocalizedString(@"Bow", "Weapon Type");
        break;
    case WEAP_CROSSBOW:
        weaponString = NSLocalizedString(@"Crossbow", "Weapon Type");
        break;
    case WEAP_HALBERD:
        weaponString = NSLocalizedString(@"Halberd", "Weapon Type");
        break;
    case WEAP_MAGICAXE:
        weaponString = NSLocalizedString(@"Magic Axe", "Weapon Type");
        break;
    case WEAP_MAGICBOW:
        weaponString = NSLocalizedString(@"Magic Bow", "Weapon Type");
        break;            
    case WEAP_MAGICSWORD:
        weaponString = NSLocalizedString(@"Magic Sword", "Weapon Type");
        break;            
    case WEAP_MAGICWAND:
        weaponString = NSLocalizedString(@"Magic Wand", "Weapon Type");
        break;            
    case WEAP_MYSTICSWORD:
        weaponString = NSLocalizedString(@"Mystic Sword", "Weapon Type");
        break;
    default:
        break;
    }
    return  reinterpret_cast<CFStringRef>(weaponString);
}

CFStringRef armorAsString(ArmorType armor) {
    NSString *armorString = nil;
    switch (armor) {
    case ARMR_NONE:
        armorString = NSLocalizedString(@"None", "Armor Type");
        break;
    case ARMR_CLOTH:
        armorString = NSLocalizedString(@"Cloth", "Armor Type");
        break;
    case ARMR_LEATHER:
        armorString = NSLocalizedString(@"Leather", "Armor Type");
        break;
    case ARMR_CHAIN:
        armorString = NSLocalizedString(@"Chain", "Armor Type");
        break;
    case ARMR_PLATE:
        armorString = NSLocalizedString(@"Plate", "Armor Type");
        break;
    case ARMR_MAGICCHAIN:
        armorString = NSLocalizedString(@"Magic Chain", "Armor Type");
        break;
    case ARMR_MAGICPLATE:
        armorString = NSLocalizedString(@"Magic Plate", "Armor Type");
        break;
    case ARMR_MYSTICROBES:
        armorString = NSLocalizedString(@"Mystic Robes", "Armor Type");
        break;
    default:
        break;
    }
    return  reinterpret_cast<CFStringRef>(armorString);
}
    
CFStringRef reagentAsString(Reagent reagent) {
    NSString *reagentString = nil;
    switch (reagent) {
    default:
        break;
    case REAG_ASH:
        reagentString = NSLocalizedString(@"Sulphorous Ash", @"Reagent Type");
        break;
    case REAG_GINSENG:
        reagentString = NSLocalizedString(@"Ginseng", @"Reagent Type");
        break;
    case REAG_GARLIC:
        reagentString = NSLocalizedString(@"Garlic", @"Reagent Type");
        break;
    case REAG_SILK:
        reagentString = NSLocalizedString(@"Spider Silk", @"Reagent Type");
        break;
    case REAG_MOSS:
        reagentString = NSLocalizedString(@"Blood Moss", @"Reagent Type");
        break;
    case REAG_PEARL:
        reagentString = NSLocalizedString(@"Black Pearl", @"Reagent Type");
        break;
    case REAG_NIGHTSHADE:
        reagentString = NSLocalizedString(@"Nightshade", @"Reagent Type");
        break;
    case REAG_MANDRAKE:
        reagentString = NSLocalizedString(@"Mandrake Root", @"Reagent Type");
        break;
    }
    return  reinterpret_cast<CFStringRef>(reagentString);
}
    
void HIViewDrawCGImage(CGContextRef inContext, const CGRect *inBounds, CGImageRef inImage)
{
    CGContextGStateSaver saver(inContext);
    CGContextTranslateCTM (inContext, 0, inBounds->origin.y + CGRectGetMaxY(*inBounds));
    CGContextScaleCTM(inContext, 1, -1);
    
    CGContextDrawImage(inContext, *inBounds, inImage);
}

CGColorSpaceRef genericColorSpace = 0;
CGColorSpaceRef u4colorSpace() {
    if (genericColorSpace == 0) {
        genericColorSpace = CGColorSpaceCreateDeviceRGB();
    }
    return genericColorSpace;
}
    
void syncPartyMembersWithSaveGame() {
    c->party->syncMembers();
    c->party->notifyOfChange(0);
}

}

