# Tasks

A phased implementation plan for building UnrealCombat to AAA standards in Unreal Engine 5.

---

## Phase 1 — Project Foundation

- [ ] Create UE5 project using the C++ Third Person template
- [ ] Establish folder structure following AAA conventions:
  ```
  Content/
  ├── Core/          # GameMode, GameInstance, GameState
  ├── Characters/    # Player and Enemy assets
  ├── Combat/        # Abilities, data assets, hit detection
  ├── UI/            # HUD, menus, widgets
  ├── VFX/           # Niagara systems
  ├── SFX/           # MetaSound assets
  ├── Animations/    # Montages, ABPs, Linked Anim Layers
  └── Levels/        # Maps and world partition
  ```
- [ ] Enable required plugins:
  - Gameplay Ability System (GAS)
  - Enhanced Input
  - Motion Warping
  - Game Features (optional, for modularity)
- [ ] Define all Gameplay Tags in a central `GameplayTags.ini`:
  - `State.Combat.Blocking`
  - `State.Combat.Attacking`
  - `State.Status.Vulnerable`
  - `State.Status.Dead`
  - `Ability.BasicAttack`, `Ability.Block`, `Ability.Expel`, `Ability.Rip`
  - `Effect.TempNode`, `Effect.Knockback`, `Effect.HitStop`

---

## Phase 2 — Core Character & Movement

- [ ] Create `ACombatCharacter` base C++ class inheriting from `ACharacter`
  - Implement `UAbilitySystemComponent` and `UAttributeSet`
  - Attach to both Player and Enemy via this base class
- [ ] Set up Enhanced Input
  - Define `IMC_Default` Input Mapping Context
  - Create Input Actions: `IA_Move`, `IA_Look`, `IA_Sprint`, `IA_Jump`, `IA_BasicAttack`, `IA_Block`, `IA_Expel`, `IA_Rip`, `IA_LockOn`
  - Ensure full controller and keyboard/mouse support
- [ ] Implement walk, sprint, and jump using Character Movement Component
- [ ] Set up third-person camera with spring arm
- [ ] Implement **camera lock-on** system
  - Soft-lock targeting: find nearest enemy in cone of view
  - Rotate camera and character to face target while locked
  - Toggle on/off, switch targets with input

---

## Phase 3 — Attribute System (GAS)

- [ ] Create `UCombatAttributeSet` with the following attributes:
  - `Health`, `MaxHealth`
  - `Nodes`, `MaxNodes`
  - `TempNodes` (separate pool, not capped by `MaxNodes`)
- [ ] Implement attribute clamping and change delegates in `PostGameplayEffectExecute`
- [ ] Create Gameplay Effects for each attribute interaction:
  - `GE_DamageHealth` — deals damage
  - `GE_RestoreNode` — adds Nodes
  - `GE_ConsumeNode` — removes Nodes
  - `GE_ApplyTempNode` — grants Temp Nodes with a duration
  - `GE_HealthThreshold` — reduces `MaxNodes` at 20% health intervals
  - `GE_PassiveNodeRegen` — periodic regen, rate scaled by Health

---

## Phase 4 — Data-Driven Combat

- [ ] Create `UCombatMoveData` Primary Data Asset
  - Fields: `Damage`, `NodeCost`, `NodeGain`, `StartupFrames`, `ActiveFrames`, `RecoveryFrames`, `AnimationMontage`, `HitEffect` (Niagara), `HitSound` (MetaSound)
- [ ] Populate a Data Asset for each move: `DA_BasicAttack`, `DA_Block`, `DA_Expel`, `DA_Rip`
- [ ] Create a `UCombatMoveRegistry` (Data Asset or Subsystem) that maps Gameplay Tags to move data — abilities look up their data here rather than having it hardcoded

---

## Phase 5 — Ability Implementation (GAS)

Implement each ability as a `UGameplayAbility` subclass. Each ability should:
- Check and consume required tags/costs via GAS
- Read all tuning values from `UCombatMoveData`
- Apply status tags for the duration of the ability (e.g. `State.Combat.Attacking`)

- [ ] `UGA_BasicAttack`
  - Play montage from move data
  - Fire hit detection (trace or overlap) during Active frames
  - On hit: apply `GE_DamageHealth` and `GE_RestoreNode` (attacker gains 1 Node)
- [ ] `UGA_Block`
  - Apply `State.Combat.Blocking` tag for block window
  - If hit during window: negate damage via `UGE_NegateHit`
  - Detect **perfect block** timing (input within N frames of incoming hit)
    - On perfect: apply `GE_RestoreNode`, apply `GE_Knockback` to nearby enemies
  - Apply cooldown tag after use
- [ ] `UGA_Expel`
  - Cost: consume x Nodes via `GE_ConsumeNode`
  - On hit: apply `GE_DamageHealth` to target, apply `GE_ApplyTempNode` to target
- [ ] `UGA_Rip`
  - Cost: consume x Nodes via `GE_ConsumeNode`
  - Check target for `State.Status.Vulnerable` tag:
    - **Not vulnerable:** apply `GE_DamageHealth` (minor) + `GE_StealNode` (transfers Nodes from target to self)
    - **Vulnerable:** trigger execution — activate Motion Warping, play execution montage, apply `GE_Kill`

---

## Phase 6 — Animation

- [ ] Source or create animation assets (Mixamo or Fab marketplace is fine for prototyping)
- [ ] Build `ABP_CombatCharacter` Animation Blueprint
  - Use **Linked Anim Layers** to separate concerns:
    - `ABL_Locomotion` — walk, sprint, jump, idle
    - `ABL_Combat` — all attack, block, and ability montages
- [ ] Create Animation Montages for each action with clearly named notify sections:
  - `NotifyState_ActiveFrames` — window for hit detection
  - `AnimNotify_SpawnHitEffect` — trigger Niagara VFX
  - `AnimNotify_PlayHitSound` — trigger MetaSound
  - `AnimNotify_EnableMotionWarping` — warp to target during Rip execution
- [ ] Set up **Motion Warping** for the Rip execution
  - Add `MotionWarpingComponent` to `ACombatCharacter`
  - Set warp target to locked-on enemy before montage plays
- [ ] Implement **Hit-Stop**
  - On Perfect Block or Expel hit: call `UGameplayStatics::SetGlobalTimeDilation` to ~0.05 for 3–5 frames, then restore

---

## Phase 7 — Hit Detection & Collision

- [ ] Create a `UCombatHitDetectionComponent` for reusable trace/overlap logic
  - Capsule or box sweep along weapon bone during Active frames
  - Use a dedicated `ECC_HitDetection` collision channel
  - Prevent hitting the same target twice per swing
- [ ] Set up collision profiles:
  - `Profile_Character` — responds to `ECC_HitDetection` and `ECC_Pawn`
  - `Profile_Hitbox` — used by detection sweeps
- [ ] Add debug visualization (lines, spheres) gated behind a console variable `combat.ShowHitboxes 1`

---

## Phase 8 — UI & HUD

- [ ] Create `WBP_HUD` with:
  - Health bar (with damage flash on hit)
  - Node pips (individual icons, not a bar) — grayed out when empty, highlighted for Temp Nodes
  - Lock-on reticle (visible only when locked)
- [ ] Create `WBP_PauseMenu` with:
  - **Full move list** — dynamically populated from `UCombatMoveRegistry` (name, inputs, Node cost, description)
  - Settings page (see Phase 9)
  - Resume / Quit buttons
- [ ] Create `WBP_DeathScreen`
- [ ] Bind all UI to GAS attribute change delegates — never poll in Tick

---

## Phase 9 — Settings

- [ ] Create a `USaveGame`-based `UCombatSettings` class
- [ ] Implement settings:
  - **Fullscreen / Windowed** toggle — `UGameUserSettings`
  - **Camera sensitivity** (separate for controller and mouse)
  - **Controller vibration** toggle
- [ ] Persist settings to disk on change, load on startup

---

## Phase 10 — Polish (SFX, VFX, Juice)

- [ ] **VFX** — create Niagara systems for:
  - Hit impact (per-surface type)
  - Perfect block burst
  - Expel projectile/pulse
  - Rip execution (energy drain effect)
  - Node gain/loss flash on character
- [ ] **SFX** — create MetaSound assets for:
  - Hit impacts (layered: impact body + weapon material)
  - Perfect block (distinct, satisfying crack)
  - Expel and Rip ability activations
  - UI navigation sounds
- [ ] **Camera Shakes** — create `UCameraShakeBase` assets:
  - `CS_HitImpact` — subtle, directional
  - `CS_PerfectBlock` — sharp outward push
  - `CS_Execution` — slow, cinematic roll
- [ ] **Knockback** — apply physics impulse via `LaunchCharacter` on affected enemies; tune per-ability in move data

---

## Phase 11 — Enemy AI

- [ ] Create `ACombatEnemy` inheriting from `ACombatCharacter` (shares all GAS/attributes)
- [ ] Build Behavior Tree with:
  - **Chase** state — move toward player when in range
  - **Attack** state — execute BasicAttack ability, with cooldown
  - **Damaged** state — react to being hit
  - **Dead** state — play death montage, disable collision, destroy
- [ ] Give enemies the same Node system as the player — makes Rip and Expel work on AI without special-casing

---

## Phase 12 — Debug & QA

- [ ] Implement a **Debug Mode** toggled via console variable `combat.Debug 1`:
  - Render hit detection sweeps (colored by hit/miss)
  - Display Gameplay Tags on each character above their head
  - Display current attribute values (Health, Nodes, TempNodes) in world space
  - Log ability activations and Gameplay Effect applications to screen
- [ ] Create a debug level (`L_Debug`) with:
  - A stationary training dummy enemy
  - Console command shortcuts for resetting state, granting Nodes, etc.
- [ ] Playtest all edge cases:
  - [ ] Rip on enemy at exactly 0 Nodes triggers execution
  - [ ] Perfect block window timing feels consistent
  - [ ] Temp Nodes take precedence over regular Nodes
  - [ ] Health thresholds correctly reduce max Nodes
  - [ ] All inputs work on controller and keyboard/mouse
