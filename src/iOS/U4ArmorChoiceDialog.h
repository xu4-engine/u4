//
//  U4ArmorChoiceDialog.h
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

@class U4PartyArmor;
@class U4ArmorCell;
@class U4PlayerCharacter;

@protocol U4ArmorChoiceDialogDelegate
- (void)didChooseArmor:(U4PartyArmor *)armor;
@end

@interface U4ArmorChoiceDialog : UITableViewController {
@private
    NSArray *partyArmor;
    U4ArmorCell *tableCell;
    U4PlayerCharacter *partyMember;
    id<U4ArmorChoiceDialogDelegate> delegate;
}
@property (nonatomic, retain) IBOutlet U4ArmorCell *tableCell;
@property(nonatomic, assign) id<U4ArmorChoiceDialogDelegate> delegate;
- (id)initWithPartyMember:(U4PlayerCharacter *)character nibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil;
- (void)reloadArmorData;
- (void)clearSelection;
@end
