//
//  U4View.mm
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

#import "U4View.h"
#include "image.h"
#include "imagemgr.h"
#include "settings.h"

@implementation U4View

- (void)initHelper {
    ImageWidth = 320 * settings.scale;
    ImageHeight = 200 * settings.scale;

    image = Image::create(ImageWidth, ImageHeight, false, Image::HARDWARE);
    centerPoint = CGPointZero;
}

- (void)computeCenter:(UIInterfaceOrientation)fromInterfaceOrientation {
    CGSize bounds = self.window.frame.size;
    if (UIInterfaceOrientationIsPortrait(fromInterfaceOrientation))
        bounds = CGSizeMake(bounds.height, bounds.width);

    centerPoint = CGPointMake(bounds.width / 2 - ImageWidth / 2,
                              bounds.height / 3 - ImageHeight / 3);    
}

- (id)initWithFrame:(CGRect)frame {
    if ((self = [super initWithFrame:frame])) {
        [self initHelper];
    }
    return self;
}

- (id)initWithCoder:(NSCoder *)decoder {
    if ((self = [super initWithCoder:decoder])) {
        [self initHelper];
    }
    return self;
}

- (Image *)image {
    return image;
}

- (void)drawRect:(CGRect)rect {
    if (CGPointEqualToPoint(centerPoint, CGPointZero))
        [self computeCenter:UIInterfaceOrientationLandscapeLeft];
    CGContextRef context = UIGraphicsGetCurrentContext();
    CGContextDrawLayerAtPoint(context, centerPoint, image->getSurface());
}
    
- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event {
}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event {
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event {
}

- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event {
}

- (void)dealloc {
    [super dealloc];
}

@end
