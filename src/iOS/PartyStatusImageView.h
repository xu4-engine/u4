//
//  PartyStatusImageView.h
//  xu4
//


#import <UIKit/UIKit.h>

class Image;
@interface PartyStatusImageView : UIView {
    CGLayerRef ankth;
    CGLayerRef finalImage;
    Image *charset;
    NSUInteger currentState;
    unsigned char savedEigthsMask;
}

- (void)drawEigths;
- (void)drawHorn;
- (void)drawJinx;
- (void)drawNegate;
- (void)drawProtection;
- (void)drawQuickness;
@end
