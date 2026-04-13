# Game Mechanics

## Resources

### Health
- Die when reduced to 0
- Reduces max Nodes at certain thresholds (every 20% -> -1 max Node)
- Lower health slows passive Node regeneration

### Nodes
- Consumed to use abilities
- Left vulnerable when reduced to 0 (no gameplay effects)
- Passive regen when at 0 and close to enemy

### Temp Nodes
- Take precedence over regular nodes when consumed
- Last x seconds
- Not affected by max nodes

## Actions

- Walk, Sprint, Jump
- All attacks can be interrupted by other attacks

### Basic Attack
- Free to use
- Regenerates 1 Node on connection with enemy
- Deals minor damage
- Moderate attack speed

### Block
- Lasts for a short duration on use (~1 second)
- Negates next instance of damage taken
- Perfect block when activated right before taking hit
  - Restores 1 Node
  - Prevents knockback on self
  - Causes knockback on nearby enemies (no damage)
- Short cooldown (~1 second)

### Expel
- Consumes x Nodes
- Gives target x Temp Nodes
- Deals moderate damage
- Fast attack speed

### Rip
- Consumes x Nodes
- If enemy has at least 1 Node:
  - Deals minor damage
  - Steals x Nodes
- If enemy has 0 Nodes:
  - Executes them
- Slow attack speed

## Additional Requirements

- Controller support
- SFX, VFX, Juice
- Basic settings (full screen, sensitivity)
- Full move list in pause menu
- Debug mode to show hitboxes and such
- Camera lock on

---

# High Level Implementation

## 1. Data-Driven Systems (Scalability)

Don't hardcode damage or Node costs. Use Data Assets.

- Create a `UCombatMoveData` primary data asset
- Fields: `Damage`, `NodeCost`, `NodeGain`, `StartupFrames`, `RecoveryFrames`, `AnimationMontage`

> This shows you understand how to build tools that allow other designers to balance the game without touching code.

## 2. State Management & Tags

Use Gameplay Tags to manage the combat state.

- `State.Combat.Blocking`
- `State.Status.Vulnerable` (applied when Nodes = 0)

**Interaction Logic:** Instead of complex `if` statements, check tags. Example: If the player triggers a Rip and the target has the `State.Status.Vulnerable` tag, trigger the Motion Warping execution.

## 3. Animation Fidelity (The "Feel")

In AAA, "feel" is everything. Use these UE5 features:

- **Motion Warping:** Use for the Rip execution. Ensures that no matter where the player is standing, they "slide" into the perfect position to align with the enemy's execution animation.
- **Linked Anim Layers:** Keep your Animation Blueprint clean by separating your "Combat Layer" from your "Locomotion Layer."
- **Hit-Stop & Camera Shakes:** When a Perfect Block or Expel lands, freeze the game for 0.05s (3–5 frames) and trigger a subtle camera shake. This provides the "crunchy" feedback expected in modern melee games.
