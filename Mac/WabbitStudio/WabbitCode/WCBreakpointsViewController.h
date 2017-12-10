//
//  WCBreakpointsViewController.h
//  WabbitStudio
//
//  Created by William Towe on 4/19/11.
//  Copyright 2011 Revolution Software. All rights reserved.
//

#import "WCProjectNavigationViewController.h"

@class WCBreakpoint,BWAnchoredButton,BWAnchoredPopUpButton;

@interface WCBreakpointsViewController : WCProjectNavigationViewController <NSOutlineViewDelegate,NSOutlineViewDataSource> {
@private
    IBOutlet NSOutlineView *_outlineView;
	
	
}
@property (readonly,nonatomic) NSOutlineView *outlineView;

- (IBAction)breakpointsOutlineViewSingleClick:(id)sender;
- (IBAction)breakpointsOutlineViewDoubleClick:(id)sender;
@end
