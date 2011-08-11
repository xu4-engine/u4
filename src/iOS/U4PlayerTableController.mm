//
//  U4PlayerTableController.mm
//  xu4


#import "U4PlayerTableController.h"
#import "U4PlayerCharacter.h"
#import "U4PlayerCell.h"
#import "U4PlayerDetailView.h"
#import "U4Button.h"
#import "ios_helpers.h"
#include "location.h"
#include "context.h"
#include "game.h"

@implementation U4PlayerTableController
@synthesize tableCell;
@synthesize tableView;
@synthesize foodLabel;
@synthesize goldLabel;
@synthesize goldShipLabel;
@synthesize partyStatusImage;

- (void)finishInit {
}

- (void)loadPartyDataFromSave {
    [super viewDidLoad];
    NSMutableArray *tmpArray = [NSMutableArray arrayWithCapacity:8];
    const SaveGame * const saveGame = c->saveGame;
    for (int i = 0; i < saveGame->members; ++i)
        [tmpArray addObject:[[[U4PlayerCharacter alloc] initWithPlayerRecord:&saveGame->players[i]] autorelease]];
    playerArray = tmpArray;
    [playerArray retain];
}

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        [self finishInit];
    }
    return self;
}

- (id)initWithCoder:(NSCoder *)aDecoder {
    self = [super initWithCoder:aDecoder];
    if (self) {
        [self finishInit];
    }
    return  self;
}

- (void)dealloc
{
    [playerArray release];
    [tableView release];
    [foodLabel release];
    [goldLabel release];
    [goldShipLabel release];
    [partyStatusImage release];
    [super dealloc];
}

- (void)didReceiveMemoryWarning
{
    // Releases the view if it doesn't have a superview.
    [super didReceiveMemoryWarning];
    
    // Release any cached data, images, etc that aren't in use.
}

- (void)updateOtherPartyStats
{
    if (foodLabel == nil)
        return;
    foodLabel.text = [NSString stringWithFormat:@"%04d", c->saveGame->food / 100];
    if (c->transportContext == TRANSPORT_SHIP) {
        goldShipLabel.text = NSLocalizedString(@"Ship:", "Label shown for ship hull integretity");
        goldLabel.text = [NSString stringWithFormat:@"%02d", c->saveGame->shiphull];
    } else {
        goldShipLabel.text = NSLocalizedString(@"Gold:", "Label shown for party gold");
        goldLabel.text = [NSString stringWithFormat:@"%04d", c->saveGame->gold];
    }
    
}

- (void)finishPartyOrderEditing
{
    if (tableView.isEditing) { // Yes, a bit redundant, but this function is called on every button press, make it cheap.
        tableView.editing = NO;
        [[self orderButton] setTitle:NSLocalizedString(@"Order", "New Party Order Button") forState:UIControlStateNormal];
        if (moveCount > 0) {
            moveCount = 0;
            c->location->turnCompleter->finishTurn();
        }
    }
}


- (UIButton *)orderButton 
{
    U4PlayerCell *cell = static_cast<U4PlayerCell *>([tableView cellForRowAtIndexPath:[NSIndexPath indexPathForRow:0 inSection:0]]);
    return cell.orderButton;
}


- (void)partyOrderButtonPressed:(id)sender
{
    if (tableView.isEditing) {
        [self finishPartyOrderEditing];
    } else {
        tableView.editing = YES;
        [[self orderButton] setTitle:NSLocalizedString(@"Done", "Cancel New Party Order") forState:UIControlStateNormal];        
    }
}

#pragma mark - View lifecycle

- (void)viewDidLoad
{
    self.tableView.separatorStyle = UITableViewCellSeparatorStyleNone;
	self.tableView.backgroundColor = [UIColor clearColor];
    if (c) {
        [self updateOtherPartyStats];
        U4IOS::IOSObserver::sharedInstance()->update(c->aura);
    }
}

- (void)viewDidUnload
{
    [self setTableView:nil];
    [self setFoodLabel:nil];
    [self setGoldLabel:nil];
    [self setGoldShipLabel:nil];
    [self setPartyStatusImage:nil];
    [super viewDidUnload];
    // Release any retained subviews of the main view.
    // e.g. self.myOutlet = nil;
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
    // Return YES for supported orientations
	return YES;
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
    U4PlayerCharacter *character = [playerArray objectAtIndex:indexPath.row];
    UIScrollView *scrollView = [[UIScrollView alloc] initWithFrame:CGRectMake(0, 0, 350, 400)];
    scrollView.contentSize = CGSizeMake(350, 800);
    U4PlayerDetailView *detailView = [[U4PlayerDetailView alloc] initWithPlayerRecord:character forRow:indexPath.row nibName:@"U4PlayerDetailView" bundle:nil];
    [self.navigationController pushViewController:detailView animated:YES];
    [detailView.scrollView flashScrollIndicators];
    [detailView release];
}

- (void)tableView:(UITableView *)tableView didDeselectRowAtIndexPath:(NSIndexPath *)indexPath {
    ;
}

// Editing

// Allows customization of the editingStyle for a particular cell located at 'indexPath'. If not implemented, all editable cells will have UITableViewCellEditingStyleDelete set for them when the table has editing property set to YES.
- (UITableViewCellEditingStyle)tableView:(UITableView *)tableView editingStyleForRowAtIndexPath:(NSIndexPath *)indexPath {
    return UITableViewCellEditingStyleNone;
}

// The willBegin/didEnd methods are called whenever the 'editing' property is automatically changed by the table (allowing insert/delete/move). This is done by a swipe activating a single row
- (void)tableView:(UITableView*)tableView willBeginEditingRowAtIndexPath:(NSIndexPath *)indexPath {
    ;
}

- (void)tableView:(UITableView*)tableView didEndEditingRowAtIndexPath:(NSIndexPath *)indexPath {
    ;
}

// Moving/reordering

// Allows customization of the target row for a particular row as it is being moved/reordered
- (NSIndexPath *)tableView:(UITableView *)tableView targetIndexPathForMoveFromRowAtIndexPath:(NSIndexPath *)sourceIndexPath toProposedIndexPath:(NSIndexPath *)proposedDestinationIndexPath {
    return proposedDestinationIndexPath;
}

// Indentation
// return 'depth' of row for hierarchies
- (NSInteger)tableView:(UITableView *)tableView indentationLevelForRowAtIndexPath:(NSIndexPath *)indexPath {
    return 0;
}

#pragma mark - UITableViewSource
- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    return [playerArray count];
}

- (void)updateCell:(U4PlayerCell *)cell withRecord:(const SaveGamePlayerRecord *)record {
    cell.characterName.text = [NSString stringWithUTF8String:record->name];
    cell.characterHP.text = [NSString stringWithFormat:@"%d", record->hp];
    cell.characterStatus.text = const_cast<NSString *>(reinterpret_cast<const NSString *>(U4IOS::playerStatusAsString(record->status)));
}

- (void)updatePartyMemberData:(const SaveGamePlayerRecord *)partyMember {
    NSInteger row = 0;
    for (U4PlayerCharacter *character in playerArray) {
        if (partyMember == [character playerRecordSheet]) {
            break;
        }
        ++row;
    }
    
    U4PlayerCell *cell = static_cast<U4PlayerCell *>([self.tableView cellForRowAtIndexPath:[NSIndexPath indexPathForRow:row inSection:0]]);
    [self updateCell:cell withRecord:partyMember];
}

// Row display. Implementers should *always* try to reuse cells by setting each cell's reuseIdentifier and querying for available reusable cells with dequeueReusableCellWithIdentifier:
// Cell gets various attributes set automatically based on table (separators) and data source (accessory views, editing controls)

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    static NSString *U4PlayerCellID = @"U4PlayerCell";
    U4PlayerCell *playerCell = static_cast<U4PlayerCell *>([self.tableView dequeueReusableCellWithIdentifier:U4PlayerCellID]);
    
    if (playerCell == nil) {
        [[NSBundle mainBundle] loadNibNamed:U4PlayerCellID owner:self options:nil];
        self.tableCell.selectionStyle = UITableViewCellSelectionStyleGray;
        playerCell = [[self.tableCell retain] autorelease];
        self.tableCell = nil;
    }
    [self updateCell:playerCell withRecord:[[playerArray objectAtIndex:indexPath.row] playerRecordSheet]];
    if (indexPath.row == 0 && [playerArray count] > 2) {
        playerCell.orderButton.hidden = NO;
        [playerCell.orderButton addTarget:self action:@selector(partyOrderButtonPressed:) forControlEvents:UIControlEventTouchUpInside];
    } else {
        playerCell.orderButton.hidden = YES;
        [playerCell.orderButton removeTarget:self action:@selector(partyOrderButtonPressed:) forControlEvents:UIControlEventTouchUpInside];

    }
    return playerCell;
}

- (void)updateActivePartyMember:(NSInteger)row {
    if (row == -1) {
        NSIndexPath *path = [self.tableView indexPathForSelectedRow];
        if (path != nil)
            [self.tableView deselectRowAtIndexPath:path animated:YES];
    } else {
        [self.tableView selectRowAtIndexPath:[NSIndexPath indexPathForRow:row inSection:0] animated:YES scrollPosition:UITableViewScrollPositionNone];
    }
}

- (void)reloadPartyMembers {
    [playerArray release];
    [self loadPartyDataFromSave];
    [self.tableView reloadData];
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView { 
    return 1;
}

// Editing

// Individual rows can opt out of having the -editing property set for them. If not implemented, all rows are assumed to be editable.
- (BOOL)tableView:(UITableView *)tableView canEditRowAtIndexPath:(NSIndexPath *)indexPath {
    if (indexPath.row == 0)
        return NO;
    return YES;
}

// Moving/reordering

// Allows the reorder accessory view to optionally be shown for a particular row. By default, the reorder control will be shown only if the datasource implements -tableView:moveRowAtIndexPath:toIndexPath:
- (BOOL)tableView:(UITableView *)tableView canMoveRowAtIndexPath:(NSIndexPath *)indexPath {
    if (indexPath.row == 0)
        return NO;
    return YES;
}

// Index

// Data manipulation - reorder / moving support

- (void)tableView:(UITableView *)tableView moveRowAtIndexPath:(NSIndexPath *)sourceIndexPath toIndexPath:(NSIndexPath *)destinationIndexPath {
    int player1 = sourceIndexPath.row;
    int player2 = destinationIndexPath.row;
    
    if (player1 == player2)
        return; // Nothing to do!
    
    if (player1 > player2) {
        SaveGamePlayerRecord newRec = c->saveGame->players[player1];
        for (int i = player2; i <= player1; ++i) {
            SaveGamePlayerRecord oldRec = c->saveGame->players[i];
            c->saveGame->players[i] = newRec;
            newRec = oldRec;
        }
    } else {
        assert(player2 > player1);
        SaveGamePlayerRecord newRec = c->saveGame->players[player1];
        for (int i = player2; i >= player1; --i) {
            SaveGamePlayerRecord oldRec = c->saveGame->players[i];
            c->saveGame->players[i] = newRec;
            newRec = oldRec;            
        }
    }
    moveCount++;
    U4IOS::syncPartyMembersWithSaveGame();
    [self reloadPartyMembers];
}





@end
