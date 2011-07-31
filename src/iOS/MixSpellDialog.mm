//
//  MixSpell.mm
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

#import "MixSpellDialog.h"
#import "SpellInfo.h"
#import "SpellReagent.h"
#import "SpellInfoPickerLabel.h"
#include "event.h"
#include "spell.h"
#include "ios_helpers.h"

@implementation MixSpellDialog
@synthesize spellPicker;
@synthesize componentView;
@synthesize spellDescription;
@synthesize spellSlider;
@synthesize totalSpells;
@synthesize navBar;
@synthesize tableCell;
@synthesize delegate;
@synthesize pickerLabel;

 // The designated initializer.  Override if you create the controller programmatically and want to perform customization that is not appropriate for viewDidLoad.
- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil {
    if ((self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil])) {
        const SaveGame * const saveGame = c->saveGame;
        NSArray *spellNames = [NSArray arrayWithObjects:@"Awaken", @"Blink", @"Cure", @"Dispel",
                                                        @"Energy Field", @"Fireball", @"Gate Travel",
                                                        @"Heal", @"Iceball", @"Jinx", @"Kill", @"Light",
                                                        @"Magic Missle", @"Negate", @"Open", @"Protection",
                                                        @"Quickness", @"Resurrect", @"Sleep", @"Tremor",
                                                        @"Undead", @"View", @"Wind Change", @"Xit", @"Y-Up",
                                                        @"Z-Down", nil];
        NSMutableArray *basicSpells = [NSMutableArray arrayWithCapacity:SPELL_MAX];
        int i = 0;
        for (NSString *spell in spellNames) {
            int spellCount = saveGame->mixtures[i++];
            SpellInfo *spInfo = [[SpellInfo alloc] init];
            spInfo.name = spell;
            spInfo.amount = spellCount;
            [basicSpells addObject:spInfo];
            [spInfo release];
        }

        allSpells = [[NSArray alloc] initWithArray:basicSpells];
        NSMutableArray *tmpReagents = [NSMutableArray arrayWithCapacity:REAG_MAX];
        for (int i = 0; i < REAG_MAX; ++i) {
            int reagentCount = saveGame->reagents[i];
            if (reagentCount > 0) {
                SpellReagent *regen = [[SpellReagent alloc] init];
                regen.name = const_cast<NSString *>(reinterpret_cast<const NSString *>(U4IOS::reagentAsString(Reagent(i))));
                regen.amount = reagentCount;
                regen.selected = NO;
                regen.index = i;
                [tmpReagents addObject:regen];
                [regen release];
            }
        }
        allComponents = [[NSArray alloc] initWithArray:tmpReagents];
    }
    return self;
}



// Implement viewDidLoad to do additional setup after loading the view, typically from a nib.
- (void)viewDidLoad {
    [super viewDidLoad];
    UINavigationItem *myItem = navBar.topItem;
    myItem.leftBarButtonItem = [[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemCancel target:self action:@selector(cancelMix:)];
    myItem.rightBarButtonItem = [[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemDone target:self action:@selector(mixSpell:)];
    [self pickerView:spellPicker didSelectRow:0 inComponent:0];
}

- (void)cancelMix:(id)sender {
    [delegate mixWasCanceled:self];
}

- (void)mixSpell:(id)sender {
    Ingredients ingred;
    for (SpellReagent *reagent in allComponents) {
        if (reagent.selected)
            ingred.addReagent(Reagent(reagent.index));
    }
    [delegate mixSpellDialog:self chooseSpell:[self.spellPicker selectedRowInComponent:0] reagents:&ingred numberOfSpells:int([spellSlider value])];
}


- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
    // Overriden to allow any orientation.
    return YES;
}


- (void)didReceiveMemoryWarning {
    // Releases the view if it doesn't have a superview.
    [super didReceiveMemoryWarning];
    
    // Release any cached data, images, etc that aren't in use.
}


- (void)viewDidUnload {
    [super viewDidUnload];
    self.spellPicker = nil;
    self.componentView = nil;
    self.spellDescription = nil;
    self.spellSlider = nil;
    self.pickerLabel = nil;
    self.tableCell = nil;
}

- (void)viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];
    [componentView deselectRowAtIndexPath:[componentView indexPathForSelectedRow] animated:NO];
}

- (void)viewDidAppear:(BOOL)animated {
    [super viewDidAppear:animated];
    [componentView flashScrollIndicators];
}


- (void)dealloc {
    [pickerLabel release];
    [spellPicker release];
    [componentView release];
    [spellDescription release];
    [spellSlider release];
    [allComponents release];
    [allSpells release];
    [totalSpells release];
    [navBar release];
    [tableCell release];
    [super dealloc];
}

// TableView stuff
- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    return 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    // Number of rows is the number of time zones in the region for the specified section.
    return [allComponents count];
}


- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section {
    return @"Reagents";
}


- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    static NSString *MyIdentifier = @"SpellReagentTableCell";
    SpellReagentTableCell *cell = static_cast<SpellReagentTableCell *>([tableView dequeueReusableCellWithIdentifier:MyIdentifier]);
    if (cell == nil) {
        [[NSBundle mainBundle] loadNibNamed:MyIdentifier owner:self options:nil];
        self.tableCell.selectionStyle = UITableViewCellSelectionStyleNone;
        cell = [[self.tableCell retain] autorelease];
        self.tableCell = nil;
    }
    SpellReagent *reagent = [allComponents objectAtIndex:indexPath.row];
    cell.reagent = reagent;
    cell.reagentNameLabel.text = reagent.name;
    cell.reagentCountLabel.text = [NSString stringWithFormat:@"%d", reagent.amount];
    return cell;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
    SpellReagentTableCell *thisCell = static_cast<SpellReagentTableCell *>([tableView cellForRowAtIndexPath:indexPath]);
    SpellReagent *reagent = thisCell.reagent;
    BOOL selected = !reagent.selected;
    reagent.selected = selected;
    
    static const unichar CheckMark = 0x2713;
    thisCell.reagentSelectedLabel.text = [NSString stringWithCharacters:&CheckMark length:1];
    thisCell.reagentCountLabel.text = [NSString stringWithFormat:@"%d", selected ? reagent.amount - 1 : reagent.amount];
    // ### Animate?
    thisCell.reagentSelectedLabel.hidden = !selected;

}


// PickerView Stuff
- (NSInteger)numberOfComponentsInPickerView:(UIPickerView *)pickerView {
    return 1;
}

// ### Just leave this here for the moment.
//- (CGFloat)pickerView:(UIPickerView *)pickerView widthForComponent:(NSInteger)component {
//    return 740.;
//}
//- (CGFloat)pickerView:(UIPickerView *)pickerView rowHeightForComponent:(NSInteger)component {
//    return 56.;
//}

- (NSInteger)pickerView:(UIPickerView *)pickerView numberOfRowsInComponent:(NSInteger)component {
    return [allSpells count];
}

- (UIView *)pickerView:(UIPickerView *)pickerView viewForRow:(NSInteger)row forComponent:(NSInteger)component reusingView:(UIView *)view {
    SpellInfo *sp = [allSpells objectAtIndex:row];
    SpellInfoPickerLabel *label = nil;
    if ([view isKindOfClass:[SpellInfoPickerLabel class]]) {
        label = static_cast<SpellInfoPickerLabel *>(view);
    } else {
        [[NSBundle mainBundle] loadNibNamed:@"SpellInfoPickerLabel" owner:self options:nil];
        label = self.pickerLabel;
        [[label retain] autorelease];
        self.pickerLabel = nil;
    }
    label.nameLabel.text = sp.name;
    label.amountLabel.text = [NSString stringWithFormat:@"%d", sp.amount];
    return label;
}

- (void)pickerView:(UIPickerView *)pickerView didSelectRow:(NSInteger)row inComponent:(NSInteger)component {
    SpellInfo *spI = [allSpells objectAtIndex:row];
    // ### Possibly Cache this
    NSString *spellDescriptionPath = [[NSBundle mainBundle] pathForResource:spI.name ofType:@"txt" inDirectory:@"spells"];
    NSError *ignored;
    spellDescription.text = [NSString stringWithContentsOfFile:spellDescriptionPath encoding:NSUTF8StringEncoding error:&ignored];
    [spellDescription flashScrollIndicators];
}


- (IBAction)spellCountChanged:(id)sender {
    totalSpells.text = [NSString stringWithFormat:@"%d", int([spellSlider value])];
}
@end
