//
//  U4AudioController.mm
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

#import "U4AudioController.h"
#include <cmath>
#include "settings.h"

static const int FadeSteps = 30;

@implementation U4AudioController

- (id)initWithFile:(NSString *)file {
    self = [super init];
    if (self) {
        NSError *playerError = nil;
        player = [[AVAudioPlayer alloc] initWithContentsOfURL:[NSURL fileURLWithPath:file]
                                                        error:&playerError];
        if (player == nil) {
            NSLog(@"Problem creating player: %@", [playerError localizedDescription]);
        } else {
            player.delegate = self;
            player.numberOfLoops = 0;
        }
        interruptedWhilePlaying = NO;
    }
    return self;
}

- (void)dealloc {
    [player release];
    [super dealloc];
}

- (void)audioPlayerDidFinishPlaying:(AVAudioPlayer *)thePlayer successfully:(BOOL)flag {
}

- (void)audioPlayerBeginInterruption:(AVAudioPlayer *)player {
    if (settings.musicVol) {
        interruptedWhilePlaying = YES;
    }
}

- (void)audioPlayerEndInterruption:(AVAudioPlayer *)thePlayer {
    if (interruptedWhilePlaying) {
        if (settings.musicVol) {
            [thePlayer prepareToPlay];
            [thePlayer play];
            interruptedWhilePlaying = NO;
        }
    }
}

- (void)play {
    player.currentTime = 0;
    [player prepareToPlay];
    [player play];
}

- (void)stop {
    [player stop];
}

- (BOOL)isPlaying {
    return player.isPlaying;
}

- (void)setVolume:(float)volume {
    player.volume = volume;
}

- (float)volume {
    return player.volume;
}

- (void)setNumberOfLoops:(int)numberOfLoops {
    player.numberOfLoops = numberOfLoops;
}

- (int)numberOfLoops {
    return player.numberOfLoops;
}

- (void)fadeOut:(NSTimeInterval)duration {
    volumeStep = duration / FadeSteps;
    volumeDelta = player.volume / FadeSteps;
    [self performSelector:@selector(doVolumeFadeOut) withObject:nil afterDelay:volumeStep];
}

- (void)doVolumeFadeOut {
    if (player.volume > volumeDelta) {
        player.volume = player.volume - volumeDelta;
        [self performSelector:@selector(doVolumeFadeOut) withObject:nil afterDelay:volumeStep];
    } else {
        [self stop];
    }
}

- (void)fadeIn:(NSTimeInterval)duration {
    volumeStep = duration / FadeSteps;
    volumeDelta = settings.musicVol / CGFloat(MAX_VOLUME) / FadeSteps;
    if (!player.isPlaying)
        [self play];

    [self performSelector:@selector(doVolumeFadeIn) withObject:nil afterDelay:volumeStep];
}

-(void)doVolumeFadeIn {  
    CGFloat finalVolume = settings.musicVol / CGFloat(MAX_VOLUME);
    if (player.volume < finalVolume - volumeDelta) {
        player.volume = player.volume + volumeDelta;
        [self performSelector:@selector(doVolumeFadeIn) withObject:nil afterDelay:volumeStep];
    } else {
        player.volume = finalVolume;
    }
}

@end
