//
//  U4PlayerCell.h
//  xu4


#import <UIKit/UIKit.h>

@class U4Button;
@interface U4PlayerCell : UITableViewCell {
    
    UILabel *characterName;
    UILabel *characterHP;
    UILabel *characterStatus;
    U4Button *orderButton;
}
@property (nonatomic, retain) IBOutlet UILabel *characterName;
@property (nonatomic, retain) IBOutlet UILabel *characterHP;
@property (nonatomic, retain) IBOutlet UILabel *characterStatus;
@property (nonatomic, retain) IBOutlet U4Button *orderButton;

@end
