//
//  U4WeaponChoiceDialog.mm
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

#import "U4WeaponChoiceDialog.h"
#import "U4PartyWeapon.h"
#import "U4WeaponCell.h"
#import "U4PlayerCharacter.h"
#include "context.h"
#include "event.h"
#include "ios_helpers.h"
#include "weapon.h"

@implementation U4WeaponChoiceDialog(privateStuff)

- (void)loadData
{
    NSMutableArray *tmpArray = [NSMutableArray arrayWithCapacity:WEAP_MAX];
    const SaveGame * const savegame = c->saveGame;

    // We always have our hands
    [tmpArray addObject:[[[U4PartyWeapon alloc] initWithWeapon:Weapon::get(WeaponType(0))
                                                        amount:-1] autorelease]];
    // Now start at one.
    for (int i = 1; i < WEAP_MAX; ++i) {
        short weaponCount = savegame->weapons[i];
        if (weaponCount > 0) {
            [tmpArray addObject:[[[U4PartyWeapon alloc] initWithWeapon:Weapon::get(WeaponType(i))
                                                                amount:weaponCount] autorelease]];
        }
    }
    partyWeapons = tmpArray;
    [partyWeapons retain];
}

- (void)updateCell:(U4WeaponCell *)weaponCell withRecord:(U4PartyWeapon *)weapon;
{
    weaponCell.weaponName.text = [NSString stringWithUTF8String:[weapon weapon]->getName().c_str()];
    if (weapon.amount > 0)
        weaponCell.amount.text = [NSString stringWithFormat:@"%d", weapon.amount];
    else
        weaponCell.amount.text = @"";
    
    UIColor *colorCell;
    if ([weapon weapon]->canReady([partyMember playerRecordSheet]->klass)) {
        colorCell = [UIColor darkTextColor];
    } else {
        colorCell = [UIColor darkGrayColor];
    }
    weaponCell.weaponName.textColor = colorCell;
    weaponCell.amount.textColor = colorCell;
}


@end

@implementation U4WeaponChoiceDialog
@synthesize tableCell, delegate;

 // The designated initializer.  Override if you create the controller programmatically and want to perform customization that is not appropriate for viewDidLoad.

- (id)initWithPartyMember:(U4PlayerCharacter *)character nibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil {
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        [self loadData];
        partyMember = character;
        [partyMember retain];
    }
    return self;
}

// Implement viewDidLoad to do additional setup after loading the view, typically from a nib.
- (void)viewDidLoad {
    self.title = [NSString stringWithFormat:NSLocalizedString(@"Using %@", "Party member's current weapon"),
                  reinterpret_cast<const NSString *>(U4IOS::weaponAsString([partyMember playerRecordSheet]->weapon))];

}


- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
    // Overriden to allow any orientation.
    return YES;
}


- (void)didReceiveMemoryWarning {
    // Releases the view if it doesn't have a superview.
    [super didReceiveMemoryWarning];
    
    // Release any cached data, images, etc. that aren't in use.
}


- (void)viewDidUnload {
    [super viewDidUnload];
}


- (void)dealloc {
    [partyWeapons release];
    [partyMember release];
    [super dealloc];
}

- (void)reloadWeaponData
{
    [partyWeapons release];
    [self reloadWeaponData];
    [self.tableView reloadData];
}

- (void)clearSelection
{
    [self.tableView deselectRowAtIndexPath:[self.tableView indexPathForSelectedRow] animated:YES];
}


#pragma mark - Table View Delegate

// Display customization

// Section header & footer information. Views are preferred over title should you decide to provide both

// custom view for header. will be adjusted to default or specified header height
- (UIView *)tableView:(UITableView *)tableView viewForHeaderInSection:(NSInteger)section {
    return nil;
}
// custom view for footer. will be adjusted to default or specified footer height
- (UIView *)tableView:(UITableView *)tableView viewForFooterInSection:(NSInteger)section {
    return nil;
}

// Accessories (disclosures). 
- (void)tableView:(UITableView *)tableView accessoryButtonTappedForRowWithIndexPath:(NSIndexPath *)indexPath {
    ;
}

// Selection

// Called before the user changes the selection. Return a new indexPath, or nil, to change the proposed selection.
- (NSIndexPath *)tableView:(UITableView *)tableView willSelectRowAtIndexPath:(NSIndexPath *)indexPath {
    return indexPath;
}

- (NSIndexPath *)tableView:(UITableView *)tableView willDeselectRowAtIndexPath:(NSIndexPath *)indexPath {
    return indexPath;
}
// Called after the user changes the selection.
- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
    [delegate didChooseWeapon:[partyWeapons objectAtIndex:indexPath.row]];
}

- (UITableViewCellEditingStyle)tableView:(UITableView *)tableView editingStyleForRowAtIndexPath:(NSIndexPath *)indexPath {
    return UITableViewCellEditingStyleNone;
}

#pragma mark - UITableViewSource
- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    return [partyWeapons count];
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView { 
    return 1;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    static NSString *U4WeaponCellID = @"U4WeaponCell";
    U4WeaponCell *armorCell = static_cast<U4WeaponCell *>([self.tableView dequeueReusableCellWithIdentifier:U4WeaponCellID]);
    
    if (armorCell == nil) {
        [[NSBundle mainBundle] loadNibNamed:U4WeaponCellID owner:self options:nil];
        self.tableCell.selectionStyle = UITableViewCellSelectionStyleGray;
        armorCell = [[self.tableCell retain] autorelease];
        self.tableCell = nil;
    }
    [self updateCell:armorCell withRecord:[partyWeapons objectAtIndex:indexPath.row]];
    return armorCell;
}


@end
