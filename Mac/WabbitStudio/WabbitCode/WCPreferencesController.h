//
//  WCPreferencesController.h
//  WabbitStudio
//
//  Created by William Towe on 3/22/11.
//  Copyright 2011 Revolution Software. All rights reserved.
//

#import "WCSingleton.h"


extern NSString *const kWCPreferencesAddFilesToProjectCopyFilesKey;

enum WCPreferencesAddFilesToProjectGroupsOrFolderReferences {
	WCPreferencesAddFilesToProjectGroups = 0,
	WCPreferencesAddFilesToProjectFolderReferences
};
typedef NSUInteger WCPreferencesAddFilesToProjectGroupOrFolderReferences;
extern NSString *const kWCPreferencesAddFilesToProjectGroupOrFolderReferencesKey;

enum WCPreferencesOnStartup {
	WCPreferencesOnStartupShowNewProjectWindow = 0,
	WCPreferencesOnStartupOpenMostRecentProject,
	WCPreferencesOnStartupDoNothing,
	WCPreferencesOnStartupOpenMostRecentDocument
};
typedef NSUInteger WCPreferencesOnStartup;
extern NSString *const kWCPreferencesOnStartupKey;

extern NSString *const kWCPreferencesUseSyntaxHighlightingKey;

extern NSString *const kWCPreferencesDirectivesKey;
extern NSString *const kWCPreferencesDirectivesColorKey;
extern NSString *const kWCPreferencesOpCodesKey;
extern NSString *const kWCPreferencesOpCodesColorKey;
extern NSString *const kWCPreferencesConditionalsKey;
extern NSString *const kWCPreferencesConditionalsColorKey;
extern NSString *const kWCPreferencesRegistersKey;
extern NSString *const kWCPreferencesRegistersColorKey;
extern NSString *const kWCPreferencesPreOpsKey;
extern NSString *const kWCPreferencesPreOpsColorKey;
extern NSString *const kWCPreferencesStringsKey;
extern NSString *const kWCPreferencesStringsColorKey;
extern NSString *const kWCPreferencesNumbersKey;
extern NSString *const kWCPreferencesNumbersColorKey;
extern NSString *const kWCPreferencesHexadecimalsKey;
extern NSString *const kWCPreferencesHexadecimalsColorKey;
extern NSString *const kWCPreferencesBinariesKey;
extern NSString *const kWCPreferencesBinariesColorKey;
extern NSString *const kWCPreferencesCommentsKey;
extern NSString *const kWCPreferencesCommentsColorKey;
extern NSString *const kWCPreferencesMultilineCommentsColorKey;

extern NSString *const kWCPreferencesLabelsKey;
extern NSString *const kWCPreferencesLabelsColorKey;
extern NSString *const kWCPreferencesEquatesKey;
extern NSString *const kWCPreferencesEquatesColorKey;
extern NSString *const kWCPreferencesDefinesKey;
extern NSString *const kWCPreferencesDefinesColorKey;
extern NSString *const kWCPreferencesMacrosKey;
extern NSString *const kWCPreferencesMacrosColorKey;

extern NSString *const kWCPreferencesCurrentLineHighlightKey;
extern NSString *const kWCPreferencesCurrentLineHighlightColorKey;

extern NSString *const kWCPreferencesEditorTextColorKey;
extern NSString *const kWCPreferencesEditorBackgroundColorKey;

extern NSString *const kWCPreferencesEditorFontKey;

extern NSString *const kWCPreferencesEditorShowEquateValueTooltipsKey;

extern NSString *const kWCPreferencesEditorAutomaticallyIndentNewLinesKey;
extern NSString *const kWCPreferencesEditorAutomaticallyIndentAfterLabelsKey;
extern NSString *const kWCPreferencesEditorTabWidthKey;

enum {
	WCPreferencesEditorIndentUsingSpaces = 0,
	WCPreferencesEditorIndentUsingTabs
};
typedef NSUInteger WCPreferencesEditorIndentUsing;
extern NSString *const kWCPreferencesEditorIndentUsingKey;

extern NSString *const kWCPreferencesEditorDisplayErrorBadgesKey;
extern NSString *const kWCPreferencesEditorErrorLineHighlightKey;
extern NSString *const kWCPreferencesEditorErrorLineHighlightColorKey;
extern NSString *const kWCPreferencesEditorDisplayWarningBadgesKey;
extern NSString *const kWCPreferencesEditorWarningLineHighlightKey;
extern NSString *const kWCPreferencesEditorWarningLineHighlightColorKey;
extern NSString *const kWCPreferencesEditorWrapLinesKey;
extern NSString *const kWCPreferencesEditorBreakpointLineHighlightColorKey;
extern NSString *const kWCPreferencesEditorProgramCounterHighlightColorKey;

extern NSString *const kWCPreferencesBuildingTreatWarningsAsErrorsKey;

extern NSString *const kWCPreferencesNewBuildTargetsCopyActiveBuildTargetKey;

extern NSString *const kWCPreferencesAdvancedProjectTemplatesPreferUserIncludeFilesKey;
extern NSString *const kWCPreferencesAdvancedConfirmDeleteOfBreakpointKey;

enum WCPreferencesBuildingForUnsavedFiles {
	WCPreferencesBuildingForUnsavedFilesAlwaysSave = 0,
	WCPreferencesBuildingForUnsavedFilesPromptBeforeSaving,
	WCPreferencesBuildingForUnsavedFilesNeverSave
};
typedef NSUInteger WCPreferencesBuildingForUnsavedFiles;
extern NSString *const kWCPreferencesBuildingForUnsavedFilesKey;

enum WCPreferencesBuildingLocation {
	WCPreferencesBuildingLocationProjectFolder = 0,
	WCPreferencesBuildingLocationCustom
};
typedef NSUInteger WCPreferencesBuildingLocation;
extern NSString *const kWCPreferencesBuildingLocationKey;
extern NSString *const kWCPreferencesBuildingCustomLocationKey;

extern NSString *const kWCPreferencesFilesTextEncodingKey;

enum WCPreferencesFilesOpenWith {
	WCPreferencesFilesOpenWithDoubleClick = 0,
	WCPreferencesFilesOpenWithSingleClick
};
typedef NSUInteger WCPreferencesFilesOpenWith;
extern NSString *const kWCPreferencesFilesOpenWithKey;

@interface WCPreferencesController : WCSingleton {
@private
    
}

@end
