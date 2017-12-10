//
//  WCFilesTreeController.h
//  WabbitStudio
//
//  Created by William Towe on 3/21/11.
//  Copyright 2011 Revolution Software. All rights reserved.
//

#import <AppKit/NSTreeController.h>
#import <AppKit/NSOutlineView.h>


@class WCProject,WCAddFilesToProjectViewController;

@interface WCFilesTreeController : NSTreeController <NSOutlineViewDataSource> {
@private
	WCAddFilesToProjectViewController *_currentAddToFilesController;
}
@end
