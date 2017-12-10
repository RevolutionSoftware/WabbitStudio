//
//  WCTextView.m
//  WabbitStudio
//
//  Created by William Towe on 3/23/11.
//  Copyright 2011 Revolution Software. All rights reserved.
//

#import "WCTextView.h"
#import "WCFile.h"
#import "WCSyntaxHighlighter.h"
#import "WCSymbolScanner.h"
#import "WCLineHighlighter.h"
#import "WCPreferencesController.h"
#import "NSUserDefaults+WCExtensions.h"
#import "WCSymbol.h"
#import "WCProject.h"
#import "WCFileViewController.h"
#import "WCBuildMessage.h"
#import "WCTextStorage.h"
#import "NSObject+WCExtensions.h"
#import "WCBuildTarget.h"
#import "WCFindBarViewController.h"
#import "WCFileViewController.h"
#import "WCFindInProjectViewController.h"
#import "WCGotoLineSheetController.h"
#import "NSTextView+WCExtensions.h"
#import "WCBreakpoint.h"
#import "WCFileWindowController.h"
#import "WCProjectFilesOutlineViewController.h"
#import "NSTreeController+WCExtensions.h"
#import "WCTooltipManager.h"
#import "WCTooltip.h"

// without this xcode complains about the restrict qualifiers in the regexkit header
#define restrict
#import <RegexKit/RegexKit.h>

@interface WCTextView ()
@end

@implementation WCTextView
#pragma mark *** Subclass Overrides ***
- (void)dealloc {
#ifdef DEBUG
	NSLog(@"%@ called in %@",NSStringFromSelector(_cmd),[self className]);
#endif
	if (_mouseMovedTimer != nil)
		[_mouseMovedTimer invalidate];
	[[NSNotificationCenter defaultCenter] removeObserver:self];
	[self cleanupUserDefaultsObserving];
	[_syntaxHighlighter release];
	[_lineHighlighter release];
	_file = nil;
    [super dealloc];
}

- (id)initWithCoder:(NSCoder *)decoder {
	if (!(self = [super initWithCoder:decoder]))
		return nil;
	
	_lineHighlighter = [[WCLineHighlighter alloc] initWithTextView:self];
	
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_selectionDidChange:) name:NSTextViewDidChangeSelectionNotification object:self];
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_boundsDidChange:) name:NSViewBoundsDidChangeNotification object:[[self enclosingScrollView] contentView]];
	
	[self setupUserDefaultsObserving];
	
	return self;
}

- (void)awakeFromNib {
	[super awakeFromNib];
	[self toggleWrapLines:nil];
}

- (void)mouseDown:(NSEvent *)event {
	[super mouseDown:event];
	
	if ([event type] == NSLeftMouseDown &&
		[event clickCount] == 2 &&
		(([event modifierFlags] & NSCommandKeyMask) != 0)) {
		
		NSRange range = [self symbolRangeForRange:NSMakeRange([self characterIndexForInsertionAtPoint:[self convertPointFromBase:[event locationInWindow]]], 0)];
		
		if (range.location == NSNotFound)
			return;
		
		[self setSelectedRange:range];
		[self jumpToDefinition:nil];
	}
}

- (void)mouseMoved:(NSEvent *)theEvent {
	[super mouseMoved:theEvent];
	
	if (![[NSUserDefaults standardUserDefaults] boolForKey:kWCPreferencesEditorShowEquateValueTooltipsKey])
		return;
	
	if (_mouseMovedTimer == nil)
		_mouseMovedTimer = [NSTimer scheduledTimerWithTimeInterval:kTooltipDelay target:self selector:@selector(_mouseMovedTimerFired:) userInfo:nil repeats:NO];
	else {
		[_mouseMovedTimer setFireDate:[NSDate dateWithTimeIntervalSinceNow:kTooltipDelay]];
		
		[[WCTooltipManager sharedTooltipManager] hideTooltip];
	}
}

- (void)_mouseMovedTimerFired:(NSTimer *)timer {
	_mouseMovedTimer = nil;
	if (!NSMouseInRect([[[self window] currentEvent] locationInWindow], [self convertRectToBase:[self visibleRect]], [self isFlipped]))
		return;
	
	NSString *string = [self symbolStringForRange:NSMakeRange([self characterIndexForInsertionAtPoint:[self convertPointFromBase:[[[self window] currentEvent] locationInWindow]]],0)];
	
	if (string == nil)
		return;
	
	NSArray *symbols = ([[self file] project] == nil)?[[[self file] symbolScanner] equatesForSymbolName:string]:[[[self file] project] equatesForSymbolName:string];
	
	if ([symbols count] == 0)
		return;
	
	NSMutableString *tString = [NSMutableString string];
	
	for (WCSymbol *symbol in symbols)
		[tString appendFormat:@"%@\n",[symbol symbolValue]];
	
	NSPoint tPoint = [[self window] convertBaseToScreen:[[[self window] currentEvent] locationInWindow]];
	
	tPoint.x += floor([[NSCursor currentSystemCursor] image].size.width/2.0);
	tPoint.y -= floor([[NSCursor currentSystemCursor] image].size.height/2.0);
	
	[[WCTooltipManager sharedTooltipManager] showTooltip:[WCTooltip tooltipWithString:tString atLocation:tPoint]];
}

- (void)drawViewBackgroundInRect:(NSRect)rect {
	[super drawViewBackgroundInRect:rect];
	
	NSRect visibleRect = [self visibleRect];
	
	if ([[NSUserDefaults standardUserDefaults] boolForKey:kWCPreferencesCurrentLineHighlightKey]) {
		NSRange range = [self selectedRange];
		
		if (range.length == 0) {
			if (range.location < [[self string] length]) {
				NSUInteger rectCount = 0;
				NSRectArray newRects = [[self layoutManager] rectArrayForCharacterRange:[[self string] lineRangeForRange:range] withinSelectedCharacterRange:NSMakeRange(NSNotFound, 0) inTextContainer:[self textContainer] rectCount:&rectCount];
				NSRect newRect = NSZeroRect;
				
				if (rectCount > 0) {
					newRect = newRects[0];
					
					newRect.origin.x = NSMinX([self bounds]);
					newRect.size.width = NSWidth([self bounds]);
					
					[[[NSUserDefaults standardUserDefaults] colorForKey:kWCPreferencesCurrentLineHighlightColorKey] setFill];
					NSRectFillUsingOperation(newRect, NSCompositeCopy);
				}
			}
		}
	}
	
	if ([[[self file] project] isDebugging]) {
		if ([[[self file] project] currentBreakpointFile] == [self file]) {
			NSRect lineRect = [[self layoutManager] lineFragmentRectForGlyphAtIndex:[(WCTextStorage *)[self textStorage] safeLineStartIndexForLineNumber:[[[self file] project] currentBreakpointLineNumber]] effectiveRange:NULL];
			
			if (NSIntersectsRect(lineRect, visibleRect) && [self needsToDrawRect:lineRect]) {
				NSColor *breakpointColor = [[NSUserDefaults standardUserDefaults] colorForKey:kWCPreferencesEditorBreakpointLineHighlightColorKey];
				
				[[breakpointColor colorWithAlphaComponent:0.4] setFill];
				NSRectFillUsingOperation(lineRect, NSCompositeSourceOver);
				[[breakpointColor colorWithAlphaComponent:0.6] setFill];
				NSRectFillUsingOperation(NSMakeRect(lineRect.origin.x, lineRect.origin.y, lineRect.size.width, 1.0), NSCompositeSourceOver);
				NSRectFillUsingOperation(NSMakeRect(lineRect.origin.x, lineRect.origin.y+lineRect.size.height - 1.0, lineRect.size.width, 1.0), NSCompositeSourceOver);
			}
		}
		else if ([[[self file] project] programCounterFile] == [self file]) {
			NSRect lineRect = [[self layoutManager] lineFragmentRectForGlyphAtIndex:[(WCTextStorage *)[self textStorage] safeLineStartIndexForLineNumber:[[[self file] project] programCounterLineNumber]] effectiveRange:NULL];
			
			if (NSIntersectsRect(lineRect, visibleRect) && [self needsToDrawRect:lineRect]) {
				NSColor *pcColor = [[NSUserDefaults standardUserDefaults] colorForKey:kWCPreferencesEditorProgramCounterHighlightColorKey];
				
				[[pcColor colorWithAlphaComponent:0.4] setFill];
				NSRectFillUsingOperation(lineRect, NSCompositeSourceOver);
				[[pcColor colorWithAlphaComponent:0.6] setFill];
				NSRectFillUsingOperation(NSMakeRect(lineRect.origin.x, lineRect.origin.y, lineRect.size.width, 1.0), NSCompositeSourceOver);
				NSRectFillUsingOperation(NSMakeRect(lineRect.origin.x, lineRect.origin.y+lineRect.size.height - 1.0, lineRect.size.width, 1.0), NSCompositeSourceOver);
			}
		}
	}
	
	if (![[NSUserDefaults standardUserDefaults] boolForKey:kWCPreferencesEditorDisplayErrorBadgesKey] &&
		![[NSUserDefaults standardUserDefaults] boolForKey:kWCPreferencesEditorDisplayWarningBadgesKey])
		return;
	else if (![[NSUserDefaults standardUserDefaults] boolForKey:kWCPreferencesEditorErrorLineHighlightKey] &&
			 ![[NSUserDefaults standardUserDefaults] boolForKey:kWCPreferencesEditorWarningLineHighlightKey])
		return;
		
	BOOL displayErrorBadges = [[NSUserDefaults standardUserDefaults] boolForKey:kWCPreferencesEditorDisplayErrorBadgesKey];
	BOOL displayWarningBadges = [[NSUserDefaults standardUserDefaults] boolForKey:kWCPreferencesEditorDisplayWarningBadgesKey];
	BOOL lineHighlightErrors = [[NSUserDefaults standardUserDefaults] boolForKey:kWCPreferencesEditorErrorLineHighlightKey];
	BOOL lineHighlightWarnings = [[NSUserDefaults standardUserDefaults] boolForKey:kWCPreferencesEditorWarningLineHighlightKey];
	NSColor *errorColor = [[NSUserDefaults standardUserDefaults] colorForKey:kWCPreferencesEditorErrorLineHighlightColorKey];
	NSColor *warningColor = [[NSUserDefaults standardUserDefaults] colorForKey:kWCPreferencesEditorWarningLineHighlightColorKey];
	NSMutableIndexSet *linesAlreadyDrawn = [NSMutableIndexSet indexSet];
	
	for (WCBuildMessage *message in [[self file] allBuildMessages]) {
		if ([message messageType] == WCBuildMessageTypeError &&
			displayErrorBadges &&
			lineHighlightErrors &&
			![linesAlreadyDrawn containsIndex:[message lineNumber]]) {
			[linesAlreadyDrawn addIndex:[message lineNumber]];
			
			NSRect lineRect = [[self layoutManager] lineFragmentRectForGlyphAtIndex:[(WCTextStorage *)[self textStorage] safeLineStartIndexForLineNumber:[message lineNumber]] effectiveRange:NULL];
			
			if (NSIntersectsRect(lineRect, visibleRect) && [self needsToDrawRect:lineRect]) {
				[[errorColor colorWithAlphaComponent:0.4] setFill];
				NSRectFillUsingOperation(lineRect, NSCompositeSourceOver);
				[[errorColor colorWithAlphaComponent:0.6] setFill];
				NSRectFillUsingOperation(NSMakeRect(lineRect.origin.x, lineRect.origin.y, lineRect.size.width, 1.0), NSCompositeSourceOver);
				NSRectFillUsingOperation(NSMakeRect(lineRect.origin.x, lineRect.origin.y+lineRect.size.height - 1.0, lineRect.size.width, 1.0), NSCompositeSourceOver);
			}
		}
		else if ([message messageType] == WCBuildMessageTypeWarning &&
				 displayWarningBadges &&
				 lineHighlightWarnings &&
				 ![linesAlreadyDrawn containsIndex:[message lineNumber]]) {
			
			NSRect lineRect = [[self layoutManager] lineFragmentRectForGlyphAtIndex:[(WCTextStorage *)[self textStorage] safeLineStartIndexForLineNumber:[message lineNumber]] effectiveRange:NULL];
			
			if (NSIntersectsRect(lineRect, visibleRect) && [self needsToDrawRect:lineRect]) {
				[[warningColor colorWithAlphaComponent:0.4] setFill];
				NSRectFillUsingOperation(lineRect, NSCompositeSourceOver);
				[[warningColor colorWithAlphaComponent:0.6] setFill];
				NSRectFillUsingOperation(NSMakeRect(lineRect.origin.x, lineRect.origin.y, lineRect.size.width, 1.0), NSCompositeSourceOver);
				NSRectFillUsingOperation(NSMakeRect(lineRect.origin.x, lineRect.origin.y+lineRect.size.height - 1.0, lineRect.size.width, 1.0), NSCompositeSourceOver);
			}
		}
	}
}

- (NSArray *)completionsForPartialWordRange:(NSRange)charRange indexOfSelectedItem:(NSInteger *)index {
	NSArray *symbols = ([[self file] project] == nil)?[[[self file] symbolScanner] symbols]:[[[self file] project] symbols];
	NSMutableArray *csymbols = [NSMutableArray array];
	NSString *prefix = [[self string] substringWithRange:charRange];
	NSString *lowercasePrefix = [prefix lowercaseString];
	BOOL symbolsAreCaseSensitive = [[[[self file] project] activeBuildTarget] symbolsAreCaseSensitive];
	
	for (WCSymbol *symbol in symbols) {
		if (symbolsAreCaseSensitive && [[symbol name] hasPrefix:prefix])
			[csymbols addObject:[symbol name]];
		else if ([[[symbol name] lowercaseString] hasPrefix:lowercasePrefix])
			[csymbols addObject:[symbol name]];
	}
	
	[csymbols sortUsingSelector:@selector(compare:)];
	
	return [[csymbols copy] autorelease];
}

/*
- (void)insertCompletion:(NSString *)word forPartialWordRange:(NSRange)charRange movement:(NSInteger)movement isFinal:(BOOL)flag {
	if (flag && (movement == NSReturnTextMovement || movement == NSTabTextMovement))
		[super insertCompletion:word forPartialWordRange:charRange movement:movement isFinal:flag];
}
 */

- (NSRange)selectionRangeForProposedRange:(NSRange)proposedCharRange granularity:(NSSelectionGranularity)granularity {
	if (granularity != NSSelectByWord || [[self string] length] == proposedCharRange.location || [[NSApp currentEvent] clickCount] != 2)
		return [super selectionRangeForProposedRange:proposedCharRange granularity:granularity];
	
	NSRange range = [self symbolRangeForRange:proposedCharRange];
	
	if (range.location != NSNotFound)
		return range;
	return [super selectionRangeForProposedRange:proposedCharRange granularity:granularity];
}

- (NSMenu *)menu {
	NSMenu *retval = [[[super menu] copy] autorelease];
	
	for (NSMenuItem *item in [retval itemArray])
		[item setKeyEquivalent:@""];
	
	return retval;
}

#pragma mark IBActions
- (IBAction)insertNewline:(id)sender {
	[super insertNewline:sender];
	
	if ([[NSUserDefaults standardUserDefaults] boolForKey:kWCPreferencesEditorAutomaticallyIndentAfterLabelsKey]) {
		RKRegex *regex = [[[RKRegex alloc] initWithRegexString:@"^(?<name>[A-z0-9!?]+:)" options:RKCompileUTF8|RKCompileMultiline] autorelease];
		NSRange lRange = [[self string] lineRangeForRange:NSMakeRange([self selectedRange].location - 1, 0)];
		RKEnumerator *enumerator = [[[RKEnumerator alloc] initWithRegex:regex string:[self string] inRange:lRange] autorelease];
		NSRange mRange = [enumerator nextRangeForCaptureName:@"name"];
		
		if (mRange.location != NSNotFound) {
			[self insertTab:nil];		
			return;
		}
	}
	
	if ([[NSUserDefaults standardUserDefaults] boolForKey:kWCPreferencesEditorAutomaticallyIndentNewLinesKey]) {
		NSString *previousLineWhitespaceString = nil;
		NSScanner *previousLineScanner = [[[NSScanner alloc] initWithString:[[self string] substringWithRange:[[self string] lineRangeForRange:NSMakeRange([self selectedRange].location - 1, 0)]]] autorelease];
		[previousLineScanner setCharactersToBeSkipped:nil];
		
		if ([previousLineScanner scanCharactersFromSet:[NSCharacterSet whitespaceCharacterSet] intoString:&previousLineWhitespaceString])
			[self insertText:previousLineWhitespaceString];
	}
}

- (void)insertTab:(id)sender {
	if ([[NSUserDefaults standardUserDefaults] unsignedIntegerForKey:kWCPreferencesEditorIndentUsingKey] == WCPreferencesEditorIndentUsingSpaces) {
		NSMutableString *spacesString = [NSMutableString string];
		NSInteger numberOfSpacesPerTab = [[NSUserDefaults standardUserDefaults] unsignedIntegerForKey:kWCPreferencesEditorTabWidthKey];
		NSInteger locationOnLine = [self selectedRange].location - [[self string] lineRangeForRange:[self selectedRange]].location;
		if (numberOfSpacesPerTab != 0) {
			NSInteger numberOfSpacesLess = locationOnLine % numberOfSpacesPerTab;
			numberOfSpacesPerTab = numberOfSpacesPerTab - numberOfSpacesLess;
		}
		while (numberOfSpacesPerTab--)
			[spacesString appendString:@" "];
		
		[self insertText:spacesString];
	}
	else
		[super insertTab:sender];
}

- (void)insertText:(id)insertString {
	if ([insertString isEqualToString:@":"] &&
		[self selectedRange].length == 0) {
		
		RKRegex *regex = [[[RKRegex alloc] initWithRegexString:@"^\\s+(?<name>[A-z0-9!?]+)" options:RKCompileUTF8|RKCompileMultiline] autorelease];
		NSRange lRange = [[self string] lineRangeForRange:[self selectedRange]];
		RKEnumerator *enumerator = [[[RKEnumerator alloc] initWithRegex:regex string:[self string] inRange:lRange] autorelease];
		NSRange mRange = [enumerator nextRangeForCaptureName:@"name"];
		
		if (mRange.location == NSNotFound)
			return;
		
		if ([self shouldChangeTextInRange:[enumerator currentRange] replacementString:[[[self string] substringWithRange:mRange] stringByAppendingString:@":"]]) {
			[self replaceCharactersInRange:[enumerator currentRange] withString:[[[self string] substringWithRange:mRange] stringByAppendingString:@":"]];
			[self didChangeText];
		}
	}
	else
		[super insertText:insertString];
}

- (IBAction)performFindPanelAction:(id)sender {
	switch ([sender tag]) {
		case NSFindPanelActionShowFindPanel:
			if ([self findBarViewController] == nil)
				[WCFindBarViewController presentFindBarForTextView:self];
			else {
				[[self window] makeFirstResponder:[[self findBarViewController] searchField]];
				[[self findBarViewController] find:nil];
			}
			break;
		case NSFindPanelActionNext:
			[[self findBarViewController] findNext:nil];
			break;
		case NSFindPanelActionPrevious:
			[[self findBarViewController] findPrevious:nil];
			break;
		case NSFindPanelActionReplaceAll:
			[[self findBarViewController] replaceAll:nil];
			break;
		case NSFindPanelActionReplace:
			[[self findBarViewController] replace:nil];
			break;
		case NSFindPanelActionReplaceAndFind:
			[[self findBarViewController] replaceAndFind:nil];
			break;
		case NSFindPanelActionSetFindString:
			if ([self findBarViewController] == nil)
				[WCFindBarViewController presentFindBarForTextView:self];
			[[self findBarViewController] setFindString:[[self string] substringWithRange:[self selectedRange]]];
			[[self findBarViewController] find:nil];
			break;
		case NSFindPanelActionReplaceAllInSelection:
		case NSFindPanelActionSelectAll:
		case NSFindPanelActionSelectAllInSelection:
		default:
			break;
	}
}

- (void)paste:(id)sender {
	[self pasteAsPlainText:sender];
}

- (void)pasteAsRichText:(id)sender {
	[self pasteAsPlainText:sender];
}

#pragma mark *** Protocol Overrides ***
#pragma mark NSUserInterfaceValidations
- (BOOL)validateUserInterfaceItem:(id <NSValidatedUserInterfaceItem>)item {
	if ([item action] == @selector(jumpToNextBuildMessage:)) {
		if ([[self file] numberOfBuildMessages] == 0)
			return NO;
		return YES;
	}
	else if ([item action] == @selector(jumpToPreviousBuildMessage:)) {
		if ([[self file] numberOfBuildMessages] == 0)
			return NO;
		return YES;
	}
	else if ([item action] == @selector(addBreakpointAtCurrentLine:)) {
		WCBreakpoint *breakpoint = [[self file] breakpointAtLineNumber:[(WCTextStorage *)[self textStorage] lineNumberForCharacterIndex:[self selectedRange].location]];
		
		if (breakpoint == nil) {
			if ([(id <NSObject>)item isKindOfClass:[NSMenuItem class]])
				[(NSMenuItem *)item setTitle:NSLocalizedString(@"Add Breakpoint at Current Line", @"Add Breakpoint at Current Line")];
		}
		else {
			if ([(id <NSObject>)item isKindOfClass:[NSMenuItem class]])
				[(NSMenuItem *)item setTitle:NSLocalizedString(@"Edit Breakpoint at Current Line\u2026", @"Edit Breakpoint at Current Line with ellipsis")];
		}
		return YES;
	}
	else if ([item action] == @selector(revealInSymbolsView:)) {
		if ([[self file] project] == nil)
			return NO;
		return YES;
	}
	return [super validateUserInterfaceItem:item];
}
#pragma mark NSKeyValueObserving
- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context {
	
	if ([(NSString *)context isEqualToString:kWCPreferencesEditorFontKey]) {
		
	}
	else if ([(NSString *)context isEqualToString:kWCPreferencesEditorTextColorKey])
		[self setTextColor:[[NSUserDefaults standardUserDefaults] colorForKey:kWCPreferencesEditorTextColorKey]];
	else if ([(NSString *)context isEqualToString:kWCPreferencesEditorBackgroundColorKey])
		[self setBackgroundColor:[[NSUserDefaults standardUserDefaults] colorForKey:kWCPreferencesEditorBackgroundColorKey]];
	else if ([(NSString *)context isEqualToString:kWCPreferencesEditorErrorLineHighlightKey] ||
			 [(NSString *)context isEqualToString:kWCPreferencesEditorErrorLineHighlightColorKey] ||
			 [(NSString *)context isEqualToString:kWCPreferencesEditorWarningLineHighlightKey] ||
			 [(NSString *)context isEqualToString:kWCPreferencesEditorWarningLineHighlightColorKey] ||
			 [(NSString *)context isEqualToString:kWCPreferencesEditorDisplayErrorBadgesKey] ||
			 [(NSString *)context isEqualToString:kWCPreferencesEditorDisplayWarningBadgesKey])
		[self setNeedsDisplayInRect:[self visibleRect]];
	else if ([(NSString *)context isEqualToString:kWCPreferencesEditorWrapLinesKey])
		[self toggleWrapLines:nil];
	else
		[super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
}
#pragma mark *** Category Overrides ***
- (NSArray *)userDefaultsKeys {
	return [NSArray arrayWithObjects:kWCPreferencesEditorErrorLineHighlightKey,kWCPreferencesEditorErrorLineHighlightColorKey,kWCPreferencesEditorWarningLineHighlightKey,kWCPreferencesEditorWarningLineHighlightColorKey,kWCPreferencesEditorWrapLinesKey,kWCPreferencesEditorDisplayErrorBadgesKey,kWCPreferencesEditorDisplayWarningBadgesKey,kWCPreferencesEditorBackgroundColorKey, nil];
}
#pragma mark *** Public Methods ***
- (void)jumpToSymbol:(WCSymbol *)symbol; {
	WCFileViewController *controller = [[[self file] project] addFileViewControllerForFile:[symbol file] inTabViewContext:[[[self file] project] currentTabViewContext]];
	WCTextView *textView = [controller textView];
	
	if (textView == nil)
		textView = self;
	
#ifdef DEBUG
	NSAssert(textView != nil, @"cannot jump to a symbol without a text view!");
#endif
	
	[textView setSelectedRangeSafely:[symbol symbolRange] scrollRangeToVisible:YES];
}

- (NSString *)symbolStringForRange:(NSRange)range; {
	NSRange fRange = [self symbolRangeForRange:range];
	if (fRange.location != NSNotFound)
		return [[self string] substringWithRange:fRange];
	return nil;
}

- (NSRange)symbolRangeForRange:(NSRange)range; {
	NSString *string = [self string];
	
	if (string == nil || [string length] == 0)
		return WCNotFoundRange;
	
	// search the line string for anything that looks like a symbol name
	RKEnumerator *lineEnum = [[[RKEnumerator alloc] initWithRegex:kWCSyntaxHighlighterSymbolsRegex string:string inRange:[string lineRangeForRange:range]] autorelease];
	
	while ([lineEnum nextRanges] != NULL) {
		NSRangePointer matchRange = [lineEnum currentRanges];
		
		if (NSLocationInRange(range.location, *matchRange))
			return *matchRange;
	}
	return WCNotFoundRange;
}
#pragma mark Accessors
@dynamic file;
- (WCFile *)file {
	return _file;
}
- (void)setFile:(WCFile *)file {
	if (_file == file)
		return;
	
	[_syntaxHighlighter release];
	
	_file = file;
	
	_syntaxHighlighter = [[WCSyntaxHighlighter alloc] initWithTextView:self];
	
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_fileNumberOfBuildMessagesChanged:) name:kWCFileNumberOfErrorMessagesChangedNotification object:_file];
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_fileNumberOfBuildMessagesChanged:) name:kWCFileNumberOfWarningMessagesChangedNotification object:_file];
}
@dynamic currentSymbolString;
- (NSString *)currentSymbolString {
	return [self symbolStringForRange:[self selectedRange]];
}

@synthesize fileViewController=_fileViewController;
@synthesize syntaxHighlighter=_syntaxHighlighter;
@synthesize findBarViewController=_findBarViewController;
#pragma mark IBActions
- (IBAction)jumpToDefinition:(id)sender; {
	NSString *symbolString = [self currentSymbolString];
	
	if (!symbolString) {
		NSBeep();
		return;
	}
	
	NSArray *symbols = ([[self file] project] == nil)?[[[self file] symbolScanner] symbolsForSymbolName:symbolString]:[[[self file] project] symbolsForSymbolName:symbolString];
	
	if (![symbols count]) {
		NSBeep();
		return;
	}
	else if ([symbols count] == 1)
		[self jumpToSymbol:[symbols lastObject]];
	else {
		NSMenu *menu = [[[NSMenu alloc] initWithTitle:@""] autorelease];
		
		[menu setShowsStateColumn:NO];
		[menu setFont:[NSFont menuFontOfSize:11.0]];
		
		for (WCSymbol *symbol in symbols) {
			NSMenuItem *item = [menu addItemWithTitle:[NSString stringWithFormat:@"%@ - %@:%lu",[symbol name],[[symbol file] name],[[[symbol file] textStorage] lineNumberForCharacterIndex:[symbol symbolRange].location]+1] action:@selector(_jumpToSymbolFromMenu:) keyEquivalent:@""];
			[item setImage:[symbol iconForContextualMenu]];
			[item setRepresentedObject:symbol];
		}
		
		NSRect lineRect = [[self layoutManager] lineFragmentRectForGlyphAtIndex:[self selectedRange].location effectiveRange:NULL];
		NSPoint selectedPoint = [[self layoutManager] locationForGlyphAtIndex:[self selectedRange].location];
		
		lineRect.origin.y += lineRect.size.height;
		lineRect.origin.x += selectedPoint.x;
		
		[menu popUpMenuPositioningItem:nil atLocation:lineRect.origin inView:self];
	}
}

- (IBAction)jumpToNextBuildMessage:(id)sender; {
	NSArray *messages = [[self file] allBuildMessagesSortedByLineNumber];
	NSRange range = [self selectedRange];
	
	for (WCBuildMessage *message in messages) {
		NSRange mRange = NSMakeRange([[[self file] textStorage] safeLineStartIndexForLineNumber:[message lineNumber]], 0);
		
		if (mRange.location > range.location) {
			[self setSelectedRange:mRange];
			[self scrollRangeToVisible:mRange];
			return;
		}
	}
	
	NSBeep();
}
- (IBAction)jumpToPreviousBuildMessage:(id)sender; {
	NSArray *messages = [[self file] allBuildMessagesSortedByLineNumber];
	NSRange range = [self selectedRange];
	
	for (WCBuildMessage *message in [messages reverseObjectEnumerator]) {
		NSRange mRange = NSMakeRange([[[self file] textStorage] safeLineStartIndexForLineNumber:[message lineNumber]], 0);
		
		if (mRange.location < range.location) {
			[self setSelectedRange:mRange];
			[self scrollRangeToVisible:mRange];
			return;
		}
	}
	
	NSBeep();
}

- (IBAction)commentOrUncomment:(id)sender; {
	NSRange range = [self selectedRange];
	NSRange lineRange = [[self string] lineRangeForRange:range];
	
	if (!range.length) {
		NSString *string = [[self string] substringWithRange:lineRange];
		
		if ([string rangeOfString:@";" options:NSLiteralSearch].location == NSNotFound) {
			NSString *replacementString = [@";;" stringByAppendingString:string];
			
			if ([self shouldChangeTextInRange:lineRange replacementString:replacementString]) {
				[self replaceCharactersInRange:lineRange withString:replacementString];
				[self setSelectedRange:NSMakeRange(range.location + 2, range.length)];
			}
		}
		else {
			NSMutableString *replacementString = [NSMutableString stringWithString:string];
			
			[replacementString replaceOccurrencesOfString:@";" withString:@"" options:NSLiteralSearch range:NSMakeRange(0, [replacementString length])];
			
			if ([self shouldChangeTextInRange:lineRange replacementString:replacementString]) {
				[self replaceCharactersInRange:lineRange withString:replacementString];
				[self didChangeText];
				[self setSelectedRange:NSMakeRange(range.location-([string length] - [replacementString length]), range.length)];
			}
		}
	}
	else {
		NSString *string = [[self string] substringWithRange:lineRange];
		
		RKRegex *mRegex = [[[RKRegex alloc] initWithRegexString:@"^\\s*;+.*" options:RKCompileUTF8|RKCompileMultiline] autorelease];
		if ([string rangeOfRegex:mRegex].location == NSNotFound) {
			RKRegex *regex = [[[RKRegex alloc] initWithRegexString:@"^(?<first>\\s*)(?<second>.*)" options:RKCompileUTF8|RKCompileMultiline] autorelease];
			NSMutableString *newString = [NSMutableString stringWithString:string];
			[newString match:regex replace:RKReplaceAll withString:@"${first};;${second}"];
			
			if ([self shouldChangeTextInRange:lineRange replacementString:newString]) {
				[self replaceCharactersInRange:lineRange withString:newString];
				[self didChangeText];
				[self setSelectedRange:NSMakeRange(lineRange.location, [newString length])];
			}
		}
		else {
			RKRegex *regex = [[[RKRegex alloc] initWithRegexString:@"^(?<first>\\s*)(?<comment>;+)(?<second>.*)" options:RKCompileUTF8|RKCompileMultiline] autorelease];
			NSMutableString *newString = [NSMutableString stringWithString:string];
			[newString match:regex replace:RKReplaceAll withString:@"${first}${second}"];
			
			if ([self shouldChangeTextInRange:lineRange replacementString:newString]) {
				[self replaceCharactersInRange:lineRange withString:newString];
				[self didChangeText];
				[self setSelectedRange:NSMakeRange(lineRange.location, [newString length])];
			}
		}
	}
}
- (IBAction)blockCommentOrUncomment:(id)sender; {
	NSRange range = [[self string] lineRangeForRange:[self selectedRange]];
	NSString *string = [[self string] substringWithRange:range];
	
	// the selection does not have a matching block comment
	if ([string rangeOfString:@"#comment" options:NSLiteralSearch].location == NSNotFound && [string rangeOfString:@"#endcomment" options:NSLiteralSearch].location == NSNotFound) {
		NSMutableString *newString = [NSMutableString stringWithString:@"#comment\n"];
		[newString appendString:string];
		[newString appendString:@"#endcomment\n"];
		
		if ([self shouldChangeTextInRange:range replacementString:newString]) {
			[self replaceCharactersInRange:range withString:newString];
			[self didChangeText];
			[self setSelectedRange:NSMakeRange(range.location, [newString length])];
		}
	}
	else {
		NSMutableString *newString = [NSMutableString stringWithString:string];
		[newString replaceOccurrencesOfString:@"#comment\n" withString:@"" options:NSLiteralSearch range:NSMakeRange(0, [newString length])];
		[newString replaceOccurrencesOfString:@"#endcomment\n" withString:@"" options:NSLiteralSearch range:NSMakeRange(0, [newString length])];
		[newString replaceOccurrencesOfString:@"#comment" withString:@"" options:NSLiteralSearch range:NSMakeRange(0, [newString length])];
		[newString replaceOccurrencesOfString:@"#endcomment" withString:@"" options:NSLiteralSearch range:NSMakeRange(0, [newString length])];
		
		if ([self shouldChangeTextInRange:range replacementString:newString]) {
			[self replaceCharactersInRange:range withString:newString];
			[self didChangeText];
			[self setSelectedRange:NSMakeRange(range.location, [newString length])];
		}
	}
}

- (IBAction)shiftLeft:(id)sender; {
	NSRange range = [self selectedRange];
	NSRange lineRange = [[self string] lineRangeForRange:range];
	NSString *string = [[self string] substringWithRange:lineRange];
	NSMutableString *newString = [NSMutableString stringWithString:string];
	
	if (!range.length) {
		RKRegex *regex = [[[RKRegex alloc] initWithRegexString:@"^(\t)(.*)" options:RKCompileUTF8|RKCompileNoOptions] autorelease];
		
		[newString match:regex replace:RKReplaceAll withString:@"$2"];
		
		if ([self shouldChangeTextInRange:lineRange replacementString:newString]) {
			[self replaceCharactersInRange:lineRange withString:newString];
			[self didChangeText];
			[self setSelectedRange:NSMakeRange(range.location-1, 0)];
		}
	}
	else {
		RKRegex *regex = [[[RKRegex alloc] initWithRegexString:@"^(\t)(.*)" options:RKCompileUTF8|RKCompileMultiline] autorelease];
		
		[newString match:regex replace:RKReplaceAll withString:@"$2"];
		
		if ([self shouldChangeTextInRange:lineRange replacementString:newString]) {
			[self replaceCharactersInRange:lineRange withString:newString];
			[self didChangeText];
			[self setSelectedRange:NSMakeRange(lineRange.location, [newString length])];
		}
	}
}

- (IBAction)shiftRight:(id)sender; {
	NSRange range = [self selectedRange];
	NSRange lineRange = [[self string] lineRangeForRange:range];
	NSString *string = [[self string] substringWithRange:lineRange];
	
	if (!range.length) {
		NSMutableString *newString = [NSMutableString stringWithString:@"\t"];
		[newString appendString:string];
		
		if ([self shouldChangeTextInRange:lineRange replacementString:newString]) {
			[self replaceCharactersInRange:lineRange withString:newString];
			[self didChangeText];
			[self setSelectedRange:NSMakeRange(range.location+1, 0)];
		}
	}
	else {
		NSMutableString *newString = [NSMutableString stringWithString:string];
		RKRegex *regex = [[[RKRegex alloc] initWithRegexString:@"^(.*)" options:RKCompileUTF8|RKCompileMultiline] autorelease];
		
		[newString match:regex replace:RKReplaceAll withString:@"\t$1"];
		
		if ([self shouldChangeTextInRange:lineRange replacementString:newString]) {
			[self replaceCharactersInRange:lineRange withString:newString];
			[self didChangeText];
			[self setSelectedRange:NSMakeRange(lineRange.location, [newString length])];
		}
	}
}
- (IBAction)useSelectionForFindInProject:(id)sender; {
	if (![[self file] project] || ![self selectedRange].length) {
		NSBeep();
		return;
	}
	
	WCProject *project = [[self file] project];
	
	[project viewSearch:nil];
	[[project findInProjectViewController] setFindString:[[self string] substringWithRange:[self selectedRange]]];
	[[project findInProjectViewController] findInProject:nil];
}
- (IBAction)toggleWrapLines:(id)sender; {
	NSTextView *textView = self;
	NSScrollView *textScrollView = [self enclosingScrollView];
	NSSize contentSize = [textScrollView contentSize];
	if ([[NSUserDefaults standardUserDefaults] boolForKey:kWCPreferencesEditorWrapLinesKey]) {
		[textView setMinSize:contentSize];
		[textScrollView setHasHorizontalScroller:NO];
		[textView setHorizontallyResizable:YES];
		[[textView textContainer] setWidthTracksTextView:YES];
		[[textView textContainer] setContainerSize:NSMakeSize(contentSize.width, CGFLOAT_MAX)];
	}
	else {
		[textView setMinSize:contentSize];
		[textScrollView setHasHorizontalScroller:YES];
		[textView setHorizontallyResizable:YES];
		[[textView textContainer] setContainerSize:NSMakeSize(CGFLOAT_MAX, CGFLOAT_MAX)];
		[[textView textContainer] setWidthTracksTextView:NO];
	}
}
- (IBAction)gotoLine:(id)sender; {
	[WCGotoLineSheetController presentGotoLineSheetForTextView:self];
}

- (IBAction)addBreakpointAtCurrentLine:(id)sender; {
	WCBreakpoint *breakpoint = [[self file] breakpointAtLineNumber:[(WCTextStorage *)[self textStorage] lineNumberForCharacterIndex:[self selectedRange].location]];
	
	if (breakpoint != nil) {
		NSBeep();
		return;
	}
	
	breakpoint = [WCBreakpoint breakpointWithLineNumber:[(WCTextStorage *)[self textStorage] lineNumberForCharacterIndex:[self selectedRange].location] inFile:[self file]];
	
	[[self file] addBreakpoint:breakpoint];
}

- (IBAction)openInSeparateEditor:(id)sender; {
	[[[self file] project] performSelector:@selector(_openSeparateEditorForFile:) withObject:[self file]];
}

- (IBAction)revealInProjectView:(id)sender; {
	[[[self file] project] viewProject:nil];
	
	[(NSTreeController *)[[[[[self file] project] projectFilesOutlineViewController] outlineView] dataSource] setSelectedRepresentedObject:[self file]];
	
	[[self window] makeFirstResponder:[[[[self file] project] projectFilesOutlineViewController] outlineView]];
}
#pragma mark *** Private Methods ***
#pragma mark IBActions
- (IBAction)_jumpToSymbolFromMenu:(id)sender {
	[self jumpToSymbol:[sender representedObject]];
}

#pragma mark Notifications
- (void)_fileNumberOfBuildMessagesChanged:(NSNotification *)note {
	[self setNeedsDisplay:YES];
}

- (void)_selectionDidChange:(NSNotification *)note {
	[[WCTooltipManager sharedTooltipManager] hideTooltip];
}

- (void)_boundsDidChange:(NSNotification *)note {
	[[WCTooltipManager sharedTooltipManager] hideTooltip];
}
@end
