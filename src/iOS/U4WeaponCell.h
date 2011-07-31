//
//  U4WeaponCell.h
//  xu4
//


#import <UIKit/UIKit.h>

@interface U4WeaponCell : UITableViewCell {
    UILabel *weaponName;
    UILabel *amount;
}

@property (nonatomic, retain) IBOutlet UILabel *weaponName;
@property (nonatomic, retain) IBOutlet UILabel *amount;

@end
