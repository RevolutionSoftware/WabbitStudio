//
//  WCDebuggerWindowController.h
//  WabbitStudio
//
//  Created by William Towe on 4/30/11.
//  Copyright 2011 Revolution Software. All rights reserved.
//

#import <AppKit/NSWindowController.h>
#import "RSCalculatorOwnerProtocol.h"


@class RSLCDView;

@interface WCDebuggerWindowController : NSWindowController <NSWindowDelegate> {
@private
    IBOutlet RSLCDView *_LCDView;
}
@property (readonly,nonatomic) id <RSCalculatorOwner> calculatorOwner;
@property (readonly,nonatomic) RSLCDView *LCDView;
@end
