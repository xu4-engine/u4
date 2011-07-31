//
//  U4ArmorCell.h
//  xu4


#import <UIKit/UIKit.h>

@interface U4ArmorCell : UITableViewCell {
    UILabel *armorName;
    UILabel *amount;
}

@property (nonatomic, retain) IBOutlet UILabel *amount;
@property (nonatomic, retain) IBOutlet UILabel *armorName;
@end
