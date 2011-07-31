//
//  U4PlayerCharacter.h
//  xu4
//


#import <Foundation/Foundation.h>

class SaveGamePlayerRecord;

@interface U4PlayerCharacter : NSObject {
@private
    const SaveGamePlayerRecord *playerRecordSheet;
}

- (id)initWithPlayerRecord:(const SaveGamePlayerRecord *)record;
- (const SaveGamePlayerRecord *)playerRecordSheet;
@end
