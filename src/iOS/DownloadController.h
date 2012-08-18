//
//  DownloadController.h
//  xu4
//
//  Created by Trenton Schulz on 7/31/12.
//
//

#import <UIKit/UIKit.h>

@interface DownloadController : UIViewController<NSURLConnectionDataDelegate> {
    UIProgressView *downloadIndicator;
    NSMutableData *bufferData;
    long long totalBytes;
}
- (IBAction) startConnection:(id)sender;
@property (retain, nonatomic) IBOutlet UIButton *tryAgainButton;
@property (retain, nonatomic) IBOutlet UILabel *errorLabel;
@property (retain, nonatomic) IBOutlet UIProgressView *downloadIndicator;

@end
