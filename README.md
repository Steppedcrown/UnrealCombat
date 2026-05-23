# UnrealCombat

A high-intensity, third-person tactical melee combat system demo. The game features fluid combo strings, frame-precise blocking, and a unique **Node System** that directly links a fighter's offensive resources to their mortality, culminating in brutal execution finishers.

---

## Core Resource: The Node System

At the heart of combat is the **Node System**, a shared energy mechanic that governs your most powerful abilities and determines when a fighter is vulnerable to being instantly killed.

* **The Pool:** Every fighter starts with a maximum pool of **Nodes** (Default: 4).
* **The Health Attrition Loop:** As you take damage, your capacity to hold power shrinks. For every **20% of maximum health lost**, your maximum Node capacity permanently drops by 1. 
* **The Vulnerable State:** If a fighter's Nodes drop to **0**, they enter a **Vulnerable** state. While Vulnerable, they can be targeted for a cinematic, one-hit execution.
* **Resource Economy:** Basic strikes **restore** Nodes on a successful hit, while special abilities **consume** them.

---

## Combat Mechanics & Moveset

### 1. Basic & Combo Attacks
Your bread-and-butter offensive option. Light attacks naturally chain into a multi-stage combo string. 
* **Input Queuing:** The system features input caching—pressing the attack button slightly early queues up the next strike, ensuring seamless transitions between swings. Missing the timing window resets the combo chain back to the initial strike.

### 2. Charged Heavy Attacks
Holding down the attack input allows you to wind up a devastating heavy strike. Releasing the input at any point immediately unleashes the release blow, allowing you to catch enemies off-guard by varying your attack timing.

### 3. Active Defense: Block & Perfect Block
Defending yourself requires precise timing to turn the tide of battle.
* **Standard Block:** Holding the block button mitigates incoming damage, protecting your health bar at the cost of defensive positioning.
* **Perfect Block:** Activating your block within a split-second window (~0.15 seconds) of an incoming attack triggers a Perfect Block. This rewards your precision by:
    * Completely absorbing the damage.
    * Refilling your **Nodes**.
    * Unleashing a shockwave that knocks back all surrounding enemies.

### 4. Expel (Special Ability)
* **Cost:** Consumes Nodes.
* **Effect:** A heavy-hitting strike that deals direct damage and inflicts **TempNodes** onto the target. 
* **Tactical Use:** TempNodes act as an artificial buffer that inflates the enemy's apparent resource pool. This forces them closer to bankruptcy and sets them up perfectly for a follow-up *Rip* ability.

### 5. Rip & Execution (Finisher)
A high-stakes ability that changes behavior drastically depending on the target's status:

| Target Status | Combat Outcome |
| :--- | :--- |
| **Normal State** | Deals minor damage and **drains all remaining Nodes** from the target, transferring them directly to you. |
| **Vulnerable State** (0 Nodes) | Triggers an **Instant Kill Finisher**. If you are locked onto the target, the camera snaps into a cinematic view and your character automatically leaps into position to execute the enemy. |

### 6. Target Lock-On
Toggling the lock-on system automatically targets the closest enemy within a forward cone of vision. While locked on:
* The camera dynamically tracks the target's movements.
* Your character maintains proper spacing and orientation.
* The cinematic *Rip* execution becomes fully available if the target's **Nodes** drops to 0.
