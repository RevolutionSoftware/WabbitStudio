//
//  WCNewFileAccessoryViewController.h
//  WabbitStudio
//
//  Created by William Towe on 4/3/11.
//  Copyright 2011 Revolution Software. All rights reserved.
//

#import <AppKit/NSViewController.h>
#import <AppKit/NSPopUpButton.h>

@class WCNewFileWindowController,WCFile;

@interface WCNewFileAccessoryViewController : NSViewController <NSOpenSavePanelDelegate> {
@private
    IBOutlet NSPopUpButton *_groupPopUpButton;
	
	__weak WCNewFileWindowController *_controller;
}
@property (readonly,nonatomic) WCFile *selectedGroup;

+ (void)presentSheetForNewFileWindowController:(WCNewFileWindowController *)controller;
@end
