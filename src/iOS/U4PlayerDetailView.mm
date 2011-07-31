//
//  U4PlayerDetailView.mm
//  xu4
//


#import "U4PlayerDetailView.h"
#import "ios_helpers.h"
#import "U4PlayerCharacter.h"
#import "U4ArmorChoiceDialog.h"
#import "U4WeaponChoiceDialog.h"
#import "U4PartyArmor.h"
#import "U4PartyWeapon.h"
#include "armor.h"
#include "game.h"
#include "weapon.h"
#include "player.h"
#include "context.h"
#include "spell.h"

@implementation U4PlayerDetailView
@synthesize genderLabel;
@synthesize statusLabel;
@synthesize characterClassLabel;
@synthesize levelLabel;
@synthesize strengthLabel;
@synthesize dexterityLabel;
@synthesize intelligenceLabel;
@synthesize magicPointsLabel;
@synthesize hitPointsLabel;
@synthesize maxHitPointsLabel;
@synthesize experienceLabel;
@synthesize weaponButton;
@synthesize armorButton;
@synthesize scrollView;
@synthesize attributesView;
@synthesize weaponsList;
@synthesize armorList;
@synthesize reagentsList;
@synthesize spellList;
@synthesize itemsList;
@synthesize equipmentList;
@synthesize weaponView;
@synthesize armorView;
@synthesize reagentsView;
@synthesize spellView;
@synthesize equipmentView;
@synthesize itemsView;


- (void)resizeLabel:(UILabel *)label forText:(NSString *)text
{
    const CGSize ContraintSize = CGSizeMake(label.frame.size.width, label.superview.frame.size.height - 20);
    CGSize textSize = [text sizeWithFont:label.font constrainedToSize:ContraintSize lineBreakMode:label.lineBreakMode];
    CGRect frame = label.frame;
    frame.size = textSize;
    label.frame = frame;
}

- (void)setupWeapons {
    NSMutableString *string;
    string = [NSMutableString stringWithCapacity:256];
    for (int i = 0; i < WEAP_MAX; ++i) {
        int weaponCount = c->saveGame->weapons[i];
        if (weaponCount > 0)
            [string appendFormat:@"%2d %@\n", weaponCount, U4IOS::weaponAsString(WeaponType(i))];
    }
    weaponsList.text = string;
    [self resizeLabel:weaponsList forText:string];
}

- (void)setupArmor {
    NSMutableString *string;
    string = [NSMutableString stringWithCapacity:256];
    for (int i = 0; i < ARMR_MAX; ++i) {
        int armorCount = c->saveGame->armor[i];
        if (armorCount > 0)
            [string appendFormat:@"%2d %@\n", armorCount, U4IOS::armorAsString(ArmorType(i))];
    }
    armorList.text = string;
    [self resizeLabel:armorList forText:string];
}

- (void)setupItems {
    NSMutableString *string = [NSMutableString stringWithCapacity:256];
    if (c->saveGame->stones != 0) {
        [string appendString:@"Stones:\n"];
        for (int i = 0; i < VIRT_MAX; i++) {
            if (c->saveGame->stones & (1 << i)) {
                [string appendString:[NSString stringWithUTF8String:getStoneName((Virtue) i)]];
                [string appendString:@" "];
            }
        }
    }
    
    if ([string length] > 0)
        [string appendString:@"\n"];
    
    if (c->saveGame->runes != 0) {
        [string appendString:@"Runes:\n"];
        for (int i = 0; i < VIRT_MAX; i++) {
            if (c->saveGame->runes & (1 << i)) {
                [string appendString:[NSString stringWithUTF8String:getVirtueName((Virtue) i)]];
                [string appendString:@" "];
            }           
        }
    }

    if (c->saveGame->items & (ITEM_CANDLE | ITEM_BOOK | ITEM_BELL)) {
        if ([string length] > 0)
            [string appendString:@"\n"];
        
        if (c->saveGame->items & ITEM_BELL) {
            [string appendFormat:@"%@ ", [NSString stringWithUTF8String:getItemName(ITEM_BELL)]];
        }
        if (c->saveGame->items & ITEM_BOOK) {
            [string appendFormat:@"%@ ", [NSString stringWithUTF8String:getItemName(ITEM_BOOK)]];
        }
        if (c->saveGame->items & ITEM_CANDLE) {
            [string appendFormat:@"%@", [NSString stringWithUTF8String:getItemName(ITEM_CANDLE)]];
        }
    }

    if (c->saveGame->items & (ITEM_KEY_C | ITEM_KEY_L | ITEM_KEY_T)) {
        [string appendString:@"\nKey Parts: "];
        if (c->saveGame->items & ITEM_KEY_T)
            [string appendFormat:@"%@ ", [NSString stringWithUTF8String:getItemName(ITEM_KEY_T)]];
        if (c->saveGame->items & ITEM_KEY_L)
            [string appendFormat:@"%@ ", [NSString stringWithUTF8String:getItemName(ITEM_KEY_L)]];
        if (c->saveGame->items & ITEM_KEY_C)
            [string appendFormat:@"%@\n", [NSString stringWithUTF8String:getItemName(ITEM_KEY_C)]];
    }
    [string appendString:@"\n"];
    if (c->saveGame->items & ITEM_HORN)
        [string appendFormat:@"%@ ", [NSString stringWithUTF8String:getItemName(ITEM_HORN)]];
    if (c->saveGame->items & ITEM_WHEEL)
        [string appendFormat:@"%@ ", [NSString stringWithUTF8String:getItemName(ITEM_WHEEL)]];
    if (c->saveGame->items & ITEM_SKULL)
        [string appendFormat:@"%@", [NSString stringWithUTF8String:getItemName(ITEM_SKULL)]];

    itemsList.text = string;
    [self resizeLabel:itemsList forText:string];
}

- (void)setupOtherLists {
    // Weapon
    [self setupWeapons];
    // Armor
    [self setupArmor];
    NSMutableString *string;

    // Equipment
    string = [NSMutableString stringWithCapacity:256];
    if (c->saveGame->torches > 0)
        [string appendFormat:@"%2d %@\n", c->saveGame->torches, NSLocalizedString(@"Torches", "Equipment")];
    if (c->saveGame->gems > 0)
        [string appendFormat:@"%2d %@\n", c->saveGame->gems, NSLocalizedString(@"Gems", "Equipment")];
    if (c->saveGame->keys > 0)
    [string appendFormat:@"%2d %@\n", c->saveGame->keys, NSLocalizedString(@"Keys", "Equipment")];
    if (c->saveGame->sextants > 0)
        [string appendFormat:@"%2d %@\n", c->saveGame->sextants, NSLocalizedString(@"Sextants", "Equipment")];
    equipmentList.text = string;
    [self resizeLabel:equipmentList forText:string];

    // Items
    [self setupItems];

    // Reagents
    string = [NSMutableString stringWithCapacity:256];
    for (int i = 0; i < REAG_MAX; ++i) {
        int reagentCount = c->saveGame->reagents[i];
        if (reagentCount > 0)
            [string appendFormat:@"%2d %@\n", reagentCount, U4IOS::reagentAsString(Reagent(i))];
    }
    reagentsList.text = string;
    [self resizeLabel:reagentsList forText:string];

    // Spells
    string = [NSMutableString stringWithCapacity:256];
    for (int i = 0; i < SPELL_MAX; ++i) {
        int spellCount = c->saveGame->mixtures[i];
        if (spellCount > 0) {
            const Spell *spell = getSpell(i);
            [string appendFormat:@"%2d %@\n", spellCount, [NSString stringWithUTF8String:spell->name]];
        }
    }
    spellList.text = string;
    [self resizeLabel:spellList forText:string];
}

- (id)initWithPlayerRecord:(U4PlayerCharacter *)newPlayer forRow:(NSInteger)row nibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil;
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        player = newPlayer;
        rowInTable = row;
        [player retain];
    }
    return self;
}

- (void)dealloc
{
    [player release];
    [genderLabel release];
    [statusLabel release];
    [characterClassLabel release];
    [levelLabel release];
    [strengthLabel release];
    [dexterityLabel release];
    [intelligenceLabel release];
    [magicPointsLabel release];
    [hitPointsLabel release];
    [maxHitPointsLabel release];
    [experienceLabel release];
    [weaponButton release];
    [armorButton release];
    [scrollView release];
    [attributesView release];
    [weaponsList release];
    [armorList release];
    [reagentsList release];
    [spellList release];
    [itemsList release];
    [equipmentList release];
    [weaponView release];
    [armorView release];
    [reagentsView release];
    [spellView release];
    [equipmentView release];
    [itemsView release];
    [super dealloc];
}

- (void)didReceiveMemoryWarning
{
    // Releases the view if it doesn't have a superview.
    [super didReceiveMemoryWarning];
    
    // Release any cached data, images, etc that aren't in use.
}

#pragma mark - View lifecycle

- (void)viewDidLoad
{
    [super viewDidLoad];
    [scrollView addSubview:attributesView];
    scrollView.indicatorStyle = UIScrollViewIndicatorStyleWhite;
    // This order is a bit different than what was in the original game, but this flows better for
    // the maximum number of spells.
    NSArray *views = [NSArray arrayWithObjects:attributesView, itemsView, equipmentView, weaponView,
                                               armorView, reagentsView, spellView, nil];
    int i = 0;
    CGFloat totalHeight = 0;
    for (UIView *view in views) {
        CGFloat y = i * self.view.frame.size.height;
        if (view != spellView)
            view.frame = CGRectMake(0, y, self.view.frame.size.width, self.view.frame.size.height);
        else
            view.frame = CGRectMake(0, y, view.frame.size.width, view.frame.size.height);
        [scrollView addSubview:view];
        totalHeight += view.frame.size.height;
        ++i;
    }
    [self setupOtherLists];
    scrollView.contentSize = CGSizeMake(self.view.frame.size.width, totalHeight);
    const SaveGamePlayerRecord *playerRecord = [player playerRecordSheet];
    self.title = [NSString stringWithUTF8String:playerRecord->name];
    genderLabel.text = playerRecord->sex == SEX_MALE ? NSLocalizedString(@"Male", "Gender") : NSLocalizedString(@"Female", "Gender");
    statusLabel.text = const_cast<NSString *>(reinterpret_cast<const NSString *>(U4IOS::playerStatusAsString(playerRecord->status)));
    characterClassLabel.text = const_cast<NSString *>(reinterpret_cast<const NSString *>(U4IOS::playerClassAsString(playerRecord->klass)));
    levelLabel.text = [NSString stringWithFormat:@"%d", playerRecord->hpMax / 100];
    strengthLabel.text = [NSString stringWithFormat:@"%d", playerRecord->str];
    dexterityLabel.text = [NSString stringWithFormat:@"%d", playerRecord->dex];
    intelligenceLabel.text = [NSString stringWithFormat:@"%d", playerRecord->intel];
    magicPointsLabel.text = [NSString stringWithFormat:@"%d", playerRecord->mp];
    hitPointsLabel.text = [NSString stringWithFormat:@"%d", playerRecord->hp];
    maxHitPointsLabel.text = [NSString stringWithFormat:@"%d", playerRecord->hpMax];
    experienceLabel.text = [NSString stringWithFormat:@"%d", playerRecord->xp];
    [weaponButton setTitle:const_cast<NSString *>(reinterpret_cast<const NSString *>(U4IOS::weaponAsString(playerRecord->weapon))) forState:UIControlStateNormal];
    [armorButton setTitle:const_cast<NSString *>(reinterpret_cast<const NSString *>(U4IOS::armorAsString(playerRecord->armor))) forState:UIControlStateNormal];
    armorButton.enabled = !(c->location->context & CTX_COMBAT);
}

- (void)viewDidUnload
{
    [self setStatusLabel:nil];
    [self setCharacterClassLabel:nil];
    [self setLevelLabel:nil];
    [self setStrengthLabel:nil];
    [self setDexterityLabel:nil];
    [self setIntelligenceLabel:nil];
    [self setMagicPointsLabel:nil];
    [self setHitPointsLabel:nil];
    [self setMaxHitPointsLabel:nil];
    [self setExperienceLabel:nil];
    [self setWeaponButton:nil];
    [self setArmorButton:nil];
    [self setScrollView:nil];
    [self setAttributesView:nil];
    [self setWeaponsList:nil];
    [self setArmorList:nil];
    [self setReagentsList:nil];
    [self setSpellList:nil];
    [self setItemsList:nil];
    [self setEquipmentList:nil];
    [self setWeaponView:nil];
    [self setArmorView:nil];
    [self setReagentsView:nil];
    [self setSpellView:nil];
    [self setEquipmentView:nil];
    [self setItemsView:nil];
    [super viewDidUnload];
    // Release any retained subviews of the main view.
    // e.g. self.myOutlet = nil;
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
    // Return YES for supported orientations
	return YES;
}

- (IBAction)weaponPressed:(id)sender {
    weaponPanel = [[U4WeaponChoiceDialog alloc] initWithPartyMember:player nibName:@"U4WeaponChoiceDialog" bundle:nil];
    weaponPanel.delegate = self;
    [self.navigationController pushViewController:weaponPanel animated:YES];
}

- (IBAction)armorPressed:(id)sender {
    armorPanel = [[U4ArmorChoiceDialog alloc] initWithPartyMember:player nibName:@"U4ArmorChoiceDialog" bundle:nil];
    armorPanel.delegate = self;
    [self.navigationController pushViewController:armorPanel animated:YES];
}

- (void)didChooseWeapon:(U4PartyWeapon *)weapon {
    if ([weapon weapon]->canReady([player playerRecordSheet]->klass)) {
        PartyMember *pm = c->party->member(rowInTable);
        pm->setWeapon([weapon weapon]);
        [self.navigationController popViewControllerAnimated:YES];
        [weaponButton setTitle:const_cast<NSString *>(reinterpret_cast<const NSString *>(U4IOS::weaponAsString([player playerRecordSheet]->weapon))) forState:UIControlStateNormal];
        [weaponPanel release];
        [self setupWeapons];
        c->location->turnCompleter->finishTurn();
    } else {
        [weaponPanel clearSelection];
    }
}

- (void)didChooseArmor:(U4PartyArmor *)armor {
    if ([armor armor]->canWear([player playerRecordSheet]->klass)) {
        PartyMember *pm = c->party->member(rowInTable);
        pm->setArmor([armor armor]);
        [self.navigationController popViewControllerAnimated:YES];
        [armorButton setTitle:const_cast<NSString *>(reinterpret_cast<const NSString *>(U4IOS::armorAsString([player playerRecordSheet]->armor))) forState:UIControlStateNormal];
        [armorPanel release];
        [self setupArmor];
        c->location->turnCompleter->finishTurn();
    } else {
        [armorPanel clearSelection];
    }
}

@end
