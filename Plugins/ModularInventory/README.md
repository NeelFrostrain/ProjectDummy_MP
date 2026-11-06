# Modular Inventory (v2)

**One component. Any use.** `Item Container` holds N slots of items. Put it on anything:
a backpack, a hotbar, a chest, an equipment slot, a vendor. They all work the same, you only change the settings.

Server-authoritative, multiplayer-ready, no GAS needed.

---

## Install (2 minutes)

1. Copy the `ModularInventory` folder into your project's `Plugins/` folder. Regenerate project files, build.
2. If you use C++, add `"ModularInventory"` to your Build.cs dependencies.
3. Done. The plugin has no other setup.

## The 4 building blocks

| Thing | What it is |
|---|---|
| **Item Definition** (Data Asset) | One per item type: name, icon, stack size, tags, optional use action |
| **Item Container** (Component) | Holds slots. Add it to any actor. |
| **Item Pickup** (Actor) | An item lying in the world |
| **Item Container Widget** + **Item Slot Widget** | UI that shows any container |

---

## Recipes

### A player with a backpack and a hotbar
Add **two** Item Container components to your Character (or PlayerController / PlayerState, anywhere works):

| | Backpack | Hotbar |
|---|---|---|
| Container Name | `Backpack` | `Hotbar` |
| Num Slots | 28 | 6 |
| Allow Selection | off | **on** |

Hotbar input in Blueprint: key `1` -> `Select Slot (0)`, mouse wheel -> `Select Next` / `Select Previous`, left click -> `Use Selected Item`, `Q` -> `Drop Selected Item`.

### Show a container on screen (3 steps)
1. Create a Widget Blueprint, parent class **Item Slot Widget**. Add an Image named `IconImage` and a Text named `CountText`. (Optional: Text `KeyText`, any widget `SelectionHighlight`.)
2. Create a Widget Blueprint, parent class **Item Container Widget**. Add a **Uniform Grid Panel** named `SlotPanel`. Set `Slot Widget Class`, `Columns`, and `Auto Find Container Name` (`Hotbar`).
3. Create it and add to viewport. It finds the local player's container by itself.

For a second container (backpack) make another instance of the same widget with name `Backpack`.

### A chest
1. Chest actor (must replicate) + Item Container component:
   `Replication Mode = Everyone`, `Allow External Access = on`, fill `Starting Items`.
2. When the player interacts: create your Container Widget, call `Set Container (ChestContainer)`, then `Set Quick Transfer Target (PlayerBackpack)`.
3. Drag & drop and Shift+click now move items between chest and backpack. Distance and permission are checked on the server.

### An equipment slot
Container with `Num Slots = 1` and `Allowed Item Tags = Item.Weapon`. Only weapons can be dropped into it.

### Picking things up
- From a trace / overlap / interact key: `Container -> Pickup Actor (HitActor)`
- Nearest item around you: `Container -> Pickup Nearby (Radius)`
- Walk-over pickup: tick `Auto Pickup` on the Item Pickup in the level.

### Making an item do something
- **Simple:** bind to the container's `On Item Used` event (server) and check which item it was.
- **Reusable:** make a Blueprint subclass of **Item Action**, override `On Use`, put it in the item's `Use Action`. Return true if it worked.
- Tick `Consume On Use` on the item to remove 1 after a successful use.

### Crafting
```
Has Items(Cost)  ->  Can Fit(Result)  ->  Remove Items(Cost)  ->  Add Item(Result)
```
Run it on the server (your own Server RPC) - `Cost` is an array of `Item Stack`.

---

## API cheat sheet

**Server only** (do nothing on clients): `Add Item`, `Add Item To Slot`, `Remove Item`, `Remove From Slot`, `Remove Items`, `Clear All`, `Set Num Slots`

**Client or Server** (clients are sent to the server automatically): `Move Item`, `Drop Item`, `Use Item`, `Sort Items`, `Pickup Actor`, `Pickup Nearby`, `Select Slot / Next / Previous`

**Safe anywhere:** `Get Slot`, `Get Item Count`, `Has Item`, `Has Items`, `Find Slot With Item`, `Find Empty Slot`, `Can Fit`, `Get Free Space For`, `Is Full`, `Get Num Slots`

**Events:** `On Slot Changed` (server + clients), `On Container Changed`, `On Selection Changed`, `On Item Used` (server)

**Find a container:** `Find Item Container (Pawn / Controller / PlayerState, Name)` searches all three.

`Move Item (FromSlot, ToContainer, ToSlot, Count)` is the one function behind everything:
- different container = transfer, same container = rearrange
- `ToSlot -1` = automatic, `Count 0` = whole stack
- partial `Count` into an empty slot = split a stack
- target slot has a different item = swap (whole stacks only)

## Custom rules in Blueprint
Make a Blueprint child of Item Container and override:
- `Can Accept Item` - weight limits, level requirements...
- `Can Be Accessed By` - locked chests, team-only containers...

## Networking notes
- `Owner Only` containers replicate only to the owning player. `Everyone` replicates to every client that sees the actor.
- For a request on a container the player does not own (a chest), the client sends it through one of their own containers, so the local player needs at least one container.
- Every request is validated on the server (slot, item, access, distance).
- `Use Action` and `On Item Used` run on the server only.

## Differences from v1
- No more fixed `Inventory` / `Hotbar` enum. Any number of containers, found by name.
- No GAS dependency. Use `On Item Used` or Item Actions.
- Hotbar selection is a checkbox (`Allow Selection`), not a separate system.
- Slot widgets use fixed optional names (`IconImage`, `CountText`...) instead of guessing names.
- Item Data -> Item Definition, World Inventory Item -> Item Pickup. Data assets and Blueprints from v1 need to be recreated.

## Known limits
- Fixed slot grid (no weight or tetris-style grids).
- Shift+click needs `Set Quick Transfer Target` to know where to send items.
- Dropping an item on empty UI space (not on a slot) throws it into the world. Turn off with `Drop To World If Cancelled` on the slot widget.
