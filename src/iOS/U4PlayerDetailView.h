//
//  U4PlayerDetailView.h
//  xu4
//


#import <UIKit/UIKit.h>
#import "U4ArmorChoiceDialog.h"
#import "U4WeaponChoiceDialog.h"

@class U4PlayerCharacter;

@interface U4PlayerDetailView : UIViewController<U4WeaponChoiceDialogDelegate, U4ArmorChoiceDialogDelegate> {
@private    
    UILabel *genderLabel;
    UILabel *statusLabel;
    UILabel *characterClassLabel;
    UILabel *levelLabel;
    UILabel *strengthLabel;
    UILabel *dexterityLabel;
    UILabel *intelligenceLabel;
    UILabel *magicPointsLabel;
    UILabel *hitPointsLabel;
    UILabel *maxHitPointsLabel;
    UILabel *experienceLabel;
    UIButton *weaponButton;
    UIButton *armorButton;
    UIScrollView *scrollView;
    UIView *attributesView;
    UILabel *weaponsList;
    UILabel *armorList;
    UILabel *reagentsList;
    UILabel *spellList;
    UILabel *itemsList;
    UILabel *equipmentList;
    UIView *weaponView;
    UIView *armorView;
    UIView *reagentsView;
    UIView *spellView;
    UIView *equipmentView;
    UIView *itemsView;
    U4PlayerCharacter *player;
    NSInteger rowInTable;
    U4ArmorChoiceDialog *armorPanel;
    U4WeaponChoiceDialog *weaponPanel;
}
@property (nonatomic, retain) IBOutlet UILabel *genderLabel;
@property (nonatomic, retain) IBOutlet UILabel *statusLabel;
@property (nonatomic, retain) IBOutlet UILabel *characterClassLabel;
@property (nonatomic, retain) IBOutlet UILabel *levelLabel;
@property (nonatomic, retain) IBOutlet UILabel *strengthLabel;
@property (nonatomic, retain) IBOutlet UILabel *dexterityLabel;
@property (nonatomic, retain) IBOutlet UILabel *intelligenceLabel;
@property (nonatomic, retain) IBOutlet UILabel *magicPointsLabel;
@property (nonatomic, retain) IBOutlet UILabel *hitPointsLabel;
@property (nonatomic, retain) IBOutlet UILabel *maxHitPointsLabel;
@property (nonatomic, retain) IBOutlet UILabel *experienceLabel;
@property (nonatomic, retain) IBOutlet UIButton *weaponButton;
@property (nonatomic, retain) IBOutlet UIButton *armorButton;
@property (nonatomic, retain) IBOutlet UIScrollView *scrollView;
@property (nonatomic, retain) IBOutlet UIView *attributesView;
@property (nonatomic, retain) IBOutlet UILabel *weaponsList;
@property (nonatomic, retain) IBOutlet UILabel *armorList;
@property (nonatomic, retain) IBOutlet UILabel *reagentsList;
@property (nonatomic, retain) IBOutlet UILabel *spellList;
@property (nonatomic, retain) IBOutlet UILabel *itemsList;
@property (nonatomic, retain) IBOutlet UILabel *equipmentList;
@property (nonatomic, retain) IBOutlet UIView *weaponView;
@property (nonatomic, retain) IBOutlet UIView *armorView;
@property (nonatomic, retain) IBOutlet UIView *reagentsView;
@property (nonatomic, retain) IBOutlet UIView *spellView;
@property (nonatomic, retain) IBOutlet UIView *equipmentView;
@property (nonatomic, retain) IBOutlet UIView *itemsView;

- (IBAction)weaponPressed:(id)sender;
- (IBAction)armorPressed:(id)sender;
- (void)setupOtherLists;

- (id)initWithPlayerRecord:(U4PlayerCharacter *)player forRow:(NSInteger)row nibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil;
@end
