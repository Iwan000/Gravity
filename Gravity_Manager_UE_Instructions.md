# Force System v1 — Unreal Engine Setup Guide

This guide describes the in-editor steps to use the new C++ force system.

---

## 1. Build the project

Use the standard build command for this repo:

```
"<UE root>/Engine/Build/BatchFiles/Build.bat" Gravity_testEditor Win64 Development -project="`pwd`/Gravity_test.uproject"
```

---

## 2. Place force sources (actors)

The system is Source-driven. Add any of these actors to your level:

- `AGravitySourceBlackHole`
- `AGravitySourceWhiteHole`
- `AGravitySourceWind`

Recommended workflow:

1. Create Blueprint subclasses (e.g. `BP_GravitySource_BlackHole`) so designers can tweak values safely.
2. Select the source actor and adjust:
   - **RangeCollision radius** (Sphere component radius = influence range).
   - **Strength** (cm/s² scale; higher = stronger pull/push).
   - **MinRadius / MaxAccel** (black/white hole only).
   - **Wind direction** (wind only; or use actor forward).

The force manager is a `UForceManagerSubsystem` and does not need to be placed in a level.

---

## 3. Default receivers (physics objects)

Any `UPrimitiveComponent` with **Simulate Physics = true** will be affected automatically.

Make sure:

- The component’s collision is enabled.

Drop a physics cube into a source’s range to verify it reacts.

---

## 4. Special receivers (characters / non-physics)

Characters and other non-simulating actors should **not** use physics apply.  
Instead, implement the interface and consume the resolved force:

1. In the Character (or Actor) Blueprint, add interface `ForceConsumerInterface`.
2. Implement `OnForceResolved(FinalAccel, DeltaTime)`.
3. Use this output to drive your custom movement logic.

Example patterns:

- Store `FinalAccel` on the actor and apply it to velocity in Tick.
- Feed it into a custom movement component.
- Convert it into movement input for a prototype feel.

This is the **ConsumeOnly** path; no physics forces are applied automatically.

---

## 5. World gravity

V1 keeps Unreal’s world gravity enabled.  
Do **not** disable world gravity yet (Phase 2 task).

---

## 6. Debugging tips

- If a receiver does not react, confirm:
  - It is overlapping the source `RangeCollision`.
  - It is simulating physics (for default receivers).
  - The Actor implements `ForceConsumerInterface` (for special receivers).
  - Strength values are high enough (cm/s² scale).

---

## 7. Summary

- Sources drive the system.
- Physics components are auto-applied.
- Characters use ConsumeOnly via interface.
- Manager is a world subsystem (no level setup).
