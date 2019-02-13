// Copyright (c) 2013-2019 7Mersenne All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class SeptemServoEditorTarget : TargetRules
{
	public SeptemServoEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;

		ExtraModuleNames.AddRange( new string[] { "SeptemServo" } );
	}
}
