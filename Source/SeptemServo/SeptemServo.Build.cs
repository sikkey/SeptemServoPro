// Copyright (c) 2013-2019 7Mersenne All Rights Reserved.

using UnrealBuildTool;

public class SeptemServo : ModuleRules
{
	public SeptemServo(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
                //"Protocol", "SeptemAlgortihm"
				// ... add public include paths required here ...
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
                "Core", "CoreUObject", "Engine", "InputCore",
            "Sockets","Networking"
            }
			);

        // Uncomment if you are using Slate UI
        /*
        PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				// ... add private dependencies that you statically link with here ...	
			}
			);
            */

        // Uncomment if you are using online features
        // PrivateDependencyModuleNames.Add("OnlineSubsystem");

        // To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true


        DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);

        MinFilesUsingPrecompiledHeaderOverride = 1;
        bFasterWithoutUnity = true;
    }
}
