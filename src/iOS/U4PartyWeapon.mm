//
//  U4Weapon.mm
//  xu4


#import "U4PartyWeapon.h"
#include "weapon.h"

@implementation U4PartyWeapon
@synthesize amount;

- (id)initWithWeapon:(const Weapon *)weaponRecord amount:(NSInteger)partyAmount;
{
    self = [super init];
    if (self) {
        weapon = weaponRecord;
        amount = partyAmount;
    }
    
    return self;
}

- (const Weapon *)weapon {
    return weapon;
}

@end
