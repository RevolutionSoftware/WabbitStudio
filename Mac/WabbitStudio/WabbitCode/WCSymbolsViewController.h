//
//  WCSymbolsViewController.h
//  WabbitStudio
//
//  Created by William Towe on 4/2/11.
//  Copyright 2011 Revolution Software. All rights reserved.
//

#import "WCProjectNavigationViewController.h"


@interface WCSymbolsViewController : WCProjectNavigationViewController <NSOutlineViewDelegate,NSOutlineViewDataSource> {
@private
    IBOutlet NSOutlineView *_outlineView;
	
	NSMutableArray *_symbols;
	NSMutableArray *_filteredSymbols;
	NSString *_filterString;
}
@property (readonly,nonatomic) NSOutlineView *outlineView;
@property (copy,nonatomic) NSString *filterString;

- (IBAction)filterSymbols:(id)sender;
- (IBAction)symbolsOutlineViewSingleClick:(id)sender;
- (IBAction)symbolsOutlineViewDoubleClick:(id)sender;
@end
