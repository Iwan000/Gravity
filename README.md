# Gravity Well Tuning

This project ships two gravity-well bullets (black- and white-hole) you can tune directly in Blueprint. The main numbers live in each bullet Blueprint under the **Gravity Well Numerical** section of the Details panel.

## Change strength and radius in the bullet Blueprints

Black-hole bullet (fires on left click):
- Asset: `Content/Variant_Shooter/Blueprints/Pickups/Projectiles/BP_GravityWellProjectile.uasset`
- Steps: open the Blueprint → select the Class Defaults (or the projectile component) → in **Gravity Well Numerical** edit:
  - **Strength**: pull force; higher values accelerate faster.
  - **Max Radius**: outer influence radius.
  - (Optional) **Min Radius**, **Max Accel**, **Tick Interval** also live here if you need finer control.

White-hole bullet (right-click transform):
- Asset: `Content/Variant_Shooter/Blueprints/Pickups/Projectiles/BP_WhiteHoleProjectile.uasset` (uses the repelling well)
- Steps: same as above; adjust **Strength** and **Max Radius** in **Gravity Well Numerical** to set the default repulsion values.

Each bullet’s settings become the defaults for the well it spawns. You can give black and white holes different ranges/strengths by setting different numbers in their respective bullet Blueprints.

## Where the defaults come from (C++)

Baseline values live in `Source/Gravity_test/GravityWellActor.h`:
- `Strength = 3,000,000`, `MaxRadius = 1500`, `MinRadius = 150`, `MaxAccel = 6000`, `TickInterval = 0.03`

Blueprint values override these defaults per asset/instance. If you need project-wide new baselines, change them in the header above and recompile.***
