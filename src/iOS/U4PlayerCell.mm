//
//  U4PlayerCell.mm
//  xu4


#import "U4PlayerCell.h"


@implementation U4PlayerCell
@synthesize characterName;
@synthesize characterHP;
@synthesize characterStatus;
@synthesize orderButton;

- (id)initWithStyle:(UITableViewCellStyle)style reuseIdentifier:(NSString *)reuseIdentifier
{
    NSLog(@"Identifier is %@", reuseIdentifier);
    self = [super initWithStyle:style reuseIdentifier:reuseIdentifier];
    if (self) {
        
    }
    return self;
}

- (void)setSelected:(BOOL)selected animated:(BOOL)animated
{
    [super setSelected:selected animated:animated];

    // Configure the view for the selected state
}

- (void)dealloc
{
    [characterName release];
    [characterHP release];
    [characterStatus release];
    [orderButton release];
    [super dealloc];
}

@end
