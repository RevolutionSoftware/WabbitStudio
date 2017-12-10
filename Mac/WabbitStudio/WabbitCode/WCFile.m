//
//  WCFile.m
//  WabbitStudio
//
//  Created by William Towe on 3/17/11.
//  Copyright 2011 Revolution Software. All rights reserved.
//

#import "WCFile.h"
#import "NSFileManager+WCExtensions.h"
#import "WCTextStorage.h"
#import "NSImage+WCExtensions.h"
#import "WCSymbolScanner.h"
#import "WCProject.h"
#import "WCPreferencesController.h"
#import "NSUserDefaults+WCExtensions.h"
#import "WCBuildMessage.h"
#import "WCAlias.h"
#import "NSString+WCExtensions.h"
#import "WCBreakpoint.h"
#import "WCDefines.h"


NSString *const kWCFileAssemblyUTI = @"org.revsoft.wabbitcode.assembly";
NSString *const kWCFileIncludeUTI = @"org.revsoft.wabbitcode.include";
NSString *const kWCFilePanicCodaImportedUTI = @"com.panic.coda.active-server-include-file";

NSString *const kWCFileHasUnsavedChangesNotification = @"kWCFileHasUnsavedChangesNotification";

NSString *const kWCFileNumberOfErrorMessagesChangedNotification = @"kWCFileNumberOfErrorMessagesChangedNotification";
NSString *const kWCFileNumberOfWarningMessagesChangedNotification = @"kWCFileNumberOfWarningMessagesChangedNotification";

NSString *const kWCFileDidAddBreakpointNotification = @"kWCFileDidAddBreakpointNotification";
NSString *const kWCFileDidRemoveBreakpointNotification = @"kWCFileDidRemoveBreakpointNotification";
NSString *const kWCFileBreakpointKey = @"kWCFileBreakpointKey";

NSString *const kWCFileNameDidChangeNotification = @"kWCFileNameDidChangeNotification";

static NSMutableDictionary *_UTIsToUnsavedIcons = nil;

@interface WCFile (Private)
- (void)_setupTextStorageAndSymbolScanner;
- (void)_setTabWidth;
@end

@implementation WCFile
#pragma mark *** Subclass Overrides ***
+ (void)initialize {
	if ([WCFile class] != self)
		return;
	
	_UTIsToUnsavedIcons = [[NSMutableDictionary alloc] init];
}

- (NSString *)description {
	return [NSString stringWithFormat:@"class: %@ name: %@",[self className],[self name]];
}

- (void)dealloc {
	/*
#ifdef DEBUG
	NSLog(@"%@ called in %@",NSStringFromSelector(_cmd),[self className]);
#endif
	 */
	
	[[NSNotificationCenter defaultCenter] removeObserver:self];
	[_lineNumbersToWarningMessages release];
	[_lineNumbersToErrorMessages release];
	[_symbolScanner release];
	[_textStorage release];
	[_undoManager release];
	[_textViewForFindInProjectReplace release];
	[_UUID release];
	[_alias release];
    [super dealloc];
}

- (BOOL)isLeaf {
	if ([self isDirectory])
		return NO;
	return YES;
}
#pragma mark *** Protocol Overrides ***
#pragma mark NSKeyValueObserving
+ (NSSet *)keyPathsForValuesAffectingValueForKey:(NSString *)key {
	if ([key isEqualToString:@"hasUnsavedChanges"])
		return [[super keyPathsForValuesAffectingValueForKey:key] setByAddingObject:@"changeCount"];
	else if ([key isEqualToString:@"icon"])
		return [[super keyPathsForValuesAffectingValueForKey:key] setByAddingObject:@"hasUnsavedChanges"];
	else if ([key isEqualToString:@"isEdited"])
		return [[super keyPathsForValuesAffectingValueForKey:key] setByAddingObject:@"hasUnsavedChanges"];
	else if ([key isEqualToString:@"name"])
		return [[super keyPathsForValuesAffectingValueForKey:key] setByAddingObjectsFromArray:[NSArray arrayWithObjects:@"URL",@"bookmarkData", nil]];
	return [super keyPathsForValuesAffectingValueForKey:key];
}
#pragma mark NSCoding
- (void)encodeWithCoder:(NSCoder *)coder {
	[coder encodeObject:[self alias] forKey:@"alias"];
	[coder encodeObject:[self UUID] forKey:@"UUID"];
	[coder encodeObject:[self allBreakpoints] forKey:@"breakpoints"];
	[super encodeWithCoder:coder];
}
- (id)initWithCoder:(NSCoder *)coder {
	if (!(self = [super initWithCoder:coder]))
		return nil;
	
	_alias = [[coder decodeObjectForKey:@"alias"] retain];
	_UUID = [[coder decodeObjectForKey:@"UUID"] retain];
	
	NSArray *breakpoints = [coder decodeObjectForKey:@"breakpoints"];
	if ([breakpoints count]) {
		_lineNumbersToBreakpoints = [[NSMutableDictionary alloc] initWithCapacity:[breakpoints count]];
		
		for (WCBreakpoint *bp in breakpoints) {
			[bp setFile:self];
			[_lineNumbersToBreakpoints setObject:bp forKey:[NSNumber numberWithUnsignedInteger:[bp lineNumber]]];
		}
	}
	
	return self;
}
#pragma mark NSCopying
- (id)copyWithZone:(NSZone *)zone  {
	WCFile *copy = [super copyWithZone:zone];
	
	copy->_UUID = [_UUID retain];
	copy->_alias = [_alias retain];
	copy->_textStorage = [_textStorage retain];
	copy->_undoManager = [_undoManager retain];
	copy->_textViewForFindInProjectReplace = [_textViewForFindInProjectReplace retain];
	copy->_encoding = _encoding;
	copy->_symbolScanner = [_symbolScanner retain];
	copy->_project = _project;
	copy->_lineNumbersToBreakpoints = [_lineNumbersToBreakpoints retain];
	copy->_lineNumbersToErrorMessages = [_lineNumbersToErrorMessages retain];
	copy->_lineNumbersToWarningMessages = [_lineNumbersToWarningMessages retain];
	
	return copy;
}
#pragma mark NSMutableCopying
- (id)mutableCopyWithZone:(NSZone *)zone {
	WCFile *copy = [super mutableCopyWithZone:zone];
	
	copy->_UUID = [_UUID retain];
	copy->_alias = [[WCAlias alloc] initWithURL:[self URL]];
	copy->_encoding = _encoding;
	copy->_project = _project;
	
	return copy;
}
#pragma mark NSPlistRepresentationProtocol
- (NSDictionary *)plistRepresentation {
	NSMutableDictionary *dict = [NSMutableDictionary dictionaryWithDictionary:[super plistRepresentation]];
	
	[dict setObject:[self UUID] forKey:@"UUID"];
	[dict setObject:[[self alias] plistRepresentation] forKey:@"alias"];
	[dict setObject:[[_lineNumbersToBreakpoints allValues] valueForKey:@"plistRepresentation"] forKey:@"breakpoints"];
	
	return [[dict copy] autorelease];
}
#pragma mark *** Public Methods ***
- (BOOL)saveFile:(NSError **)outError; {
	NSString *string = [[self textStorage] string];
	
	if (![string writeToURL:[self URL] atomically:YES encoding:_encoding error:outError])
		return NO;
	
	[self setChangeCount:0];
	
	return YES;
}
- (BOOL)resetFile:(NSError **)outError; {
	NSString *string = [[[NSString alloc] initWithContentsOfURL:[self URL] usedEncoding:&_encoding error:NULL] autorelease];
	
	[[self textStorage] replaceCharactersInRange:NSMakeRange(0, [[[self textStorage] string] length]) withString:string];
	
	[self setChangeCount:0];
	
	return YES;
}
#pragma mark Creation
+ (id)fileWithURL:(NSURL *)url; {
	return [[[[self class] alloc] initWithURL:url] autorelease];
}
- (id)initWithURL:(NSURL *)url; {
	return [self initWithURL:url name:nil];
}
+ (id)fileWithURL:(NSURL *)url name:(NSString *)name; {
	return [[[[self class] alloc] initWithURL:url name:name] autorelease];
}
- (id)initWithURL:(NSURL *)url name:(NSString *)name; {
	if (!(self = [super initWithName:name]))
		return nil;
	
	_alias = [[WCAlias alloc] initWithURL:url];
	_UUID = [[NSString UUIDString] retain];
	
	return self;
}
#pragma mark Errors & Warnings
- (void)addErrorMessage:(WCBuildMessage *)error; {
	if (_lineNumbersToErrorMessages == nil)
		_lineNumbersToErrorMessages = [[NSMutableDictionary alloc] init];
	
	NSMutableArray *errors = [_lineNumbersToErrorMessages objectForKey:[NSNumber numberWithUnsignedInteger:[error lineNumber]]];
	
	if (errors == nil) {
		errors = [NSMutableArray arrayWithCapacity:1];
		[_lineNumbersToErrorMessages setObject:errors forKey:[NSNumber numberWithUnsignedInteger:[error lineNumber]]];
	}
	
	[errors addObject:error];
	
	[[NSNotificationCenter defaultCenter] postNotificationName:kWCFileNumberOfErrorMessagesChangedNotification object:self];
}
- (void)addWarningMessage:(WCBuildMessage *)warning; {
	if (_lineNumbersToWarningMessages == nil)
		_lineNumbersToWarningMessages = [[NSMutableDictionary alloc] init];
	
	NSMutableArray *warnings = [_lineNumbersToWarningMessages objectForKey:[NSNumber numberWithUnsignedInteger:[warning lineNumber]]];
	
	if (warnings == nil) {
		warnings = [NSMutableArray arrayWithCapacity:1];
		[_lineNumbersToWarningMessages setObject:warnings forKey:[NSNumber numberWithUnsignedInteger:[warning lineNumber]]];
	}
	
	[warnings addObject:warning];
	
	[[NSNotificationCenter defaultCenter] postNotificationName:kWCFileNumberOfWarningMessagesChangedNotification object:self];
}
- (NSArray *)errorMessagesAtLineNumber:(NSUInteger)lineNumber; {
	return [_lineNumbersToErrorMessages objectForKey:[NSNumber numberWithUnsignedInteger:lineNumber]];
}
- (NSArray *)warningMessagesAtLineNumber:(NSUInteger)lineNumber; {
	return [_lineNumbersToWarningMessages objectForKey:[NSNumber numberWithUnsignedInteger:lineNumber]];
}
- (void)removeAllErrorMessages; {
	[_lineNumbersToErrorMessages removeAllObjects];
	
	[[NSNotificationCenter defaultCenter] postNotificationName:kWCFileNumberOfErrorMessagesChangedNotification object:self];
}
- (void)removeAllWarningMessages; {
	[_lineNumbersToWarningMessages removeAllObjects];
	
	[[NSNotificationCenter defaultCenter] postNotificationName:kWCFileNumberOfWarningMessagesChangedNotification object:self];
}
- (void)removeAllBuildMessages; {
	[self removeAllErrorMessages];
	[self removeAllWarningMessages];
}
- (NSArray *)allErrorMessages; {
	NSMutableArray *retval = [NSMutableArray array];
	
	for (NSArray *errors in [_lineNumbersToErrorMessages allValues])
		[retval addObjectsFromArray:errors];
	
	return [[retval copy] autorelease];
}
- (NSArray *)allWarningMessages; {
	NSMutableArray *retval = [NSMutableArray array];
	
	for (NSArray *warnings in [_lineNumbersToWarningMessages allValues])
		[retval addObjectsFromArray:warnings];
	
	return [[retval copy] autorelease];
}
- (NSArray *)allBuildMessages {
	NSMutableArray *messages = [NSMutableArray array];
	
	[messages addObjectsFromArray:[self allErrorMessages]];
	[messages addObjectsFromArray:[self allWarningMessages]];
	
	return [[messages copy] autorelease];
}
- (NSArray *)allBuildMessagesSortedByLineNumber; {
	static NSArray *sortDescriptors = nil;
	if (!sortDescriptors)
		sortDescriptors = [[NSArray alloc] initWithObjects:[[[NSSortDescriptor alloc] initWithKey:@"lineNumber" ascending:YES selector:@selector(compare:)] autorelease], nil];
	
	NSMutableArray *retval = [NSMutableArray array];
	
	[retval addObjectsFromArray:[self allErrorMessages]];
	[retval addObjectsFromArray:[self allWarningMessages]];
	
	[retval sortUsingDescriptors:sortDescriptors];
	
	return [[retval copy] autorelease];
}

- (NSUInteger)numberOfBuildMessages; {
	return ([[self allErrorMessages] count]+[[self allWarningMessages] count]);
}
#pragma mark Breakpoints
- (void)addBreakpoint:(WCBreakpoint *)breakpoint; {
	if (_lineNumbersToBreakpoints == nil)
		_lineNumbersToBreakpoints = [[NSMutableDictionary alloc] init];
	
	[_lineNumbersToBreakpoints setObject:breakpoint forKey:[NSNumber numberWithUnsignedInteger:[breakpoint lineNumber]]];
	
	[[NSNotificationCenter defaultCenter] postNotificationName:kWCFileDidAddBreakpointNotification object:self userInfo:[NSDictionary dictionaryWithObjectsAndKeys:breakpoint,kWCFileBreakpointKey, nil]];
}
- (void)removeBreakpoint:(WCBreakpoint *)breakpoint; {
	// this protects against the case where the breakpoints view has not been shown yet
	// we must retain autorelease so 'breakpoint' is still valid when we construct the userInfo dictionary
	[[breakpoint retain] autorelease];
	
	[_lineNumbersToBreakpoints removeObjectForKey:[NSNumber numberWithUnsignedInteger:[breakpoint lineNumber]]];
	
	[[NSNotificationCenter defaultCenter] postNotificationName:kWCFileDidRemoveBreakpointNotification object:self userInfo:[NSDictionary dictionaryWithObjectsAndKeys:breakpoint,kWCFileBreakpointKey, nil]];
}
- (WCBreakpoint *)breakpointAtLineNumber:(NSUInteger)lineNumber; {
	return [_lineNumbersToBreakpoints objectForKey:[NSNumber numberWithUnsignedInteger:lineNumber]];
}
- (NSArray *)allBreakpoints; {
	return [_lineNumbersToBreakpoints allValues];
}
- (NSArray *)allBreakpointsSortedByLineNumber; {
	static NSArray *sortDescriptors = nil;
	if (!sortDescriptors)
		sortDescriptors = [[NSArray alloc] initWithObjects:[[[NSSortDescriptor alloc] initWithKey:@"lineNumber" ascending:YES selector:@selector(compare:)] autorelease],nil];
	
	NSMutableArray *retval = [NSMutableArray arrayWithArray:[self allBreakpoints]];
	
	[retval sortUsingDescriptors:sortDescriptors];
	
	return [[retval copy] autorelease];
}
#pragma mark Accessors
- (NSString *)name {
	if ([super name] && [self isDirectory])
		return [super name];
	return [[[self alias] absolutePathForDisplay] lastPathComponent];
}
- (void)setName:(NSString *)name {
	// if it's a group, just rename normally
	if ([self isDirectory]) {
		[super setName:name];
		return;
	}
	
	// otherwise we need to actually rename the represented file
	// check to make sure the name is actually different
	if ([name isEqualToString:[self name]])
		return;
	
	// get our unique file path for renaming
	NSString *renamePath = [[NSFileManager defaultManager] uniqueFilePathForPath:[[[self directoryURL] path] stringByAppendingPathComponent:name]];
	
	// rename by moving
	if (![[NSFileManager defaultManager] moveItemAtPath:[self absolutePath] toPath:renamePath error:NULL])
		return;
	
	// post our notification, this is mainly for the tabs if the given file is open
	[[NSNotificationCenter defaultCenter] postNotificationName:kWCFileNameDidChangeNotification object:self];
}

@synthesize alias=_alias;
@dynamic URL;
- (NSURL *)URL {
	return [[self alias] URL];
}
- (void)setURL:(NSURL *)URL {
	if (!_alias)
		_alias = [[WCAlias alloc] initWithURL:URL];
	else
		[[self alias] setURL:URL];
	
#ifdef DEBUG
    NSAssert(_alias != nil, @"alias cannot be nil!");
#endif
}

- (NSImage *)icon {
	if ([[NSFileManager defaultManager] directoryExistsAtURL:[self URL]] &&
		![[NSWorkspace sharedWorkspace] isFilePackageAtPath:[[self URL] path]])
		return [NSImage imageNamed:@"Group16x16"];
	
	NSImage *icon = [[NSWorkspace sharedWorkspace] iconForFile:[self.URL path]];
	
	if ([self hasUnsavedChanges]) {
		NSImage *unsavedIcon = [_UTIsToUnsavedIcons objectForKey:[self UTI]];
		
		if (!unsavedIcon) {
			unsavedIcon = [icon unsavedIconFromImage];
			
			[_UTIsToUnsavedIcons setObject:unsavedIcon forKey:[self UTI]];
		}
		
		icon = unsavedIcon;
	}
	
	[icon setSize:WCSmallSize];
	
	return icon;
}

@dynamic symbolScanner;
- (WCSymbolScanner *)symbolScanner {
	[self _setupTextStorageAndSymbolScanner];
	
	return _symbolScanner;
}

@dynamic isDirectory;
- (BOOL)isDirectory {
	return [[self alias] isDirectory];
}

@dynamic absolutePath;
- (NSString *)absolutePath {
	return [[self URL] path];
}

@dynamic absolutePathForDisplay;
- (NSString *)absolutePathForDisplay {
	return [[self absolutePath] stringByReplacingPercentEscapesUsingEncoding:[self encoding]];
}

@dynamic directoryURL;
- (NSURL *)directoryURL {
	return ([self isDirectory])?[self URL]:[[self URL] URLByDeletingLastPathComponent];
}

@dynamic textStorage;
- (WCTextStorage *)textStorage {
	[self _setupTextStorageAndSymbolScanner];
	
	return _textStorage;
}

@dynamic undoManager;
- (NSUndoManager *)undoManager {
	if (!_undoManager) {
		_undoManager = [[NSUndoManager alloc] init];
		
		[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_didOpenUndoGroup:) name:NSUndoManagerDidOpenUndoGroupNotification object:_undoManager];
		[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_didUndoChange:) name:NSUndoManagerDidUndoChangeNotification object:_undoManager];
		[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_didRedoChange:) name:NSUndoManagerDidRedoChangeNotification object:_undoManager];
	}
	return _undoManager;
}

@dynamic hasUnsavedChanges;
- (BOOL)hasUnsavedChanges {
	return ([self changeCount] == 0)?NO:YES;
}
- (BOOL)isEdited {
	return [self hasUnsavedChanges];
}
@dynamic UTI;
- (NSString *)UTI {
	return [[self alias] UTI];
}

@dynamic project;
- (WCProject *)project {
	if (_project != nil)
		return _project;
	return [[self parentNode] project];
}
- (void)setProject:(WCProject *)project {
	_project = project;
}
@dynamic canEditName;
- (BOOL)canEditName {
	return [[self URL] checkResourceIsReachableAndReturnError:NULL];
}
@synthesize UUID=_UUID;
@dynamic textViewForFindInProjectReplace;
- (NSTextView *)textViewForFindInProjectReplace {
	if (!_textViewForFindInProjectReplace) {
		_textViewForFindInProjectReplace = [[NSTextView alloc] initWithFrame:NSMakeRect(0.0, 0.0, 1000.0, FLT_MAX)];
		
		[_textViewForFindInProjectReplace setUsesFindPanel:NO];
		[_textViewForFindInProjectReplace setAllowsUndo:YES];
		[_textViewForFindInProjectReplace setUsesFontPanel:NO];
		[_textViewForFindInProjectReplace setUsesRuler:NO];
		[_textViewForFindInProjectReplace setSmartInsertDeleteEnabled:NO];
		[_textViewForFindInProjectReplace setGrammarCheckingEnabled:NO];
		[_textViewForFindInProjectReplace setContinuousSpellCheckingEnabled:NO];
		[_textViewForFindInProjectReplace setDelegate:self];
		
		[[_textViewForFindInProjectReplace layoutManager] replaceTextStorage:[self textStorage]];
	}
	return _textViewForFindInProjectReplace;
}
- (NSUndoManager *)undoManagerForTextView:(NSTextView *)view {
	if (view == [self textViewForFindInProjectReplace])
		return [self undoManager];
	return nil;
}
@synthesize encoding=_encoding;
@dynamic changeCount;
- (NSInteger)changeCount; {
	return _changeCount;
}
- (void)setChangeCount:(NSInteger)value; {
	if (_changeCount == value)
		return;
	
	_changeCount = value;
	
	if (_changeCount == 0 || _changeCount == 1 || _changeCount == -1)
		[[NSNotificationCenter defaultCenter] postNotificationName:kWCFileHasUnsavedChangesNotification object:self];
}
@dynamic isTextFile;
- (BOOL)isTextFile {
	return ([[self UTI] isEqualToString:kWCFileIncludeUTI] ||
			[[self UTI] isEqualToString:kWCFileAssemblyUTI] ||
			[[self UTI] isEqualToString:kWCFilePanicCodaImportedUTI]);
}
#pragma mark *** Private Methods ***
- (void)_setupTextStorageAndSymbolScanner; {
	if (!_textStorage) {
		_encoding = NSUTF8StringEncoding;
		NSString *string = nil;
		if ([self URL]) {
			NSStringEncoding encoding = [[NSUserDefaults standardUserDefaults] unsignedIntegerForKey:kWCPreferencesFilesTextEncodingKey];
			string = [[[NSString alloc] initWithContentsOfURL:[self URL] encoding:encoding error:NULL] autorelease];
			
			if (string == nil)
				string = [[[NSString alloc] initWithContentsOfURL:[self URL] usedEncoding:&encoding error:NULL] autorelease];
			
			_encoding = encoding;
		}
		
		if (string == nil)
			string = NSLocalizedString(@"Windoze is stups. That is all.", @"blank file default contents");
		
		_textStorage = [[WCTextStorage alloc] initWithString:string];
		
		_symbolScanner = [[WCSymbolScanner alloc] initWithFile:self];
	}
}

- (void)_setTabWidth {
	// Set the width of every tab by first checking the size of the tab in spaces in the current font and then remove all tabs that sets automatically and then set the default tab stop distance
	NSMutableString *sizeString = [NSMutableString string];
	NSInteger numberOfSpaces = [[NSUserDefaults standardUserDefaults] unsignedIntegerForKey:kWCPreferencesEditorTabWidthKey];
	while (numberOfSpaces--) {
		[sizeString appendString:@" "];
	}
	NSDictionary *sizeAttribute = [NSDictionary dictionaryWithObjectsAndKeys:[[NSUserDefaults standardUserDefaults] fontForKey:kWCPreferencesEditorFontKey], NSFontAttributeName, nil];
	CGFloat sizeOfTab = [sizeString sizeWithAttributes:sizeAttribute].width;
	
	NSMutableParagraphStyle *style = [[[NSParagraphStyle defaultParagraphStyle] mutableCopy] autorelease];
	
	NSArray *array = [style tabStops];
	for (id item in array) {
		[style removeTabStop:item];
	}
	[style setDefaultTabInterval:sizeOfTab];
	NSDictionary *attributes = [NSDictionary dictionaryWithObjectsAndKeys:style, NSParagraphStyleAttributeName, nil];
	[[self textStorage] addAttributes:attributes range:NSMakeRange(0, [[self textStorage] length])];
}
#pragma mark Notifications
- (void)_didOpenUndoGroup:(NSNotification *)note {
	[self setChangeCount:[self changeCount] + 1];
}
- (void)_didUndoChange:(NSNotification *)note {
	[self setChangeCount:[self changeCount] - 1];
}
- (void)_didRedoChange:(NSNotification *)note {
	[self setChangeCount:[self changeCount] + 1];
}
@end
