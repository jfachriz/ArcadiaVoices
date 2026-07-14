
#include <TargetConditionals.h>
#if TARGET_OS_IOS == 1 || TARGET_OS_VISION == 1
#import <UIKit/UIKit.h>
#else
#import <Cocoa/Cocoa.h>
#endif

#define IPLUG_AUVIEWCONTROLLER IPlugAUViewController_vArcadiaVoices
#define IPLUG_AUAUDIOUNIT IPlugAUAudioUnit_vArcadiaVoices
#import <ArcadiaVoicesAU/IPlugAUViewController.h>
#import <ArcadiaVoicesAU/IPlugAUAudioUnit.h>

//! Project version number for ArcadiaVoicesAU.
FOUNDATION_EXPORT double ArcadiaVoicesAUVersionNumber;

//! Project version string for ArcadiaVoicesAU.
FOUNDATION_EXPORT const unsigned char ArcadiaVoicesAUVersionString[];

@class IPlugAUViewController_vArcadiaVoices;
