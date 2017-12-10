//
//  WEPreferencesController.h
//  WabbitEmu Beta
//
//  Created by William Towe on 4/25/11.
//  Copyright 2011 Revolution Software. All rights reserved.
//

#import <Foundation/NSObject.h>


extern NSString *const kWEPreferencesDisplayLCDShadesKey;
extern NSString *const kWEPreferencesDisplayLCDModeKey;

enum {
	WEGeneralOnStartupShowOpenPanel = 0,
	WEGeneralOnStartupOpenMostRecentRomOrSavestate,
	WEGeneralOnStartupDoNothing
};
typedef NSUInteger WEGeneralOnStartup;
extern NSString *const kWEPreferencesGeneralOnStartupKey;

@interface WEPreferencesController : NSObject {
@private
    
}

@end
