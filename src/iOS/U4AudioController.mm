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
        interruptedWhilePlaying = playing = NO;
    }
    return self;
}

- (void)dealloc {
    [player release];
    [super dealloc];
}

- (void)audioPlayerDidFinishPlaying:(AVAudioPlayer *)player successfully:(BOOL)flag {
    playing = NO;
}

- (void)audioPlayerBeginInterruption:(AVAudioPlayer *)player {
    if (playing) {
        playing = NO;
        interruptedWhilePlaying = YES;
    }
}

- (void)audioPlayerEndInterruption:(AVAudioPlayer *)thePlayer {
    if (interruptedWhilePlaying) {
        [thePlayer prepareToPlay];
        [thePlayer play];
        playing = YES;
        interruptedWhilePlaying = NO;
    }
}

- (void)play {
    [player prepareToPlay];
    [player play];
    playing = YES;
}

- (void)stop {
    [player stop];
    playing = NO;
}

- (BOOL)isPlaying {
    return playing;
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

@end
