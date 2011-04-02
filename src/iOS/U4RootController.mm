//
//  U4RootController.mm
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

#include "debug.h"
#include "error.h"
#include "event.h"
#include "game.h"
#include "intro.h"
#include "music.h"
#include "person.h"
#include "screen.h"
#include "settings.h"
#include "sound.h"
#include "tileset.h"
#include "utils.h"
#include "progress_bar.h"

#import "U4RootController.h"
#import "U4ViewController.h"
#import "U4IntroController.h"
#import "U4GameController.h"

@implementation U4RootController
@synthesize gameViewController;
@synthesize introController;
@synthesize gameController;

 // The designated initializer.  Override if you create the controller programmatically and want to perform customization that is not appropriate for viewDidLoad.
- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil {
    if ((self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil])) {
        finishFirstTimeLoad = NO;
    }
    return self;
}

- (void)doLayout {
    // ### Someday this should be pushed down into the view I'm controlling,
    // but since, at the moment, it's a "one time" thing, it's OK here.
    CGRect totalSize = [UIScreen mainScreen].applicationFrame;
    totalSize.origin.y = totalSize.origin.x = 0.;
    CGRect gameViewFrame = totalSize;
    CGRect introControllerFrame = totalSize;
    CGFloat twoThirdsHeight = 2. * totalSize.size.height / 3.;
    gameViewFrame.size.height = twoThirdsHeight;
    introControllerFrame.origin.y += twoThirdsHeight;
    introControllerFrame.size.height -= twoThirdsHeight;
    self.gameViewController.view.frame = gameViewFrame;
    self.introController.view.frame = introControllerFrame;
}

// Implement viewDidLoad to do additional setup after loading the view, typically from a nib.
- (void)viewDidLoad {
    [super viewDidLoad];
    UIView *contentView = self.view;
    [contentView addSubview:self.gameViewController.view];
    [contentView addSubview:self.introController.view];
}

- (void)viewWillAppear:(BOOL)animated {
    [self doLayout];
    [super viewWillAppear:animated];
}

- (void)viewDidAppear:(BOOL)animated {
    [super viewDidAppear:animated];
    if (finishFirstTimeLoad)
        return;
    finishFirstTimeLoad = YES;
    screenInit();
    ProgressBar pb((320/2) - (200/2), (200/2), 200, 10, 0, 7);
    pb.setBorderColor(240, 240, 240);
    pb.setColor(0, 0, 128);
    pb.setBorderWidth(1);
    
    screenTextAt(15, 11, "Loading...");
    screenRedrawScreen();
    ++pb;
    
    soundInit();
    ++pb;
    
    Tileset::loadAll();
    ++pb;
    
    creatureMgr->getInstance();
    ++pb;
    
    if (intro == 0)
        intro = new IntroController();
    /* do the intro */
//    intro->skipTitles();
    intro->init();
    ++pb;
    
    intro->preloadMap();
    ++pb;
    
    musicMgr->init();
    ++pb;
    
    eventHandler->pushController(intro);
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
    // Overriden to allow any orientation.
    if (UIInterfaceOrientationIsLandscape(interfaceOrientation))
        return NO;
    return YES;
}


- (void)didReceiveMemoryWarning {
    // Releases the view if it doesn't have a superview.
    [super didReceiveMemoryWarning];
    [self cleanUpController];
    // Release any cached data, images, etc that aren't in use.
}


- (void)viewDidUnload {
    [super viewDidUnload];
    self.introController = nil;
    self.gameViewController = nil;
    self.gameController = nil;
    [self cleanUpController];
    // Release any retained subviews of the main view.
    // e.g. self.myOutlet = nil;
}


- (void)dealloc {
    [super dealloc];
}

- (void)cleanUpController {
    if (self.introController && self.introController.view.hidden) {
        if (intro) {
            intro->deleteIntro();
            delete intro;
            intro = 0;
        }
        self.introController = nil;
    }
}

- (void)startGameController {
    eventHandler->popController();
    intro->deleteIntro();
    game = new GameController();
    game->init();
    eventHandler->pushController(game);
    self.gameController = [[[U4GameController alloc] initWithNibName:@"U4GameController" bundle:nil] autorelease];
    UIView *introControllerView = self.introController.view;
    CGRect introFrame = introControllerView.frame;
    [introControllerView removeFromSuperview];
    UIView *gameControllerView = gameController.view;
    [self.view addSubview:gameControllerView];
    gameControllerView.frame = introFrame;
    gameControllerView.hidden = NO;
}

@end
