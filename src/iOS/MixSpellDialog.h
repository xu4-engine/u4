//
//  MixSpell.h
//  xu4
//
// Copyright 2011 Trenton Schulz. All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
// 
//    1. Redistributions of source code must retain the above copyright notice, this list of
//       conditions and the following disclaimer.
// 
//    2. Redistributions in binary form must reproduce the above copyright notice, this list
//       of conditions and the following disclaimer in the documentation and/or other materials
//       provided with the distribution.
// 
// THIS SOFTWARE IS PROVIDED BY TRENTON SCHULZ ``AS IS'' AND ANY EXPRESS OR IMPLIED
// WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
// FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
// ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// 
// The views and conclusions contained in the software and documentation are those of the
// authors and should not be interpreted as representing official policies, either expressed
// or implied, of Trenton Schulz.
//

#import <UIKit/UIKit.h>
#import "SpellInfoPickerLabel.h"
#import "SpellReagentTableCell.h"

@class MixSpellDialog;
class Ingredients;
@protocol MixSpellDialogDelegate

-(void)mixSpellDialog:(MixSpellDialog *)dialog chooseSpell:(NSInteger)spellIndex reagents:(Ingredients *)reagents numberOfSpells:(NSInteger)amount;
-(void)mixWasCanceled:(MixSpellDialog *)dialog;

@end



@interface MixSpellDialog : UIViewController<UITableViewDataSource, UITableViewDelegate, UIPickerViewDataSource, UIPickerViewDelegate> {
    id<MixSpellDialogDelegate> delegate;
    UIPickerView *spellPicker;
    UITableView *componentView;
    UITextView *spellDescription;
    UISlider *spellSlider;
    UILabel *totalSpells;
    UINavigationBar *navBar;
    SpellReagentTableCell *tableCell;
    SpellInfoPickerLabel *pickerLabel;
    NSArray *allSpells;
    NSArray *allComponents;
}
@property (nonatomic, retain) id<MixSpellDialogDelegate> delegate;
@property (nonatomic, retain) IBOutlet UIPickerView *spellPicker;
@property (nonatomic, retain) IBOutlet UITableView *componentView;
@property (nonatomic, retain) IBOutlet UITextView *spellDescription;
@property (nonatomic, retain) IBOutlet UISlider *spellSlider;
@property (nonatomic, retain) IBOutlet UILabel *totalSpells;
@property (nonatomic, retain) IBOutlet UINavigationBar *navBar;
@property (nonatomic, retain) IBOutlet SpellReagentTableCell *tableCell;
@property (nonatomic, retain) IBOutlet SpellInfoPickerLabel *pickerLabel;
- (IBAction)spellCountChanged:(id)sender;
- (void)cancelMix:(id)sender;
- (void)mixSpell:(id)sender;

@end
