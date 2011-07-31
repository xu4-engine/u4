//
//  U4PlayerTableController.h
//  xu4


#import <UIKit/UIKit.h>

class SaveGamePlayerRecord;
@class U4PlayerCell;
@class PartyStatusImageView;
@interface U4PlayerTableController : UIViewController<UITableViewDataSource, UITableViewDelegate> {
@private
    NSArray *playerArray;
    U4PlayerCell *tableCell;
    UITableView *tableView;
    UILabel *foodLabel;
    UILabel *goldLabel;
    UILabel *goldShipLabel;
    PartyStatusImageView *partyStatusImage;
    NSUInteger moveCount;
}
@property (nonatomic, retain) IBOutlet U4PlayerCell *tableCell;
@property (nonatomic, retain) IBOutlet UITableView *tableView;
@property (nonatomic, retain) IBOutlet UILabel *foodLabel;
@property (nonatomic, retain) IBOutlet UILabel *goldLabel;
@property (nonatomic, retain) IBOutlet UILabel *goldShipLabel;
@property (nonatomic, retain) IBOutlet PartyStatusImageView *partyStatusImage;
- (void)loadPartyDataFromSave;
- (void)updateOtherPartyStats;
- (void)updatePartyMemberData:(const SaveGamePlayerRecord *)partyMember;
- (void)reloadPartyMembers;
- (void)updateActivePartyMember:(NSInteger)row;
- (void)finishPartyOrderEditing;
- (UIButton *)orderButton;

@end
