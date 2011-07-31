//
//  U4ArmorChoiceDialog.mm
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

#import "U4ArmorChoiceDialog.h"
#import "U4PartyArmor.h"
#import "U4ArmorCell.h"
#import "U4PlayerCharacter.h"
#include "context.h"
#include "event.h"
#include "ios_helpers.h"
#include "armor.h"

@implementation U4ArmorChoiceDialog(privateStuff)

- (void)loadArmorData
{
    NSMutableArray *tmpArray = [NSMutableArray arrayWithCapacity:ARMR_MAX];
    const SaveGame * const saveGame = c->saveGame;

    // We always have "No Armor."
    [tmpArray addObject:[[[U4PartyArmor alloc] initWithArmor:Armor::get(ArmorType(0))
                                                      amount:0] autorelease]];

    // Now start at one.
    for (int i = 1; i < ARMR_MAX; ++i) {
        short armorCount = saveGame->armor[i];
        if (armorCount != 0) {
            [tmpArray addObject:[[[U4PartyArmor alloc] initWithArmor:Armor::get(ArmorType(i))
                                                              amount:armorCount] autorelease]];
        }
    }
    partyArmor = tmpArray;
    [partyArmor retain];
}

- (void)updateCell:(U4ArmorCell *)armorCell withRecord:(U4PartyArmor *)armor
{
    armorCell.armorName.text = [NSString stringWithUTF8String:[armor armor]->getName().c_str()];
    if (armor.amount > 0)
        armorCell.amount.text = [NSString stringWithFormat:@"%d", armor.amount];
    else
        armorCell.amount.text = @"";

    UIColor *cellTextColor;
    if ([armor armor]->canWear([partyMember playerRecordSheet]->klass))
        cellTextColor = [UIColor darkTextColor];
    else
        cellTextColor = [UIColor darkGrayColor];

    armorCell.armorName.textColor = cellTextColor;
    armorCell.amount.textColor = cellTextColor;
}

@end


@implementation U4ArmorChoiceDialog
@synthesize tableCell, delegate;

 // The designated initializer.  Override if you create the controller programmatically and want to perform customization that is not appropriate for viewDidLoad.
- (id)initWithPartyMember:(U4PlayerCharacter *)character nibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil {
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        partyMember = character;
        [partyMember retain];
        [self loadArmorData];
    }
    return self;
}

// Implement viewDidLoad to do additional setup after loading the view, typically from a nib.
- (void)viewDidLoad {
    self.title = [NSString stringWithFormat:NSLocalizedString(@"Wearing %@", "Party member's current armor"),
                  reinterpret_cast<const NSString *>(U4IOS::armorAsString([partyMember playerRecordSheet]->armor))];
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
    // Release any retained subviews of the main view.
}

- (void)dealloc {
    [partyArmor release];
    [partyMember release];
    [super dealloc];
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
    [delegate didChooseArmor:[partyArmor objectAtIndex:indexPath.row]];
}

- (UITableViewCellEditingStyle)tableView:(UITableView *)tableView editingStyleForRowAtIndexPath:(NSIndexPath *)indexPath {
    return UITableViewCellEditingStyleNone;
}

#pragma mark - UITableViewSource
- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    return [partyArmor count];
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView { 
    return 1;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    static NSString *U4ArmorCellID = @"U4ArmorCell";
    U4ArmorCell *armorCell = static_cast<U4ArmorCell *>([self.tableView dequeueReusableCellWithIdentifier:U4ArmorCellID]);
    
    if (armorCell == nil) {
        [[NSBundle mainBundle] loadNibNamed:U4ArmorCellID owner:self options:nil];
        self.tableCell.selectionStyle = UITableViewCellSelectionStyleGray;
        armorCell = [[self.tableCell retain] autorelease];
        self.tableCell = nil;
    }
    [self updateCell:armorCell withRecord:[partyArmor objectAtIndex:indexPath.row]];
    return armorCell;
}

- (void)reloadArmorData
{
    [partyMember release];
    [partyArmor release];
    [self loadArmorData];
    [self.tableView reloadData];
}


@end
