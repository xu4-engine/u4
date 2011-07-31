//
//  U4ArmorCell.mm
//  xu4


#import "U4ArmorCell.h"

@implementation U4ArmorCell
@synthesize amount;
@synthesize armorName;

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
    [armorName release];
    [amount release];
    [super dealloc];
}
@end
