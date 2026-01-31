# Gravity System v1 — Summary (What We Built, How to Use, and Current Issues)

This is a concise summary of the new gravity/force system, how to use it, and what issues remain.

---

## 1) How to Use (Quick Start)

- **Place a Source actor** in the level:
  - `AGravitySourceBlackHole`
  - `AGravitySourceWhiteHole`
  - `AGravitySourceWind`
- **Physics objects** (any `UPrimitiveComponent` with **Simulate Physics = true**) are affected automatically.
- **Characters / special objects** must implement `UForceConsumerInterface` and handle `OnForceResolved` in Blueprint/C++.
- **Manager** is a `UForceManagerSubsystem` (no level placement required).

> Recommended: Create BP subclasses of each Source and tune `Strength`, `MinRadius`, `MaxAccel`, and `RangeCollision` radius.

---

## 2) Class Structure (What Exists Now)

### `UForceManagerSubsystem`
**Role:** Global manager that resolves + applies forces every tick (PrePhysics).

- Accumulates per-receiver acceleration.
- Applies **deadzone + clamp** (global defaults).
- Applies to physics objects.
- Calls special receivers via interface (ConsumeOnly).

Key structures:
- `FReceiverForceSettings` (global default settings)
- `FForceAccumulator` (SumAccel + touched)
- Maps: `AccMap` (physics), `ActorAccMap` (special)

---

### `AGravitySourceBase`
**Role:** Base class for force sources.

- Owns a `UShapeComponent` range collision.
- Each tick:
  - Queries physics bodies (object query).
  - Computes contribution for each receiver.
  - Sends accel to manager.
- Tracks **special receivers** via overlap events.

Key fields:
- `SourceId`, `SourceType`, `Strength`
- `RangeCollision`

---

### Concrete Sources

**`AGravitySourceBlackHole`**  
Attractive inverse-square force.  
Fields: `Strength`, `MinRadius`, `MaxAccel`

**`AGravitySourceWhiteHole`**  
Repulsive inverse-square force.  
Fields: `Strength`, `MinRadius`, `MaxAccel`

**`AGravitySourceWind`**  
Constant directional force.  
Fields: `Strength`, `bUseActorForward`, `WindDirection`

---

### `UForceConsumerInterface`
Interface for special receivers (characters, non-physics):
- `OnForceResolved(FinalAccel, DeltaTime)`

---

## 3) How to Build Future Special Receivers

1) Add `ForceConsumerInterface` to the Actor/Character BP.
2) Implement `OnForceResolved`:
   - Convert accel to movement input, velocity, or a custom movement component.
3) Keep physics simulation **off** (ConsumeOnly path).

This keeps special receivers **low-cost and scalable** for large content teams.

---

## 4) Current Issue (Known)

**Energy growth / unstable orbits**
- Objects can gain energy and spiral outward.
- Happens even with clamp + sub-stepping.
- Root cause: forces are applied per frame while physics integrates in sub-steps, causing numerical energy injection.

Symptoms:
- Oscillation amplitude increases
- Objects “break free” from attraction
- Wind cancelling gravity is imperfect

---

## 5) Likely Fixes (Next Steps)

**Best solution (engine-level stability):**
1) **Apply forces in sub-steps** (`OnCalculateCustomPhysics`) so the force is recomputed each sub-step.
2) Optional **global damping** (`FinalAccel -= Velocity * Damping`) to remove numerical energy.

**Additional tuning:**
- Keep `MaxAccel > 0` and `MinRadius > 0`.
- Consider softening: `Strength / (r² + softening²)`.

---

## 6) Files Added / Modified

**Core system:**
- `Source/Gravity_test/ForceTypes.h`
- `Source/Gravity_test/ForceConsumerInterface.h`
- `Source/Gravity_test/ForceManagerSubsystem.h`
- `Source/Gravity_test/ForceManagerSubsystem.cpp`
- `Source/Gravity_test/GravitySourceBase.h`
- `Source/Gravity_test/GravitySourceBase.cpp`
- `Source/Gravity_test/GravitySourceBlackHole.h`
- `Source/Gravity_test/GravitySourceBlackHole.cpp`
- `Source/Gravity_test/GravitySourceWhiteHole.h`
- `Source/Gravity_test/GravitySourceWhiteHole.cpp`
- `Source/Gravity_test/GravitySourceWind.h`
- `Source/Gravity_test/GravitySourceWind.cpp`

**Docs:**
- `Gravity_Manager_implementation_plan.md`
- `Gravity_Manager_UE_Instructions.md`

---

## 7) Summary in One Sentence

We built a clean Source→Manager→Receiver gravity pipeline with subsystem-based management and interface-driven special receivers, but we still need sub-step force application (and possibly damping) to eliminate energy drift and make orbits stable.
