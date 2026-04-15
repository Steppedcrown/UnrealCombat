# UnrealCombat

A third-person melee combat system built in Unreal Engine 5 using C++ and the **Gameplay Ability System (GAS)**. The project explores data-driven ability design, frame-precise hit detection, and a node-based resource mechanic that ties a character's offensive options to their remaining health.

---

## Combat Mechanics

### Combo Attacks
Light attacks chain into a multi-stage combo string. Input is cached between attacks so that a press during the current swing is queued and consumed the moment the `AnimNotifyState_ComboWindow` opens. If no input arrives before the window closes, the combo resets. Each combo stage maps to a named `AnimMontage` section, allowing the sequence to be extended purely through Blueprint configuration.

### Charged Attack
Holding the attack input plays a charge-loop section of a dedicated montage. `AnimNotify_CheckChargedAttack` polls each frame to determine whether the button is still held. Releasing transitions immediately into the release strike section.

### Block & Perfect Block
Blocking applies a `State.Combat.Blocking` tag for a configurable window (~1 second). The attribute set intercepts incoming damage effects while this tag is active and fires a `BlockHit` gameplay event instead of applying HP reduction. A hit absorbed within the first **~0.15 s** (~9 frames at 60 fps) of activation counts as a **perfect block**, which:
- Restores Nodes to the defender
- Applies knockback to all enemies within a configurable radius

### Expel
A node-spending ability that deals damage and applies **TempNodes** to the target. TempNodes are a temporary resource buffer on the enemy that inflates their apparent Node count, setting up a Rip finisher.

### Rip
A node-spending execute-or-drain ability. On hit it branches on the target's `State.Status.Vulnerable` tag:

| Target state | Outcome |
|---|---|
| Not Vulnerable | Minor damage + drain all Nodes from target to self |
| Vulnerable | Instant kill via `GE_Kill` |

When the target is both Vulnerable and locked on, Rip plays a dedicated **execution montage** using Motion Warping to snap the attacker into position.

### Node System
Nodes are a shared stamina/energy attribute that gate the more powerful abilities and determine when a character becomes killable.

- Each character starts with a `BaseMaxNodes` pool (default: 4)
- `MaxNodes` is reduced by 1 for every **20% health threshold** lost below full HP
- Nodes reaching 0 applies `State.Status.Vulnerable` — the character can now be executed by Rip
- Basic attacks **restore** Nodes on hit; Expel and Rip **consume** them on activation
- **TempNodes** are a separate attribute applied by Expel that temporarily expand a target's Node pool, making them harder to Vulnerable without a follow-up Rip

### Lock-On
`ULockOnComponent` performs a sphere overlap on toggle, filters candidates to a configurable forward cone (default: 60°, 1500 cm), and locks to the closest valid target. While locked, the camera spring arm interpolates toward the target each tick, and the execution branch of Rip uses the locked actor as its Motion Warping warp target.

---

## Technical Implementation

### Gameplay Ability System
All combat actions are implemented as `UGameplayAbility` subclasses backed by `UGameplayEffect` for stat manipulation. The `UCombatAttributeSet` owns `Health`, `MaxHealth`, `Nodes`, `MaxNodes`, and `TempNodes`. `PostGameplayEffectExecute` is the single authoritative place where damage is clamped, blocking is checked, node penalties are recalculated, and status tags (`Vulnerable`, `Dead`) are applied.

```
UCombatAttributeSet
  ├── Health / MaxHealth
  ├── Nodes / MaxNodes          ← MaxNodes recalculated every HP threshold crossed
  └── TempNodes                 ← temporary buffer applied by Expel
```

### Data-Driven Move Registry
Move parameters live in `UCombatMoveData` primary data assets (one per ability). At runtime, abilities look up their data from `UCombatMoveRegistry` — a `TMap<FGameplayTag, UCombatMoveData*>` asset assigned to the character — using the ability's own gameplay tag as the key. This keeps per-move tuning (damage, node cost/gain, frame data, weapon bone, sweep extents, VFX, SFX) entirely out of C++ and editable in-editor without recompilation.

```
UCombatMoveRegistry  (DataAsset)
  └── Moves: TMap<FGameplayTag, UCombatMoveData*>
        ├── Ability.BasicAttack  → DA_BasicAttack
        ├── Ability.Block        → DA_Block
        ├── Ability.Expel        → DA_Expel
        └── Ability.Rip         → DA_Rip

UCombatMoveData  (PrimaryDataAsset)
  ├── Damage, NodeCost, NodeGain
  ├── StartupFrames, ActiveFrames, RecoveryFrames
  ├── WeaponBoneName, SweepHalfExtent
  └── AnimationMontage, HitEffect, HitSound
```

### Frame-Precise Hit Detection
`UCombatHitDetectionComponent` sweeps a configurable box from the weapon bone's previous-tick world position to its current position every tick while tracing is active. A per-swing `TArray` deduplicates hits so each target is struck at most once per activation. Abilities drive the trace window through GAS gameplay events:

1. `ANS_ActiveFrames` (AnimNotifyState) fires `ActiveFramesBegin` / `ActiveFramesEnd` gameplay events at the anim layer
2. Abilities listen via `UAbilityTask_WaitGameplayEvent` and call `StartTrace()` / `StopTrace()` on the component
3. Each `OnHit` broadcast triggers the ability to apply the appropriate gameplay effects to the target

### Animation Notify Architecture
Three distinct notify types cover the combat animation pipeline:

| Notify | Purpose |
|---|---|
| `AnimNotify_DoAttackTrace` | Legacy sphere-trace trigger (pre-GAS path, kept for reference) |
| `AnimNotify_CheckCombo` | Evaluates cached input to decide whether to advance the combo chain |
| `AnimNotify_CheckChargedAttack` | Polls the held-input flag to decide whether to transition to the attack or cancel |
| `AnimNotifyState_ComboWindow` | Opens/closes the combo input acceptance window |
| `ANS_ActiveFrames` | Gates `UCombatHitDetectionComponent` sweeps via GAS gameplay events |

### Ability Flow (BasicAttack example)
```
Input pressed
  └─ DoComboAttackStart()
       └─ ASC->TryActivateAbility(BasicAttackAbilityClass)
            └─ UGA_BasicAttack::ActivateAbility()
                 1. Tag State.Combat.Attacking applied (blocks re-activation)
                 2. Look up DA_BasicAttack from MoveRegistry
                 3. Play AnimationMontage via UAbilityTask_PlayMontageAndWait
                 4. WaitGameplayEvent(ActiveFramesBeginTag) → StartTrace()
                 5. WaitGameplayEvent(ActiveFramesEndTag)   → StopTrace()
                 6. OnHit → ApplyHitEffects(target)
                      ├── GE_DamageHealth → target
                      └── GE_RestoreNode  → self
                 7. Montage end → EndAbility (tag removed)
```

### Project Structure
```
Source/UnrealCombat/Variant_Combat/
  ├── CombatCharacter.h/.cpp          # Base character: input, combo/charged logic, respawn
  ├── CombatAttributeSet.h/.cpp       # GAS attributes: Health, Nodes, status tag application
  ├── CombatMoveData.h                # Primary data asset: per-move parameters
  ├── CombatMoveRegistry.h/.cpp       # Tag → MoveData lookup asset
  ├── CombatHitDetectionComponent.h   # Per-tick bone sweep, OnHit delegate
  ├── LockOnComponent.h/.cpp          # Target acquisition, camera interpolation
  ├── Abilities/
  │   ├── GA_BasicAttack              # Combo hit with node restore on hit
  │   ├── GA_Block                    # Block window + perfect block detection
  │   ├── GA_Expel                    # Node-cost attack that applies TempNodes to target
  │   └── GA_Rip                      # Node drain or execution finisher
  ├── Animation/
  │   ├── ANS_ActiveFrames            # GAS event bridge for hit window
  │   ├── AnimNotify_CheckCombo       # Combo chain evaluation
  │   ├── AnimNotify_CheckChargedAttack
  │   └── AnimNotifyState_ComboWindow
  ├── AI/
  │   └── CombatAIController          # Base AI controller (Behavior Tree in Phase 11)
  ├── Interfaces/
  │   ├── CombatAttacker              # Attack trace / combo / charged attack contract
  │   └── CombatDamageable            # Damage / death / healing / danger notification contract
  └── UI/
      └── CombatLifeBar               # Health bar widget component
```

---

## Engine & Dependencies
- **Unreal Engine 5**
- **Gameplay Ability System** (GAS) — abilities, effects, attribute sets, gameplay tags
- **Motion Warping** — execution animation positioning
- **Enhanced Input** — input actions for all combat controls
- **Niagara** — hit VFX referenced from move data assets
