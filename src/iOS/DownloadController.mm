//
//  DownloadController.m
//  xu4
//
//  Created by Trenton Schulz on 7/31/12.
//
//

#import "DownloadController.h"
#import "U4AppDelegate.h"
#include <CommonCrypto/CommonDigest.h>
#include "u4file.h"
#include <string>
#include "U4CFHelper.h"
#include "ios_helpers.h"

static const unsigned char const Ultima4ZipMD5[] = { 0xf2, 0x00, 0x6a, 0x5d, 0xbf, 0x17, 0x55, 0x71,
                                                     0x91, 0x2e, 0xf2, 0x59, 0x4b, 0x6e, 0xb9, 0x00 };

static const unsigned char const Ultima4ExeMD5[] = { 0x06, 0x1f, 0x94, 0x35, 0xb5, 0xde, 0xb9, 0x43,
                                                     0x50, 0x81, 0x6f, 0x67, 0xf1, 0x0d, 0xaa, 0x71 };

static const int Ultima4ZipNumBytes = 529099;

// static const int Ultima4ExeNumBytes = 400409;

NSString *ZipURL = @"http://www.thatfleminggent.com/ultima/ultima4.zip";
NSString *ExeURL = @"http://cheekygamers.com/woo/games/ultima4.exe";

@interface DownloadController ()

@end

@implementation DownloadController
@synthesize tryAgainButton;
@synthesize errorLabel;
@synthesize downloadIndicator;

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
    }
    return self;
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    [self startConnection:self];
}

- (IBAction)startConnection:(id)sender {
    self.errorLabel.hidden = YES;
    self.tryAgainButton.hidden = YES;
    // Decide which URL to get.
    NSString *url = random() % 2 ? ZipURL : ExeURL;
    NSURLRequest *req = [NSURLRequest requestWithURL:[NSURL URLWithString:url]];
    NSURLConnection *downloadConnection = [[NSURLConnection alloc] initWithRequest:req delegate:self];
    [downloadConnection autorelease];
}

- (void)viewDidUnload
{
    [self setDownloadIndicator:nil];
    [self setErrorLabel:nil];
    [self setTryAgainButton:nil];
    [super viewDidUnload];
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
	return YES;
}

- (void)dealloc {
    [downloadIndicator release];
    [errorLabel release];
    [tryAgainButton release];
    [super dealloc];
}

- (void)connection:(NSURLConnection *)connection didReceiveData:(NSData *)data {
    [bufferData appendData:data];
    [downloadIndicator setProgress:[bufferData length] / totalBytes animated:YES];
}

- (void)connection:(NSURLConnection *)connection didReceiveResponse:(NSURLResponse *)response {
    
    totalBytes = [response expectedContentLength];
    if (totalBytes == NSURLResponseUnknownLength)
        totalBytes = Ultima4ZipNumBytes;
    
    bufferData = [[NSMutableData alloc] initWithCapacity:totalBytes];
}

- (void)handleError:(NSString *)errorMessage {
    self.errorLabel.text = errorMessage;
    self.errorLabel.hidden = NO;
    self.tryAgainButton.hidden = NO;
}

- (void)connectionDidFinishLoading:(NSURLConnection *)connection {
    [downloadIndicator setProgress:1.0 animated:YES];
    unsigned char result[CC_MD5_DIGEST_LENGTH];
    CC_MD5([bufferData bytes], [bufferData length], result);
    if (memcmp(result, Ultima4ZipMD5, CC_MD5_DIGEST_LENGTH) == 0
        || memcmp(result, Ultima4ExeMD5, CC_MD5_DIGEST_LENGTH) == 0) {
        
        boost::intrusive_ptr<CFURL> urlForLocation = cftypeFromCreateOrCopy(U4IOS::copyAppSupportDirectoryLocation());
        boost::intrusive_ptr<CFURL> u4zipURL = cftypeFromCreateOrCopy(CFURLCreateCopyAppendingPathComponent(kCFAllocatorDefault, urlForLocation.get(), CFSTR("ultima4.zip"), false));
        
        NSError *fileError;
        if ([bufferData writeToURL:(NSURL*)u4zipURL.get() options:NSDataWritingAtomic error:&fileError]) {
            U4AppDelegate *appDelegate = [UIApplication sharedApplication].delegate;
            [appDelegate performSelector:@selector(startIntroController)
                              withObject:nil afterDelay:.125];
        } else {
            [self handleError:[fileError localizedDescription]];
        }
    } else {
        [self handleError:NSLocalizedString(@"MD5 Checksum Error", @"Error string for the download not matching the checksum")];
    }
}

- (BOOL)connection:(NSURLConnection *)connection canAuthenticateAgainstProtectionSpace:(NSURLProtectionSpace *)protectionSpace {
    return NO;
}

- (void)connection:(NSURLConnection *)theConnection didFailWithError:(NSError *)error {
    NSString *errorString = [error localizedDescription];
    if ([error localizedFailureReason] != nil) {
        errorString = [errorString stringByAppendingFormat:@": %@", [error localizedFailureReason]];
    }
    [self handleError:errorString];
}

- (BOOL)connectionShouldUseCredentialStorage:(NSURLConnection *)connection {
    return NO;
}


@end
