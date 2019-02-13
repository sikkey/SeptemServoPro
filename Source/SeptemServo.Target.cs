// Copyright (c) 2013-2019 7Mersenne All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class SeptemServoTarget : TargetRules
{
	public SeptemServoTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;

		ExtraModuleNames.AddRange( new string[] { "SeptemServo" } );
	}
}
