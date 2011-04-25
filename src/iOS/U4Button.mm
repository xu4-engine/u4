//
//  U4Button.mm
//  xu4
//
//  Created by Trenton Schulz on 4/25/11.
//  Copyright 2011 Norwegian Computing Center. All rights reserved.
//

#import "U4Button.h"
#import <QuartzCore/QuartzCore.h>



@interface U4Button (hidden)
- (void)finishInit;
@end

@implementation U4Button (hidden)

- (void)finishInit {
    [[self layer] setCornerRadius:8.0f];
    [[self layer] setBorderColor:[[UIColor grayColor] CGColor]];
    [[self layer] setMasksToBounds:YES];
    [[self layer] setBorderWidth:1.0f];    
}

@end

@implementation U4Button
- (id)initWithFrame:(CGRect)frame {
    self = [super initWithFrame:frame];
    if (self)
        [self finishInit];
    return self;
}

- (id)initWithCoder:(NSCoder *)aDecoder {
    self = [super initWithCoder:aDecoder];
    if (self)
        [self finishInit];
    return self;
}
@end
