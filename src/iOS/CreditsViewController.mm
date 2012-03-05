//
//  CreditsViewController.m
//  xu4
//
//

#import "CreditsViewController.h"

@implementation  CreditsViewController (privateStuff)
- (void)dismissCredits {
    if (delegate) {
        [delegate CreditsViewControllerDidFinish:self];
    }
}
@end


@implementation CreditsViewController
@synthesize webView;
@synthesize navBar;
@synthesize delegate;

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        // Custom initialization
    }
    return self;
}

- (void)dealloc
{
    [webView release];
    [navBar release];
    [super dealloc];
}

- (void)didReceiveMemoryWarning
{
    // Releases the view if it doesn't have a superview.
    [super didReceiveMemoryWarning];
    
    // Release any cached data, images, etc that aren't in use.
}

#pragma mark - View lifecycle

- (void)viewDidLoad
{
    [super viewDidLoad];
    UINavigationItem *myItem = navBar.topItem;
    myItem.rightBarButtonItem = [[[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemDone target:self action:@selector(dismissCredits)] autorelease];
    NSURL *creditsPath = [[NSBundle mainBundle] URLForResource:@"credits" withExtension:@"html"];
    NSData *data = [NSData dataWithContentsOfURL:creditsPath];
    [webView loadData:data MIMEType:@"text/html" textEncodingName:@"UTF-8" baseURL:creditsPath];
}

- (void)viewDidUnload
{
    [self setWebView:nil];
    [self setNavBar:nil];
    [super viewDidUnload];
    // Release any retained subviews of the main view.
    // e.g. self.myOutlet = nil;
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
    // Return YES for supported orientations
	return YES;
}

@end
