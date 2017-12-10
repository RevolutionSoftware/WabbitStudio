//
//  WECalculatorDocument.m
//  WabbitEmu Beta
//
//  Created by William Towe on 4/25/11.
//  Copyright 2011 Revolution Software. All rights reserved.
//

#import "WECalculatorDocument.h"
#import "WEApplicationDelegate.h"
#import "WCDefines.h"
#import "RSLCDView.h"
#import "NSUserDefaults+WCExtensions.h"
#import "WEPreferencesController.h"
#import "RSCalculator.h"
#import "RSCalculatorSkinView.h"
#import "NSObject+WCExtensions.h"
#import "WEDebuggerWindowController.h"
#import "WETransferSheetController.h"

#import <BWToolkitFramework/BWAnchoredButtonBar.h>


NSString *const kWECalculatorWillCloseNotification = @"kWECalculatorWillCloseNotification";

static NSString *const kWECalculatorErrorDomain = @"kWECalculatorErrorDomain";
static const NSInteger kWECalculatorCreatedMaxCalcs = 1001;
static const NSInteger kWECalculatorRomOrSavestateLoadFailed = 1002;

@interface WECalculatorDocument ()
- (NSString *)_stringForCalculatorModel:(RSCalculatorModel)calculatorModel;
@end

@implementation WECalculatorDocument

+ (NSSet *)keyPathsForValuesAffectingValueForKey:(NSString *)key {
	if ([key isEqualToString:@"statusImage"])
		return [NSSet setWithObjects:@"isDebugging",@"calculator.isRunning", nil];
	return [super keyPathsForValuesAffectingValueForKey:key];
}

- (void)dealloc {
#ifdef DEBUG
	NSLog(@"%@ called in %@",NSStringFromSelector(_cmd),[self className]);
#endif
	[[NSNotificationCenter defaultCenter] removeObserver:self];
	[_statusString release];
	[_FPSString release];
	[_calculator release];
	[super dealloc];
}

- (NSString *)windowNibName {
	return @"WECalculatorDocument";
}

- (void)windowWillClose:(NSNotification *)notification {	
	_isClosing = YES;
	
#ifdef DEBUG
	NSLog(@"%@ called in %@",NSStringFromSelector(_cmd),[self className]);
#endif
	
	[[NSNotificationCenter defaultCenter] postNotificationName:kWECalculatorWillCloseNotification object:self];
	
	[(WEApplicationDelegate *)[[NSApplication sharedApplication] delegate] removeLCDView:[self LCDView]];
	
	[_statusImageView unbind:@"value"];
	[_statusTextField unbind:@"value"];
	[_progressIndicator unbind:@"animate"];
}

- (void)windowControllerDidLoadNib:(NSWindowController *)windowController {
	[super windowControllerDidLoadNib:windowController];
	
	[windowController setShouldCloseDocument:YES];
	
	if ([[windowController window] styleMask] == NSBorderlessWindowMask)
		[[NSApplication sharedApplication] addWindowsItem:[windowController window] title:[self displayName] filename:NO];
	
	[[self calculator] calc]->cpu.pio.lcd->shades = (uint32_t)[[NSUserDefaults standardUserDefaults] unsignedIntegerForKey:kWEPreferencesDisplayLCDShadesKey];
	[[self LCDView] setCalculator:[self calculator]];
	//[[self LCDView] setIsWidescreen:([[self calculator] calc]->model == TI_85 || [[self calculator] calc]->model == TI_86)];
	[self updateStatusString];
	//[self resetDisplaySize:nil];
	[(WEApplicationDelegate *)[[NSApplication sharedApplication] delegate] addLCDView:[self LCDView]];
	
	[_buttonBar setIsAtBottom:YES];
	[_buttonBar setIsResizable:NO];
	[[_statusTextField cell] setBackgroundStyle:NSBackgroundStyleLight];
	
	[_LCDView adjustSkinViewRespectingUserDefaults:YES];
	[self setHasSkin:[_LCDView hasSkin]];
	[self setIsBorderlessSkin:[_LCDView skinIsBorderless]];
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context {
	if ([(NSString *)context isEqualToString:kRSCalculatorSkinViewUseSkinsKey])
		return;
	else
		[super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
}

- (BOOL)readFromURL:(NSURL *)absoluteURL ofType:(NSString *)typeName error:(NSError **)outError {
	RSCalculator *calculator = [RSCalculator calculatorWithOwner:self breakpointSelector:@selector(handleBreakpoint)];
	
	if (calculator == nil) {
		if (outError != NULL)
			*outError = [NSError errorWithDomain:kWECalculatorErrorDomain code:kWECalculatorCreatedMaxCalcs userInfo:[NSDictionary dictionaryWithObjectsAndKeys:NSLocalizedString(@"The maximum number of calculators have already been created.", @"The maximum number of calculators have already been created"),NSLocalizedDescriptionKey,NSLocalizedString(@"The maximum number of calculators have already been created.", @"The maximum number of calculators have already been created"),NSLocalizedFailureReasonErrorKey,NSLocalizedString(@"Please close at least one open calculator and try again.", @"Please close at least one open calculator and try again."),NSLocalizedRecoverySuggestionErrorKey, nil]];
		return NO;
	}
	
	_calculator = [calculator retain];
	
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_calculatorModelDidChange:) name:kRSCalculatorModelDidChangeNotification object:[self calculator]];
	
	return [[self calculator] loadRomOrSavestate:[absoluteURL path] error:outError];
}

- (BOOL)validateUserInterfaceItem:(id<NSValidatedUserInterfaceItem>)userInterfaceItem {
	if ([userInterfaceItem action] == @selector(loadRom:)) {
		if ([self isDebugging])
			return NO;
		return YES;
	}
	else if ([userInterfaceItem action] == @selector(transferFiles:)) {
		if ([self isDebugging])
			return NO;
		return YES;
	}
	return [super validateUserInterfaceItem:userInterfaceItem];
}

- (void)handleBreakpoint {
#ifdef DEBUG
	NSLog(@"%@ called in %@",NSStringFromSelector(_cmd),[self className]);
#endif
	[self showDebugger:nil];
}

- (void)updateFPSString; {
	if ([[self calculator] isRunning])
		[self setFPSString:[NSString stringWithFormat:NSLocalizedString(@"FPS: %.2f", @"FPS format string"),[[self calculator] calc]->cpu.pio.lcd->ufps]];
}

- (void)updateStatusString {
	if ([[self calculator] isActive])
		[self setStatusString:[self _stringForCalculatorModel:[[self calculator] model]]];
}

@synthesize LCDView=_LCDView;
@synthesize calculator=_calculator;
@synthesize statusString=_statusString;
@synthesize FPSString=_FPSString;
@dynamic statusImage;
- (NSImage *)statusImage {
	if ([self isDebugging])
		return [NSImage imageNamed:NSImageNameStatusUnavailable];
	else if ([[self calculator] isRunning])
		return [NSImage imageNamed:NSImageNameStatusAvailable];
	else
		return [NSImage imageNamed:NSImageNameStatusPartiallyAvailable];
}
@dynamic calculatorWindow;
- (NSWindow *)calculatorWindow {
	return [self windowForSheet];
}
@synthesize isDebugging=_isDebugging;
@synthesize hasSkin=_hasSkin;
@synthesize isBorderlessSkin=_isBorderlessSkin;
@dynamic debuggerWindowController;
- (WEDebuggerWindowController *)debuggerWindowController {
	WEDebuggerWindowController *mController = nil;
	for (id controller in [self windowControllers]) {
		if ([controller isKindOfClass:[WEDebuggerWindowController class]]) {
			mController = controller;
			break;
		}
	}
	
	if (mController == nil) {
		mController = [[[WEDebuggerWindowController alloc] init] autorelease];
		[self addWindowController:mController];
	}
	
	return mController;
}

- (IBAction)loadRom:(id)sender; {
	NSOpenPanel *panel = [NSOpenPanel openPanel];
	
	[panel setAllowedFileTypes:[NSArray arrayWithObjects:kWECalculatorRomUTI,kWECalculatorSavestateUTI, nil]];
	[panel setPrompt:NS_LOCALIZED_STRING_LOAD];
	
	[panel beginSheetModalForWindow:[self windowForSheet] completionHandler:^(NSInteger result) {
		[panel orderOut:nil];
		if (result != NSFileHandlingPanelOKButton)
			return;
		
		NSError *error;
		if (![[self calculator] loadRomOrSavestate:[[[panel URLs] lastObject] path] error:&error]) {
			if (error != NULL)
				[self presentError:error];
			return;
		}
		
		[self updateStatusString];
		
		if ([self windowForSheet] != nil) {
			[self setFileURL:[[panel URLs] lastObject]];
			[[NSDocumentController sharedDocumentController] noteNewRecentDocumentURL:[[panel URLs] lastObject]];
		}
	}];
}

- (IBAction)transferFiles:(id)sender; {
	NSOpenPanel *panel = [NSOpenPanel openPanel];
	
	[panel setAllowedFileTypes:[NSArray arrayWithObjects:kWECalculatorProgramUTI, nil]];
	[panel setPrompt:NSLocalizedString(@"Transfer", @"transfer files open panel prompt")];
	
	[panel beginSheetModalForWindow:[self windowForSheet] completionHandler:^(NSInteger result) {
		[panel orderOut:nil];
		if (result != NSFileHandlingPanelOKButton)
			return;
		
		[WETransferSheetController transferFiles:[panel filenames] toCalculator:[self calculator]];
	}];
}

- (IBAction)saveStateAs:(id)sender; {
	NSSavePanel *panel = [NSSavePanel savePanel];
	
	[panel setRequiredFileType:kWECalculatorSavestateUTI];
	[panel setMessage:NSLocalizedString(@"Choose a name and location for your save state.", @"save state as save panel message")];
	[panel setCanCreateDirectories:YES];
	[panel setNameFieldStringValue:NSLocalizedString(@"save_state", @"save state as save panel name field string value")];
	
	[panel beginSheetModalForWindow:[self windowForSheet] completionHandler:^(NSInteger result) {
		if (result != NSFileHandlingPanelOKButton)
			return;
		
		SAVESTATE_t *savestate = SaveSlot([[self calculator] calc]);
		
		if (savestate == NULL) {
			NSLog(@"savestate was NULL!");
			return;
		}
		
		WriteSave([[[panel URL] path] fileSystemRepresentation], savestate, 0);
	}];
}

- (IBAction)reloadCurrentRomOrSavestate:(id)sender; {
	[[self calculator] loadRomOrSavestate:[[self fileURL] path] error:NULL];
}
- (IBAction)resetCalculator:(id)sender; {
	[[self calculator] setIsRunning:NO];
	calc_reset([[self calculator] calc]);
	calc_turn_on([[self calculator] calc]);
	[[self calculator] setIsRunning:YES];
}

- (IBAction)toggleLCDSize:(id)sender; {
	if ([self hasSkin])
		return;
	
	CGFloat bottomBorderHeight = [[self windowForSheet] contentBorderThicknessForEdge:NSMinYEdge];
	NSSize lcdSize = NSZeroSize;
	CGFloat baseWidth = ([[self LCDView] isWidescreen])?(kLCDWidescreenWidth*2):(kLCDWidth*2);
	CGFloat baseHeight = (kLCDHeight*2);
	
	switch ([sender tag]) {
		case 0:
			lcdSize = NSMakeSize(baseWidth, baseHeight);
			break;
		case 1:
			lcdSize = NSMakeSize(baseWidth*2, baseHeight*2);
			break;
		case 2:
			lcdSize = NSMakeSize(baseWidth*4, baseHeight*4);
			break;
		default:
			break;
	}	
	
	NSRect currentRect = [[self windowForSheet] frame];
	NSRect newRect = [[self windowForSheet] frameRectForContentRect:NSMakeRect(NSMinX(currentRect), NSMinY(currentRect), lcdSize.width, lcdSize.height+bottomBorderHeight)];
	
	if (NSHeight(currentRect) < NSHeight(newRect))
		newRect.origin.y -= NSHeight(newRect) - NSHeight(currentRect);
	else if (NSHeight(currentRect) > NSHeight(newRect))
		newRect.origin.y += NSHeight(currentRect) - NSHeight(newRect);
	else
		return;
	
	[[self windowForSheet] setFrame:newRect display:YES animate:YES];
}

- (IBAction)toggleEmulation:(id)sender; {
	[[self calculator] setIsRunning:![[self calculator] isRunning]];
}

- (IBAction)showDebugger:(id)sender; {
	[[self calculator] setIsRunning:NO];
	[[self calculator] setIsDebugging:YES];
	[self setIsDebugging:YES];
	[(WEApplicationDelegate *)[[NSApplication sharedApplication] delegate] removeLCDView:[self LCDView]];
	[[self debuggerWindowController] showWindow:nil];
}

- (NSString *)_stringForCalculatorModel:(RSCalculatorModel)calculatorModel; {
	switch (calculatorModel) {
		case RSCalculatorModelTI73:
			return NSLocalizedString(@"TI-73", @"TI-73");
		case RSCalculatorModelTI81:
			return NSLocalizedString(@"TI-81", @"TI-81");
		case RSCalculatorModelTI82:
			return NSLocalizedString(@"TI-82", @"TI-82");
		case RSCalculatorModelTI83:
			return NSLocalizedString(@"TI-83", @"TI-83");
		case RSCalculatorModelTI83P:
			return NSLocalizedString(@"TI-83+", @"TI-83+");
		case RSCalculatorModelTI83PSE:
			return NSLocalizedString(@"TI-83+SE", @"TI-83+SE");
		case RSCalculatorModelTI84P:
			return NSLocalizedString(@"TI-84+", @"TI-84+");
		case RSCalculatorModelTI84PSE:
			return NSLocalizedString(@"TI-84+SE", @"TI-84+SE");
		case RSCalculatorModelTI85:
			return NSLocalizedString(@"TI-85", @"TI-85");
		case RSCalculatorModelTI86:
			return NSLocalizedString(@"TI-86", @"TI-86");
		default:
			return nil;
	}
}

- (void)_calculatorModelDidChange:(NSNotification *)note {
	[self updateFPSString];
	//[_LCDView setIsWidescreen:([[self calculator] model] == RSCalculatorModelTI85 || [[self calculator] model] == RSCalculatorModelTI86)];
	//[self toggleSkinView:nil];
}

@end
