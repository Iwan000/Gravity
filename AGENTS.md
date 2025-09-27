# Repository Guidelines

## Project Structure & Module Organization
Source code lives under `Source/Gravity_test`, with the root gameplay module in files like `Gravity_testGameMode.cpp` and first-person character logic in `Gravity_testCharacter.cpp`. Variant prototypes (e.g., `Source/Gravity_test/Variant_Shooter`) host experimental controllers, AI, UI, and weapons. Blueprint assets, materials, and the upcoming gravity-well Blueprint belong in `Content/`, while project configuration (input, maps, packaging) sits in `Config/`. Binary outputs and intermediates regenerate; avoid editing `Binaries/`, `Intermediate/`, or `DerivedDataCache/` directly.

## Build, Test, and Development Commands
Open the project in-editor with `"<UE root>/Engine/Binaries/Win64/UnrealEditor.exe" Gravity_test.uproject`. Build C++ modules via `"<UE root>/Engine/Build/BatchFiles/Build.bat" Gravity_testEditor Win64 Development -project="`pwd`/Gravity_test.uproject"`. To run a headless PIE session for quick checks: `UnrealEditor-Cmd.exe Gravity_test -game -log`.

## Coding Style & Naming Conventions
Follow Epic’s C++ style: 4-space indentation, `UpperCamelCase` for classes (e.g., `AGravityWellActor`), `bPrefix` booleans, and `PascalCase` Blueprint assets (`BP_GravityWell_Simple`). Group related properties with `UPROPERTY(EditAnywhere)` metadata and expose gravity-well parameters (`Strength`, `MaxRadius`, `TickInterval`) for designers. Keep Blueprint graphs tidy with reroute nodes and function libraries for reusable math.

## Testing Guidelines
Prefer Unreal Automation tests (`Functional`, `Performance`) run through the Session Frontend. Execute targeted suites with `UnrealEditor-Cmd.exe Gravity_test -ExecCmds="Automation RunTests Gravity.*; Quit"`. When adding gameplay features, pair them with at least one automation or PIE smoke test validating force application and character recovery to baseline gravity.

## Commit & Pull Request Guidelines
Write commits in imperative mood (`Add gravity well timer loop`) and limit scope to one logical change. PRs should describe gameplay impact, list editor steps for verification, and attach short clips or logs demonstrating the gravity well pulling rigid bodies without instability. Link Jira/task IDs when available and note any content migrations.

## Gravity Well Feature Expectations
Implement the gravity well as `BP_GravityWell_Simple` with an `InfluenceSphere` driving overlap checks. Clamp radius (`MinRadius`) and acceleration (`MaxAccel`) before calling `AddForce` with `Accel Change` enabled. Characters drawn into the well must suspend gravity (`CharacterMovement->SetGravityScale(0)`) and restore it on exit or actor destruction. Default values: Strength 3,000,000; MaxRadius 1,500 cm; MinRadius 150 cm; TickInterval 0.03 s.
