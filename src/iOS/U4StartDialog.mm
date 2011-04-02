//
//  U4StartDialog.mm
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

#import "U4StartDialog.h"


@implementation U4StartDialogController
@synthesize delegate;
@synthesize avatarGenderSegment;
@synthesize avatarNameEdit;
@synthesize continueButton;
@synthesize cancelButton;
@synthesize success;

 // The designated initializer.  Override if you create the controller programmatically and want to perform customization that is not appropriate for viewDidLoad.
- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil {
    if ((self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil])) {
        success = NO;
    }
    return self;
}


// Implement viewDidLoad to do additional setup after loading the view, typically from a nib.
- (void)viewDidLoad {
    [super viewDidLoad];
    [self.avatarNameEdit becomeFirstResponder];
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
    self.avatarGenderSegment = nil;
    self.avatarNameEdit = nil;
    self.continueButton = nil;
    self.cancelButton = nil;
}


- (void)dealloc {
    [super dealloc];
}

- (IBAction)continuePressed:(id)sender {
    success = YES;
    if (self.delegate) {
        [self.delegate startDialogControllerDidFinish:self];
    }
}

- (IBAction)cancelPressed:(id)sender {
    success = NO;
    if (self.delegate) {
        [self.delegate startDialogControllerDidFinish:self];
    }
}


- (IBAction)genderSelected:(id)sender {
    if (sender != avatarGenderSegment)
        return;
    [self checkIfContinueShouldBeEnabled];
}

- (IBAction)avatarNameChanged:(id)sender {
    if (sender != avatarNameEdit)
        return;
    [self checkIfContinueShouldBeEnabled];
}

- (void)checkIfContinueShouldBeEnabled {
    continueButton.enabled = [avatarNameEdit.text length] > 0 && avatarGenderSegment.selectedSegmentIndex >= 0;
}

- (NSString *)avatarName {
    return avatarNameEdit.text;
}

- (NSInteger)avatarGender {
    return avatarGenderSegment.selectedSegmentIndex;
}
@end
