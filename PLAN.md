# Plan

Specific step-by-step implementation plan for UnrealCombat. Mark steps as complete with `[x]` as you go.

---

## Phase 1 — Project Foundation

### Folder Structure
- [x] In the UE5 Content Browser, create the following folders under `Content/`:
  - `Core`
  - `Characters`
  - `Combat`
  - `UI`
  - `VFX`
  - `SFX`
  - `Animations`
  - `Levels`

### Plugins
- [x] Open **Edit > Plugins** and enable **Gameplay Abilities**
- [x] Enable **Motion Warping**
- [x] Confirm **Enhanced Input** is enabled (on by default in UE5.1+)
- [x] Restart the editor after enabling plugins
- [x] In `UnrealCombat.Build.cs`, add to `PublicDependencyModuleNames`:
  - `"GameplayAbilities"`, `"GameplayTasks"`, `"GameplayTags"`, `"MotionWarping"`

### Gameplay Tags
- [x] Open **Edit > Project Settings > GameplayTags** and add a new tag source file `GameplayTags.ini`
- [x] Add the following tags:
  - [x] `State.Combat.Blocking`
  - [x] `State.Combat.Attacking`
  - [x] `State.Status.Vulnerable`
  - [x] `State.Status.Dead`
  - [x] `Ability.BasicAttack`
  - [x] `Ability.Block`
  - [x] `Ability.Expel`
  - [x] `Ability.Rip`
  - [x] `Effect.TempNode`
  - [x] `Effect.Knockback`
  - [x] `Effect.HitStop`

---

## Phase 2 — Core Character & Movement

> The template's `Variant_Combat/ACombatCharacter` already provides camera, spring arm, movement, input bindings, combo/charged attack framework, HP, damage, death, and respawn. We extend it with GAS rather than replacing it.

### ACombatCharacter — Add GAS (C++)
- [x] In `Variant_Combat/CombatCharacter.h`, add `IAbilitySystemInterface` to the class inheritance
- [x] Forward declare `UAbilitySystemComponent` and `UMotionWarpingComponent`
- [x] Add `UAbilitySystemComponent* AbilitySystemComponent` as a private `UPROPERTY`
- [x] Add `UMotionWarpingComponent* MotionWarpingComponent` as a private `UPROPERTY`
- [x] Declare `GetAbilitySystemComponent()`, `PossessedBy()`, and `OnRep_PlayerState()`
- [x] In `CombatCharacter.cpp`, include `AbilitySystemComponent.h` and `MotionWarpingComponent.h`
- [x] Create both components in the constructor with `CreateDefaultSubobject`
- [x] Implement `PossessedBy` and `OnRep_PlayerState` calling `AbilitySystemComponent->InitAbilityActorInfo(this, this)`
- [x] Compile and confirm the game runs without crashing

### Blueprints
- [x] Create Blueprint `BP_Player` in `Content/Characters/` inheriting from `ACombatCharacter`
  - [x] Assign skeletal mesh (use template mannequin temporarily)
  - [x] Assign existing template animation blueprint temporarily
  - [x] Assign all Input Actions from the template's input folder
  - [x] Assign the `LifeBar` widget class (use template's `WBP_CombatLifeBar` temporarily)
- [x] Create Blueprint `BP_Enemy` in `Content/Characters/` inheriting from `ACombatCharacter`
- [x] Set `BP_Player` as the Default Pawn in the Game Mode
- [x] Confirm the game launches and the character moves, jumps, and attacks correctly

### New Input Actions
The template already has `IA_Move`, `IA_Look`, `IA_Jump`, and attack actions. Add only what's missing:
- [x] Create `IA_Block` (Digital) in the template's input folder
- [x] Create `IA_Expel` (Digital)
- [x] Create `IA_Rip` (Digital)
- [x] Create `IA_LockOn` (Digital)
- [x] Add all four to the existing IMC with keyboard and controller bindings
- [x] Bind each to a stub handler in `SetupPlayerInputComponent()` (full ability wiring comes in Phase 5)

### Lock-On System
- [x] Create a C++ component `ULockOnComponent` and attach it to `ACombatCharacter`
- [x] On `IA_LockOn` press: perform a sphere overlap to find nearby `ACombatCharacter` enemies within a forward cone
- [x] Store the closest valid target as `TargetActor`
- [x] Each tick while locked: rotate the spring arm to face `TargetActor` using `FMath::RInterpTo`
- [x] On `IA_LockOn` press again (or target dies): clear `TargetActor` and restore free camera
- [x] Expose `LockOnRange` and `LockOnAngle` as editable Blueprint properties for tuning

---

## Phase 3 — Attribute System (GAS)

> Once GAS attributes are working, remove the template's `CurrentHP`, `MaxHP`, `LifeBarWidget`, `TakeDamage`, `HandleDeath`, `ResetHP`, and `ApplyDamage` from `ACombatCharacter` — these will all be handled by GAS effects and the attribute set going forward.

### UCombatAttributeSet (C++)
- [x] Create C++ class `UCombatAttributeSet` inheriting from `UAttributeSet`
- [x] Declare the following attributes using the `ATTRIBUTE_ACCESSORS` macro:
  - [x] `Health`, `MaxHealth`
  - [x] `Nodes`, `MaxNodes`
  - [x] `TempNodes`
- [x] Override `PostGameplayEffectExecute()` to:
  - [x] Clamp `Health` between `0` and `MaxHealth`
  - [x] Clamp `Nodes` between `0` and `MaxNodes`
  - [x] Clamp `TempNodes` to `>= 0` (no upper cap)
  - [x] When `Health` changes: recalculate `MaxNodes` (subtract 1 for every 20% threshold crossed below full health)
  - [x] When `Nodes` reaches `0`: apply `State.Status.Vulnerable` tag to the owner
  - [x] When `Nodes` goes above `0`: remove `State.Status.Vulnerable` tag
  - [x] When `Health` reaches `0`: apply `State.Status.Dead` tag

### Gameplay Effects
Create the following Blueprint Gameplay Effects in `Content/Combat/Effects/`:
- [x] `GE_DamageHealth` — Instant, modifies `Health` by `-Magnitude`
- [x] `GE_RestoreNode` — Instant, modifies `Nodes` by `+1`
- [x] `GE_ConsumeNode` — Instant, modifies `Nodes` by `-Magnitude`
- [x] `GE_ApplyTempNode` — Duration-based, adds to `TempNodes` for X seconds, removes on expiry
- [x] `GE_HealthThreshold` — Instant, modifies `MaxNodes` by `-1` (applied by attribute set logic at each threshold)
- [x] `GE_PassiveNodeRegen` — Periodic (every 1–2s), modifies `Nodes` by `+1`; apply only when `Nodes == 0` and player is near enemy (controlled via Gameplay Cue or tag condition)
- [x] `GE_Knockback` — Instant, triggers a Gameplay Cue that calls `LaunchCharacter` on the target
- [x] `GE_Kill` — Instant, sets `Health` to `0`

---

## Phase 4 — Data-Driven Combat

### UCombatMoveData (C++)
- [x] Create C++ class `UCombatMoveData` inheriting from `UPrimaryDataAsset`
- [x] Add the following `UPROPERTY` fields:
  - [x] `float Damage`
  - [x] `int32 NodeCost`
  - [x] `int32 NodeGain`
  - [x] `int32 StartupFrames`
  - [x] `int32 ActiveFrames`
  - [x] `int32 RecoveryFrames`
  - [x] `UAnimMontage* AnimationMontage`
  - [x] `UNiagaraSystem* HitEffect`
  - [x] `USoundBase* HitSound`

### Data Assets
Create the following Data Asset instances in `Content/Combat/MoveData/`:
- [x] `DA_BasicAttack`
- [x] `DA_Block`
- [x] `DA_Expel`
- [x] `DA_Rip`
- [x] Fill in all fields for each asset (use placeholder values for now, tune later)

### UCombatMoveRegistry
- [x] Create `UCombatMoveRegistry` as a `UDataAsset` with a `TMap<FGameplayTag, UCombatMoveData*>` property
- [x] Create an instance `DA_MoveRegistry` in `Content/Combat/` and populate it with all four moves mapped to their Gameplay Tags
- [x] Reference `DA_MoveRegistry` from `ACombatCharacter` as an editable property so abilities can look up move data at runtime

---

## Phase 5 — Ability Implementation (GAS)

> Each ability follows this pattern: C++ base class with core logic → Blueprint child for montage/data wiring

### UGA_BasicAttack
- [x] Create C++ class `UGA_BasicAttack` inheriting from `UGameplayAbility`
- [x] In `ActivateAbility()`: look up `DA_BasicAttack` from the registry and play its `AnimationMontage` via `UAbilityTask_PlayMontageAndWait`
- [x] Use `UAbilityTask_WaitGameplayEvent` to listen for the `NotifyState_ActiveFrames` window
- [x] During active frames: trigger hit detection via `UCombatHitDetectionComponent`
- [x] On hit: apply `GE_DamageHealth` to target, apply `GE_RestoreNode` to self
- [x] Apply and clear `State.Combat.Attacking` tag for the duration
- [x] Create Blueprint child `BP_GA_BasicAttack` in `Content/Combat/Abilities/`

### UGA_Block
- [x] Create C++ class `UGA_Block` inheriting from `UGameplayAbility`
- [x] On activation: apply `State.Combat.Blocking` tag, start a timer for the block window (~1 second)
- [x] While `State.Combat.Blocking` is active: intercept incoming `GE_DamageHealth` in `PostGameplayEffectExecute` and negate it
- [x] Detect perfect block: record timestamp of `IA_Block` input; if an incoming hit arrives within N frames of that timestamp, trigger perfect block logic
  - Apply `GE_RestoreNode` to self
  - Apply `GE_Knockback` to all enemies within radius
- [x] After block window: remove `State.Combat.Blocking`, apply cooldown
- [x] Create Blueprint child `BP_GA_Block` in `Content/Combat/Abilities/`

### UGA_Expel
- [x] Create C++ class `UGA_Expel` inheriting from `UGameplayAbility`
- [x] In `CanActivateAbility()`: check that caster has enough Nodes (>= `NodeCost` from move data)
- [x] On activation: apply `GE_ConsumeNode` to self, play montage, run hit detection
- [x] On hit: apply `GE_DamageHealth` to target, apply `GE_ApplyTempNode` to target
- [x] Create Blueprint child `BP_GA_Expel` in `Content/Combat/Abilities/`

### UGA_Rip
- [x] Create C++ class `UGA_Rip` inheriting from `UGameplayAbility`
- [x] In `CanActivateAbility()`: check that caster has enough Nodes
- [x] On activation: apply `GE_ConsumeNode` to self
- [x] Check if target has `State.Status.Vulnerable` tag:
  - **No:** play normal Rip montage, on hit apply `GE_DamageHealth` (minor) and transfer Nodes (apply `GE_ConsumeNode` on target, `GE_RestoreNode` on self for each stolen Node)
  - **Yes:** set Motion Warping target to enemy, play execution montage, on completion apply `GE_Kill` to target
- [ ] Create Blueprint child `BP_GA_Rip` in `Content/Combat/Abilities/`

### Grant Abilities
- [ ] In `ACombatCharacter::PossessedBy()`, grant all four abilities to the ASC using `GiveAbility()`
- [ ] Bind each `IA_*` input action to its corresponding ability's activation tag

---

## Phase 6 — Animation

### Source Assets
- [ ] Download a humanoid character skeleton and animations from **Fab** or **Mixamo** (ensure the skeleton is compatible with UE5's Mannequin if possible)
- [ ] Import assets into `Content/Animations/`
- [ ] Retarget animations to the project skeleton if needed using UE5's IK Retargeter

### Animation Blueprint
- [ ] Create `ABP_CombatCharacter` in `Content/Animations/` using the character skeleton
- [ ] Create Linked Anim Layer interface `ALI_CombatCharacter` with two layers: `LocomotionLayer`, `CombatLayer`
- [ ] Create `ABL_Locomotion` implementing `LocomotionLayer`:
  - [ ] Blend walk/sprint/idle using speed from `CharacterMovementComponent`
  - [ ] Handle jump (in-air blend)
- [ ] Create `ABL_Combat` implementing `CombatLayer`:
  - [ ] Drive upper body overrides from active montages
- [ ] In `ABP_CombatCharacter`, link both layers using `LinkedAnimLayer` nodes
- [ ] Assign `ABP_CombatCharacter` to the `SkeletalMeshComponent` on `BP_Player` and `BP_Enemy`

### ANS_ActiveFrames (Blueprint Anim Notify State)
- [ ] Create Blueprint class `ANS_ActiveFrames` in `Content/Animations/` inheriting from `AnimNotifyState`
- [ ] In `NotifyBegin`: call `UAbilitySystemBlueprintLibrary::SendGameplayEventToActor` on `MeshComp`'s owner, passing tag `Event.ActiveFrames.Begin`
- [ ] In `NotifyEnd`: same call with tag `Event.ActiveFrames.End`
- [ ] Add `Event.ActiveFrames.Begin` and `Event.ActiveFrames.End` to Project Settings → GameplayTags

### Animation Montages
Create in `Content/Animations/Montages/`:
- [ ] `AM_BasicAttack` — place `ANS_ActiveFrames` notify state over the active hit window; add `AnimNotify_SpawnHitEffect`, `AnimNotify_PlayHitSound`
- [ ] `AM_Block` — add notify for block window start/end
- [ ] `AM_PerfectBlock` — add `AnimNotify_SpawnHitEffect`, camera shake trigger
- [ ] `AM_Expel` — place `ANS_ActiveFrames` notify state over the active hit window; add `AnimNotify_SpawnHitEffect`, `AnimNotify_PlayHitSound`
- [ ] `AM_Rip_Normal` — place `ANS_ActiveFrames` notify state over the active hit window; add `AnimNotify_SpawnHitEffect`
- [ ] `AM_Rip_Execution` — add `AnimNotify_EnableMotionWarping`, `AnimNotify_SpawnHitEffect`, `AnimNotify_PlayHitSound`

### Motion Warping
- [ ] Add `UMotionWarpingComponent` to `ACombatCharacter` in C++
- [ ] Before playing `AM_Rip_Execution`, call `MotionWarpingComponent->AddOrUpdateWarpTargetFromTransform()` using the locked-on enemy's transform
- [ ] Add a Motion Warping window to `AM_Rip_Execution` in the Montage editor targeting the warp point

### Hit-Stop
- [ ] Create a C++ helper function `ApplyHitStop(float Duration)` on `ACombatCharacter`
- [ ] In the function: call `UGameplayStatics::SetGlobalTimeDilation(this, 0.05f)`, then use a `FTimerHandle` to restore it to `1.0f` after `Duration` seconds
- [ ] Call `ApplyHitStop` on Perfect Block and Expel hit

---

## Phase 7 — Hit Detection & Collision

### Collision Setup
- [ ] In **Project Settings > Collision**, add a new Object Channel: `HitDetection`
- [ ] Create collision profile `Profile_Character`: blocks `Pawn`, responds to `HitDetection`
- [ ] Create collision profile `Profile_Hitbox`: used for detection sweeps, ignores everything except `Profile_Character`
- [ ] Assign `Profile_Character` to the `CapsuleComponent` on `BP_Player` and `BP_Enemy`

### UCombatHitDetectionComponent (C++)
- [ ] Create C++ component `UCombatHitDetectionComponent`
- [ ] Add `StartTrace()` and `StopTrace()` functions called by ability Active Frame notifies
- [ ] In `TickComponent()` while tracing: perform a box sweep from the weapon bone's previous position to current position
- [ ] Store already-hit actors in a `TArray<AActor*> HitActors` — clear on `StartTrace()`, skip on repeat hits
- [ ] Broadcast an `OnHit` delegate with the hit result for abilities to bind to
- [ ] Attach component to `ACombatCharacter` in C++

### Debug Visualization
- [ ] Add a console variable `combat.ShowHitboxes` (default `0`)
- [ ] When `1`: draw debug boxes each frame during active sweep using `DrawDebugBox()`
- [ ] Color green on miss, red on hit

---

## Phase 8 — UI & HUD

### WBP_HUD
- [ ] Create Widget Blueprint `WBP_HUD` in `Content/UI/`
- [ ] Add a **health bar** (`UProgressBar`): bind fill percent to `Health / MaxHealth`
- [ ] On health change: flash bar white briefly using an animation
- [ ] Add **Node pips**: create a `UHorizontalBox` and dynamically spawn pip widgets based on `MaxNodes`
  - [ ] Regular Node: filled icon
  - [ ] Empty Node: grayed icon
  - [ ] Temp Node: distinct highlighted icon (takes precedence visually over regular pips)
- [ ] Add a **lock-on reticle**: small crosshair or ring widget, set visibility based on `ULockOnComponent::bIsLocked`
- [ ] Bind all values to GAS attribute change delegates (not Tick)
- [ ] Add `WBP_HUD` to the viewport in the Player Controller's `BeginPlay`

### WBP_PauseMenu
- [ ] Create Widget Blueprint `WBP_PauseMenu` in `Content/UI/`
- [ ] Add **Resume** button (removes widget, unpauses game)
- [ ] Add **Move List** panel: loop over `DA_MoveRegistry` entries and create a row widget per move showing name, input, Node cost, and description
- [ ] Add **Settings** button that opens `WBP_Settings` as a child widget
- [ ] Add **Quit** button
- [ ] Toggle pause menu on `Escape` / `Start` button press in Player Controller

### WBP_Settings
- [ ] Create Widget Blueprint `WBP_Settings` in `Content/UI/`
- [ ] Add fullscreen toggle (checkbox or button)
- [ ] Add camera sensitivity sliders (one for mouse, one for controller)
- [ ] Add controller vibration toggle
- [ ] Bind all controls to `UCombatSettings` (Phase 9)

### WBP_DeathScreen
- [ ] Create Widget Blueprint `WBP_DeathScreen` in `Content/UI/`
- [ ] Show on `State.Status.Dead` tag applied to player
- [ ] Add **Restart** and **Quit** buttons

---

## Phase 9 — Settings

- [ ] Create C++ class `UCombatSettings` inheriting from `USaveGame`
- [ ] Add properties: `bool bFullscreen`, `float MouseSensitivity`, `float ControllerSensitivity`, `bool bControllerVibration`
- [ ] Create `UCombatSettingsSubsystem` (Game Instance Subsystem) to load/save settings:
  - [ ] `LoadSettings()` — called on game startup, applies all settings
  - [ ] `SaveSettings()` — called whenever a setting changes
- [ ] Apply fullscreen via `UGameUserSettings::SetFullscreenMode()` + `ApplySettings()`
- [ ] Apply sensitivity by updating the `IMC_Default` axis scalar at runtime
- [ ] Wire `WBP_Settings` controls to `UCombatSettingsSubsystem`

---

## Phase 10 — Polish (SFX, VFX, Juice)

### VFX (Niagara)
Create in `Content/VFX/`:
- [ ] `NS_HitImpact` — burst of particles on hit, vary color/size by surface
- [ ] `NS_PerfectBlock` — radial shockwave burst
- [ ] `NS_Expel` — energy pulse emanating from caster
- [ ] `NS_RipExecution` — energy drain effect pulling from target to caster
- [ ] `NS_NodeGain` — brief upward flash on character
- [ ] `NS_NodeLoss` — brief downward flash on character
- [ ] Hook each system to the corresponding `AnimNotify_SpawnHitEffect` in the relevant montage

### SFX (MetaSound)
Create in `Content/SFX/`:
- [ ] `MS_HitImpact` — layered impact: body thud + material layer (flesh/armor)
- [ ] `MS_PerfectBlock` — sharp crack with a subtle reverb tail
- [ ] `MS_Expel` — whoosh with energy burst layer
- [ ] `MS_Rip` — tearing sound for normal Rip
- [ ] `MS_RipExecution` — cinematic execution sting
- [ ] `MS_UINavigate` — subtle click for menu navigation
- [ ] Hook each sound to `AnimNotify_PlayHitSound` in the relevant montage

### Camera Shakes
Create in `Content/Core/CameraShakes/`:
- [ ] `CS_HitImpact` — short, subtle shake (0.1s, low amplitude)
- [ ] `CS_PerfectBlock` — sharp outward push (0.15s, medium amplitude)
- [ ] `CS_Execution` — slow cinematic roll (0.5s, low frequency)
- [ ] Trigger each shake via `APlayerController::ClientStartCameraShake()` at the appropriate moment

### Knockback
- [ ] In the `GE_Knockback` Gameplay Cue: call `LaunchCharacter()` on the target with a directional impulse away from the caster
- [ ] Expose knockback force as a field on `UCombatMoveData` so it can be tuned per move

---

## Phase 11 — Enemy AI

### ACombatEnemy
- [ ] Create Blueprint `BP_Enemy` (already created in Phase 2) — confirm it inherits `ACombatCharacter` and has all GAS components
- [ ] Create an **AI Controller** `AIC_CombatEnemy` and assign it to `BP_Enemy`
- [ ] Create a **Blackboard** `BB_CombatEnemy` with keys: `TargetActor` (Object), `bCanAttack` (Bool), `DistanceToTarget` (Float)

### Behavior Tree
- [ ] Create **Behavior Tree** `BT_CombatEnemy` in `Content/Characters/Enemy/`
- [ ] Implement the following structure:
  - [ ] **Selector (root)**
    - [ ] **Dead sequence**: check `State.Status.Dead` tag → play death montage → destroy
    - [ ] **Attack sequence**: check `bCanAttack` and `DistanceToTarget < AttackRange` → activate `GA_BasicAttack` → set cooldown on `bCanAttack`
    - [ ] **Chase sequence**: move toward `TargetActor` using `MoveTo` task
    - [ ] **Idle**: play idle animation, look around
- [ ] Create a **BTService** `BTS_UpdateTargetInfo` that runs each tick to update `TargetActor` and `DistanceToTarget`
- [ ] Create a **BTTask** `BTT_ActivateAbility` that triggers a given `UGameplayAbility` on the enemy's ASC
- [ ] In `AIC_CombatEnemy::OnPossess()`, run the behavior tree and populate `TargetActor` with the player

---

## Phase 12 — Debug & QA

### Debug Mode
- [ ] Register console variable `combat.Debug` (default `0`) in a C++ module startup
- [ ] When `1`:
  - [ ] Draw hit detection sweeps each frame (green = miss, red = hit)
  - [ ] Render active Gameplay Tags above each `ACombatCharacter` using `DrawDebugString()`
  - [ ] Render current `Health`, `Nodes`, `TempNodes` values in world space above each character
  - [ ] Print ability activations and Gameplay Effect applications to screen via `GEngine->AddOnScreenDebugMessage()`

### Debug Level
- [ ] Create level `L_Debug` in `Content/Levels/`
- [ ] Place one `BP_Enemy` as a stationary training dummy (disable its AI Controller)
- [ ] Add a `BP_DebugConsole` Blueprint with buttons/key shortcuts for:
  - [ ] Reset player Health and Nodes to max
  - [ ] Grant 5 Nodes instantly
  - [ ] Drain enemy Nodes to 0 (to test Rip execution)
  - [ ] Toggle `combat.Debug`

### QA Checklist
- [ ] Rip on enemy with `>= 1` Node: steals Nodes, deals minor damage
- [ ] Rip on enemy with `0` Nodes (`State.Status.Vulnerable`): triggers execution with Motion Warping
- [ ] Perfect block window timing is consistent and responsive
- [ ] Temp Nodes are consumed before regular Nodes
- [ ] Health at 80% → `MaxNodes` reduced by 1; 60% → -2; 40% → -3; 20% → -4
- [ ] Passive Node regen only triggers when `Nodes == 0` and player is near an enemy
- [ ] Block cooldown prevents immediate re-blocking
- [ ] All abilities work on both keyboard/mouse and controller
- [ ] Settings persist across sessions (close and reopen the game)
- [ ] Death screen appears when player Health reaches 0
- [ ] Lock-on correctly switches targets and clears on target death
