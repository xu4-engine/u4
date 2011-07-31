//
//  U4WeaponCell.mm
//  xu4
//


#import "U4WeaponCell.h"

@implementation U4WeaponCell
@synthesize weaponName;
@synthesize amount;

- (id)initWithStyle:(UITableViewCellStyle)style reuseIdentifier:(NSString *)reuseIdentifier
{
    self = [super initWithStyle:style reuseIdentifier:reuseIdentifier];
    if (self) {
        // Initialization code
    }
    return self;
}

- (void)setSelected:(BOOL)selected animated:(BOOL)animated
{
    [super setSelected:selected animated:animated];

    // Configure the view for the selected state
}

- (void)dealloc {
    [weaponName release];
    [amount release];
    [super dealloc];
}
@end
