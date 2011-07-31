//
//  U4PlayerCharacter.mm
//  xu4


#import "U4PlayerCharacter.h"

#include "player.h"

@implementation U4PlayerCharacter

- (id)initWithPlayerRecord:(const SaveGamePlayerRecord *)record {
    self = [super init];
    if (self)
        playerRecordSheet = record;
    return self;
}

- (const SaveGamePlayerRecord *)playerRecordSheet {
    return playerRecordSheet;
}
@end
