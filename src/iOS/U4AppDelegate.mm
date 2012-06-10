//
//  U4AppDelegate.m
//  Ultima4-iPad
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

#import "U4AppDelegate.h"
#import "U4IntroController.h"
#import "U4GameController.h"
#import "U4PlayerTableController.h"


#include "u4.h"

#include "context.h"
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
#include "imagemgr.h"
#include "image.h"
#import "U4View.h"
#include "TestFlight.h"

#if defined(MACOSX)
#include "macosx/osxinit.h"
#endif

// ### These global variables are used in other places, until I can clear those out, they live here.
bool verbose = false;
bool quit = false;
bool useProfile = false;
string profileName = "";

@implementation U4AppDelegate

@synthesize window;
@synthesize introController;
@synthesize gameController;


- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {    
    [TestFlight takeOff:@"436f0b9458ec27dd945423eb48aff336_NzE2MDIwMTEtMTAtMjkgMDU6Mjg6MjAuNTA0MzUy"];
    Debug::initGlobal("debug/global.txt");
    
    /* initialize the settings */
    settings.init(useProfile, profileName);
//    verbose = true;
    
    xu4_srandom();
    u4viewStack = [[NSMutableArray alloc] initWithCapacity:8];
    [[UIApplication sharedApplication] setStatusBarHidden:YES];
    [window addSubview:introController.view];
    [window makeKeyAndVisible];

    return NO;
}

- (void)applicationDidEnterBackground:(UIApplication *)application {
    musicMgr->toggle();
}

- (void)applicationWillEnterForeground:(UIApplication *)application {
    if (c)
        musicMgr->toggle();
    else
        intro->tryTriggerIntroMusic();
}


- (void)dealloc {
    [introController release];
    [gameController release];
    [window release];
    [u4viewStack release];
    [super dealloc];
}

- (void)pushU4View:(U4View *)view {
    static BOOL firstTime = YES;    
    [u4viewStack addObject:view];
    if (firstTime) {
        firstTime = NO;
        return;
    }

    ImageInfo *screenInfo = imageMgr->get("screen");
    screenInfo->image = [view image];
    U4IOS::updateScreenView();
}

- (void)popU4View {
    [u4viewStack removeLastObject];
    if ([u4viewStack count] == 0)
        return;
    ImageInfo *screenInfo = imageMgr->get("screen");
    U4View *view = static_cast<U4View *>([u4viewStack lastObject]);
    screenInfo->image = [view image];
    U4IOS::updateScreenView();
}

- (U4View *)frontU4View {
    return [u4viewStack lastObject];
}

- (void)startGameController {
    [self popU4View];
    self.gameController = [[[U4GameController alloc] initWithNibName:@"U4GameController" bundle:nil] autorelease];
    UIView *introControllerView = self.introController.view;
    [introControllerView removeFromSuperview];
    [introController release];
    introController = nil;
    UIView *gameControllerView = gameController.view;
    [window addSubview:gameControllerView];
    gameControllerView.hidden = NO;
    eventHandler->popController();
    intro->deleteIntro();
    game = new GameController();
    game->init();
    [gameController.playerTableController loadPartyDataFromSave];
    
    // Depending on the orientation, our views may have already loaded, updated the information.
    if (gameController.playerTableController.isViewLoaded) {
        [gameController.playerTableController.tableView reloadData];
        [gameController.playerTableController updateOtherPartyStats];
        U4IOS::IOSObserver::sharedInstance()->update(c->aura);
    }

    eventHandler->pushController(game);
}

@end
