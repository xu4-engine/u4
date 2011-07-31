//
//  PartyStatusImageView.mm
//  xu4
//


#import "PartyStatusImageView.h"
#include "imagemgr.h"
#include "settings.h"
#include "context.h"
#include "image.h"
#include "ios_helpers.h"
#include "CGContextGStateSaver.h"
#include "U4CFHelper.h"

#include "view.h"
#include "u4.h"

enum CurrentStatus { None, Eigths, Horn, Jinx, Negate, Protection, Quickness };
static const CGFloat kRectSize = 32.0;

@implementation PartyStatusImageView


- (void)initHelper
{
    currentState = None;
    savedEigthsMask = 0xff;
    NSURL *ankthUrl = [[NSBundle mainBundle] URLForResource:@"ankh" withExtension:@"png"];
    boost::intrusive_ptr<CGDataProvider> dataProvider = cftypeFromCreateOrCopy(CGDataProviderCreateWithURL(reinterpret_cast<CFURLRef>(ankthUrl)));
    boost::intrusive_ptr<CGImage> ankhImage = cftypeFromCreateOrCopy(CGImageCreateWithPNGDataProvider(dataProvider.get(), 0, false, kCGRenderingIntentDefault));
    
    CGRect rect = CGRectMake(0, 0, kRectSize, kRectSize);
    ankth = CGLayerCreateWithContext(CGLayerGetContext(imageMgr->get("screen")->image->getSurface()), rect.size, 0);
    CGContextRef context = CGLayerGetContext(ankth);
    CGContextGStateSaver contextSaver(context); 
    CGContextClearRect(context, rect);
    U4IOS::HIViewDrawCGImage(context, &rect, ankhImage.get()); 
    finalImage = CGLayerCreateWithContext(context, rect.size, 0);
}


- (id)initWithFrame:(CGRect)frame
{
    self = [super initWithFrame:frame];
    if (self) {
        [self initHelper];
    }
    return self;
}

- (id)initWithCoder:(NSCoder *)decoder
{
    self = [super initWithCoder:decoder];
    if (self) {
        [self initHelper];
    }
    return self;
}

- (void)dealloc
{
    CFRelease(ankth);
    CFRelease(finalImage);
    [super dealloc];
}

- (void)drawEigths
{
    unsigned char mask = 0xff;
    for (int i = 0; i < VIRT_MAX; i++) {
        if (c->saveGame->karma[i] == 0)
            mask &= ~(1 << i);
    }
    if (currentState != Eigths || mask != savedEigthsMask) {
        savedEigthsMask = mask;
        currentState = Eigths;
        CGContextRef finalImageContext = CGLayerGetContext(finalImage);
        CGContextGStateSaver contextSaver(finalImageContext);
        CGContextClearRect(finalImageContext, CGRectMake(0, 0, kRectSize, kRectSize));
        CGContextDrawLayerAtPoint(finalImageContext, CGPointMake(0, 0), ankth);
        // Now draw black over it at points
        CGContextSetRGBFillColor(finalImageContext, 0., 0., 0., 1.0);
        const CGFloat band = kRectSize / 8;
        for (int i = 0; i < VIRT_MAX; i++) {
            if (mask & (1 << i)) {
                CGContextFillRect(finalImageContext, CGRectMake(0, 0 + i * band, kRectSize, band));
            }
        }
    }
    [self setNeedsDisplay];
}

- (void)drawHorn
{
    if (currentState != Horn) {
        currentState = Horn;
        CGRect rect = CGRectMake(0., 0., kRectSize, kRectSize);
        CGContextRef finalImageContext = CGLayerGetContext(finalImage);
        CGContextGStateSaver contextSaver(finalImageContext);
        CGContextClearRect(finalImageContext, rect);
        CGColorSpaceRef colorspace = U4IOS::u4colorSpace();
        CGFloat components[] = { 0.95, 0.0, 0.0, 1.0, 0.1, 0.0, 0.0, 1.0 };
        CGFloat locations[] = {0., 1.};
        boost::intrusive_ptr<CGGradient> gradient = cftypeFromCreateOrCopy(CGGradientCreateWithColorComponents(colorspace, components, locations, 2));
        CGContextDrawRadialGradient(finalImageContext, gradient.get(), CGPointMake(kRectSize / 3., kRectSize / 3.),
                                    0, CGPointMake(kRectSize / 2, kRectSize / 2), kRectSize / 2, kCGGradientDrawsAfterEndLocation);
    }
    [self setNeedsDisplay];    
}

- (void)drawCharacter:(const char *)str {
    CGRect rect = CGRectMake(0., 0., kRectSize, kRectSize);
    CGContextRef finalImageContext = CGLayerGetContext(finalImage);
    CGContextGStateSaver contextSaver(finalImageContext);
    CGContextTranslateCTM(finalImageContext, 0, kRectSize);
    CGContextScaleCTM(finalImageContext, 1, -1);
    CGContextClearRect(finalImageContext, rect);
    CGContextSelectFont(finalImageContext, "Helvetica-Bold", kRectSize, kCGEncodingMacRoman);
    CGContextSetTextDrawingMode(finalImageContext, kCGTextFillStroke);
    CGContextSetRGBFillColor(finalImageContext, 1.0, 1.0, 1.0, 1.0);
    CGContextSetRGBStrokeColor(finalImageContext, 0.25, 0.25, 0.25, 0.75);
    CGContextShowTextAtPoint(finalImageContext, 0, 0, str, 1);
}

- (void)drawJinx
{
    if (currentState != Jinx) {
        currentState = Jinx;
        [self drawCharacter:"J"];
    }
    [self setNeedsDisplay];
}

- (void)drawNegate
{
    if (currentState != Negate) {
        currentState = Negate;
        [self drawCharacter:"N"];
    }
    [self setNeedsDisplay];
    
}

- (void)drawProtection
{
    if (currentState != Protection) {
        currentState = Protection;
        [self drawCharacter:"P"];
    }
    [self setNeedsDisplay];
    
}

- (void)drawQuickness
{
    if (currentState != Quickness) {
        currentState = Quickness;
        [self drawCharacter:"Q"];
    }
    [self setNeedsDisplay];
}

// Only override drawRect: if you perform custom drawing.
// An empty implementation adversely affects performance during animation.
- (void)drawRect:(CGRect)rect
{
    CGContextRef context = UIGraphicsGetCurrentContext();
    CGContextDrawLayerAtPoint(context, CGPointZero, finalImage);
}

@end
