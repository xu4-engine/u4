//
//  U4Weapon.h
//  xu4
//


#import <Foundation/Foundation.h>

struct Weapon;

@interface U4PartyWeapon : NSObject {
@private
    const Weapon *weapon;
    NSInteger amount;
}
- (id)initWithWeapon:(const Weapon *)weaponRecord amount:(NSInteger)partyAmount;
- (const Weapon *)weapon;
@property(readonly,nonatomic) NSInteger amount;

@end
