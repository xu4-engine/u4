//
//  U4Armor.h
//  xu4
//


#import <Foundation/Foundation.h>

struct Armor;

@interface U4PartyArmor : NSObject {
@private
    const Armor *armor;
    NSInteger amount;
}
- (id)initWithArmor:(const Armor *)armorRecord amount:(NSInteger)partyAmount;
- (const Armor *)armor;
@property(nonatomic,readonly) NSInteger amount;
@end
