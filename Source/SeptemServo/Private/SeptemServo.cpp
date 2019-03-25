// Copyright (c) 2013-2019 7Mersenne All Rights Reserved.

#include "SeptemServo.h"

#define LOCTEXT_NAMESPACE "FSeptemServoModule"

//#define SEPTEM_SERVO_PLUGIN

void FSeptemServoModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FSeptemServoModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE

#ifdef SEPTEM_SERVO_PLUGIN
IMPLEMENT_MODULE(FSeptemServoModule, SeptemServo)
#else
IMPLEMENT_PRIMARY_GAME_MODULE(FDefaultGameModuleImpl, SeptemServo, "SeptemServo");
#endif // SEPTEM_SERVO_PLUGIN

