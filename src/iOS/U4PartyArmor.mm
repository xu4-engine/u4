//
//  U4Armor.mm
//  xu4
//


#import "U4PartyArmor.h"
#include "armor.h"

@implementation U4PartyArmor
@synthesize amount;

- (id)initWithArmor:(const Armor *)armorRecord amount:(NSInteger)partyAmount
{
    self = [super init];
    if (self) {
        armor = armorRecord;
        amount = partyAmount;
    }
    
    return self;
}

- (const Armor *)armor
{
    return armor;
}

@end
