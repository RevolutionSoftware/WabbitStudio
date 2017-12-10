//
//  WCGeneralPerformer.h
//  WabbitStudio
//
//  Created by William Towe on 3/18/11.
//  Copyright 2011 Revolution Software. All rights reserved.
//

#import "WCSingleton.h"
#import <Foundation/NSURL.h>
#import <AppKit/NSColor.h>

extern NSString *const kWCProjectToolbarBuildTargetPopUpButtonItemIdentifier;
extern NSString *const kWCProjectToolbarBuildItemIdentifier;
extern NSString *const kWCProjectToolbarBuildAndRunItemIdentifier;
extern NSString *const kWCProjectToolbarBuildAndDebugItemIdentifer;

extern NSString *const kWCProjectToolbarStatusViewItemIdentifier;
extern NSString *const kWCProjectToolbarProjectWindowItemIdentifier;

@class WCFile,WCProject,WCProjectTemplate,WCFileTemplate,WCDocument,WCBreakpoint;

@interface WCGeneralPerformer : WCSingleton {
@private

}
- (BOOL)addFileURLsInDirectory:(NSURL *)directoryURL toFile:(WCFile *)file;
- (BOOL)addFileURLsInDirectory:(NSURL *)directoryURL toFile:(WCFile *)file inDirectoryURL:(NSURL *)directory;
- (BOOL)addFileURLsInDirectory:(NSURL *)directoryURL toFile:(WCFile *)file inDirectoryURL:(NSURL *)directory atIndex:(NSUInteger)index;
- (BOOL)addFileURLs:(NSArray *)urls toFile:(WCFile *)file;
- (BOOL)addFileURLs:(NSArray *)urls toFile:(WCFile *)file atIndex:(NSUInteger)index;
- (BOOL)addFilePaths:(NSArray *)paths toFile:(WCFile *)file atIndex:(NSUInteger)index;
- (BOOL)addFileURLs:(NSArray *)urls toFile:(WCFile *)file inDirectoryURL:(NSURL *)directory atIndex:(NSUInteger)index;

- (BOOL)addDocument:(WCDocument *)document toProject:(WCProject *)project;

- (WCProject *)createProjectFromFolder:(NSURL *)folderURL error:(NSError **)outError;
- (WCProject *)createProjectAtURL:(NSURL *)projectURL withTemplate:(WCProjectTemplate *)projectTemplate error:(NSError **)error;
- (NSFileWrapper *)fileWrapperForProject:(WCProject *)project error:(NSError **)outError;

- (WCFile *)createFileAtURL:(NSURL *)fileURL withTemplate:(WCFileTemplate *)fileTemplate error:(NSError **)error;

- (NSURL *)userProjectTemplatesURL;
- (NSURL *)userFileTemplatesURL;
- (NSURL *)userIncludeFilesURL;

- (NSColor *)findBackgroundColor;
- (NSColor *)findUnderlineColor;
- (NSDictionary *)findAttributes;
- (NSParagraphStyle *)truncateLeftParagraphStyle;

- (void)drawBreakpoint:(WCBreakpoint *)breakpoint inRect:(NSRect)rect;

- (NSToolbarItem *)toolbar:(NSToolbar *)toolbar itemForItemIdentifier:(NSString *)itemIdentifier willBeInsertedIntoToolbar:(BOOL)flag inProject:(WCProject *)project;
@end
