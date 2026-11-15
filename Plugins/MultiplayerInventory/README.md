# Modular Multiplayer Inventory & Hotbar System (UE 5.3 - 5.8)

A high-performance, server-authoritative multiplayer inventory and hotbar system for Unreal Engine. Built with **Fast Array Replication (`FFastArraySerializer`)**, **Gameplay Ability System (GAS)** integration, **Native Gameplay Tags**, modular tag-driven abilities, and plug-and-play **UMG Widget base classes**.

---

## Key Highlights

- **Server-Authoritative Networking**: Secure validation for pickup distances, stack operations, item usage, crafting, and drops.
- **Fast Array Replication**: Private inventory contents replicate with `COND_OwnerOnly` using efficient delta serialization without sending the whole array every update.
- **Configurable Dynamic Slot Counts**: Both `HotbarSlotCount` (default: 6) and `InventorySlotCount` (default: 28) can be modified directly in the Details panel, in Blueprints, or dynamically resized at runtime via `SetSlotCount()`.
- **Unified Event Dispatcher (`OnItemEvent`)**: Replaced messy separate delegates with a single, clean event dispatcher driven by Native Gameplay Tags (`Inventory.Event.*`).
- **100% Tag-Driven Gameplay Abilities**: Subclass `UInventoryItemGameplayAbility` for weapons, potions, tools, or throwables. Abilities return `bool` to indicate success/failure and authoritatively call `ConsumeSourceItem()` when needed.
- **Direct Blueprint Branch Nodes**: All action functions return `bool` (or `int32` for index selectors) so you can connect a **Branch** node directly in Blueprint graphs.
- **UMG Base Classes & Array Setup Helpers**: `UInventorySlotWidget` and `UHotbarWidget` with built-in drag-and-drop, hit-test invisible drag visuals, outside-drop-to-world handling, and one-node array-to-slot mapping (`SetupHotbarSlotWidgets`).

---

## Table of Contents
1. [Installation & Setup](#installation--setup)
2. [Slot Count Configuration](#slot-count-configuration)
3. [Creating Item Data Assets](#creating-item-data-assets)
4. [Modular Gameplay Ability Integration (GAS)](#modular-gameplay-ability-integration-gas)
5. [Unified Event Dispatcher (`OnItemEvent`)](#unified-event-dispatcher-onitemevent)
6. [UMG & UI Integration](#umg--ui-integration)
   - [The Array-to-Slot Index Setup (6-Slot Solution)](#the-array-to-slot-index-setup-6-slot-solution)
   - [Drag and Drop Handling](#drag-and-drop-handling)
7. [Hotbar Controls: Selecting vs. Executing](#hotbar-controls-selecting-vs-executing)
8. [World Pickups & Interaction](#world-pickups--interaction)
9. [Blueprint API Reference](#blueprint-api-reference)
10. [Multiplayer & Replication Architecture](#multiplayer--replication-architecture)

---

## Installation & Setup

1. Copy the `MultiplayerInventory` folder into your project's `Plugins/` directory.
2. In Unreal Editor, go to **Edit > Plugins**, enable **Multiplayer Inventory**, and restart the editor.
3. In your project's `[ProjectName].Build.cs`, add `MultiplayerInventory` to `PublicDependencyModuleNames`:
   ```csharp
   PublicDependencyModuleNames.AddRange(new string[] {
       "Core", "CoreUObject", "Engine", "InputCore",
       "GameplayTags", "GameplayAbilities", "GameplayTasks",
       "UMG", "Slate", "SlateCore",
       "MultiplayerInventory"
   });
   ```
4. Add `UInventoryComponent` to your player Character or Pawn:
   - **In Blueprint**: Open your Character Blueprint, click **Add Component**, and search for **Inventory Component**.
   - **In C++**:
     ```cpp
     UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Inventory")
     TObjectPtr<UInventoryComponent> InventoryComponent;
     ```
5. Ensure your Character replicates (`bReplicates = true`) and has an `AbilitySystemComponent` (either on the Character, Pawn, or PlayerState).

---

## Slot Count Configuration

Slot counts are no longer hardcoded. You have full control over the size of both containers:

### 1. In Details Panel
Select the `InventoryComponent` in your Character Blueprint:
- **`HotbarSlotCount`**: Set to any desired number (default: **6**, matching typical 1-6 key layouts).
- **`InventorySlotCount`**: Set to any desired capacity (default: **28**, matching 7x4 grids).

### 2. Runtime Dynamic Resizing (Server Authority)
```cpp
// Change inventory capacity dynamically (e.g. backpacks, upgrades):
InventoryComponent->SetSlotCount(EInventoryContainer::Inventory, 35);
```
In Blueprint, call **Set Slot Count** passing the container and new size.

---

## Creating Item Data Assets

Items are defined as Data Assets derived from `UInventoryItemData`:

1. Right-click in Content Browser > **Miscellaneous > Data Asset**.
2. Select **InventoryItemData** (or your custom subclass).
3. Configure the item properties:
   - **DisplayName**: User-facing item name (e.g. `"Health Potion"`).
   - **Description**: Item tooltip description.
   - **Icon**: `UTexture2D` icon used in slot widgets.
   - **MaxStackSize**: Maximum items per slot (e.g. `1` for weapons, `64` for consumables/materials).
   - **ItemTags**: Gameplay Tags identifying item categories (e.g. `Item.Type.Weapon`, `Item.Type.Consumable`, `Item.Type.Throwable`).
   - **AbilityClass**: Blueprint derived from `UInventoryItemGameplayAbility`.
   - **WorldMesh** / **WorldActorClass**: Mesh or actor spawned when dropped into the world.
   - **bCanBeDropped**: Controls whether this item can be dropped into the world.
   - **bConsumeOnUse**: If `true` (default), using this item (when `OnItemUsed` returns `true`) automatically consumes 1 from its stack. Uncheck for reusable items like weapons or permanent tools.

---

## Modular Gameplay Ability Integration (GAS)

Item behavior is **modular** and driven by **Gameplay Tags** inside Gameplay Abilities. The inventory component does not hardcode consumable or weapon behavior; instead, your ability dictates what happens and when items are consumed.

### 1. Create Ability Blueprint
Create a Blueprint based on `UInventoryItemGameplayAbility` (e.g. `GA_HealthPotion` or `GA_Rifle`).

### 2. Ability Properties & Context
- **`ItemData`**: Defaults to the assigned Data Asset.
- **`RequiredItemTags`** / **`BlockedItemTags`**: Enforce tag prerequisites on activation.
- **`GetSourceItemTags()`**: Access tags on the activating item.
- **`GetSourceItemData()`**: Access the exact `UInventoryItemData` asset.
- **`GetSourceInventoryComponent()`**: Access the owning player's inventory component.

### 3. Implement Events with Boolean Return Values
Override these functions in your Ability Blueprint graph:

| Event | Parameters | Return | Description |
|---|---|---|---|
| **`OnItemSelected`** | `ItemData`, `ItemTags` | `bool` | Fired on server when the item is selected/equipped in the hotbar. Return `true` if equip succeeded (e.g. socket attached). |
| **`OnItemUnselected`** | `ItemData`, `ItemTags` | `bool` | Fired on server when hotbar selection changes away. Return `true` if unequip succeeded (e.g. mesh detached). |
| **`OnItemUsed`** | `ItemData`, `ItemTags` | `bool` | Fired on server when the player presses attack/use. Return `true` if activation was successful. |

> [!IMPORTANT]
> If `OnItemUsed` returns `false` (e.g. player is stunned, out of mana, or weapon is on cooldown), the inventory execution flow aborts and returns `false`, preventing false consumption or unwanted animations.

### 4. Automatic vs. Manual Consumption & Disappearance
- **Automatic Disappearance on Use**:
  When `OnItemUsed` returns `true`:
  - If `ItemData->bConsumeOnUse` is `true` (default), **1 item is automatically consumed** from the slot stack.
  - When the slot's stack quantity reaches `0`, the slot is cleared (`ItemData = nullptr`), the ability is automatically unselected/cancelled, `OnInventoryChanged` replicates to client, and the item **cleanly disappears from the hotbar and UI**.
  - For weapons, shields, or reusable tools: simply set **`bConsumeOnUse = false`** on their `UInventoryItemData` asset!
- **Manual Consumption inside Ability**:
  If your ability needs custom consumption timing (e.g., consume after a 2-second montage or when throwing a projectile), you can call:
  ```text
  ConsumeSourceItem(Amount = 1)   // or RemoveSourceItem(1)
  ```
  inside your ability graph. The system automatically detects that the ability already consumed the item and will avoid double-consumption when `OnItemUsed` finishes.

---

## Dedicated Event Dispatchers

The component provides clean, dedicated event dispatchers in the Details panel so you can bind directly to specific actions without needing to filter through every event:

All dispatchers share the **exact same 6 pins**:
- **`ItemData`** (`UInventoryItemData*`): The item definition asset.
- **`Quantity`** (`int32`): The number of items involved.
- **`Container`** (`EInventoryContainer`): Hotbar or Inventory.
- **`SlotIndex`** (`int32`): The slot index where the action took place.
- **`ItemTags`** (`FGameplayTagContainer`): Tags defined on the item (e.g. Weapon, Potion).
- **`RelatedActor`** (`AActor*`): Related actor (e.g. the world pickup item actor, or the player).

| Event Dispatcher | Description | RelatedActor Pin |
|---|---|---|
| **`OnItemSelected`** | Fired when an item in the hotbar is selected | Player Pawn |
| **`OnItemUnselected`** | Fired when an item in the hotbar is unselected | Player Pawn |
| **`OnItemUsed`** | Fired when an item's ability or action is executed/used | Player Pawn |
| **`OnItemPickedUp`** | Fired when an item is picked up from the world | The `AWorldInventoryItem` actor |
| **`OnItemDropped`** | Fired when an item is dropped into the world | The spawned `AWorldInventoryItem` actor |
| **`OnItemAdded`** | Fired whenever items enter any container | Player Pawn |
| **`OnItemRemoved`** | Fired whenever items are removed from any container | Player Pawn |

> [!NOTE]
> **Unified `OnItemEvent`**: The component also provides `OnItemEvent(EventTag, ItemData, Quantity, Container, SlotIndex, ItemTags, RelatedActor)` if you prefer handling all events through a single node with "Switch on Gameplay Tag".
> 
> **Clean Terminology**: "Equip", "Unequip", and "Consume" have been replaced with modular **Select**, **Unselect**, and **Use** (plus **Pick** and **Drop**), keeping the inventory pure and decoupling cosmetic sockets or weapon logic into your own event handlers or abilities.

---

## UMG & UI Integration

### The Array-to-Slot Index Setup (6-Slot Solution)

When designing a hotbar or inventory in UMG, designers manually place slot widgets in a Horizontal Box or Grid (e.g. `WBP_InventorySlot1` through `WBP_InventorySlot6`).

Instead of guessing slot counts or manually configuring every slot, use the built-in array setup helper:

#### In your Inventory/HUD Widget Blueprint (`WBP_Inventory`):
1. In the **Event Construct** or **Initialize**:
2. Get your `UInventoryComponent`.
3. Create a **Make Array** node containing your slot widget references:
   - `[0] = WBP_InventorySlot1`
   - `[1] = WBP_InventorySlot2`
   - `[2] = WBP_InventorySlot3`
   - `[3] = WBP_InventorySlot4`
   - `[4] = WBP_InventorySlot5`
   - `[5] = WBP_InventorySlot6`
4. Call **`Setup Hotbar Slot Widgets`** (or `SetupSlotWidgets`):

```
[Event Construct]
     │
     ▼
[Get Player Inventory Component]
     │
     ▼
[Setup Hotbar Slot Widgets]
     ├── Target: Inventory Component
     └── InWidgets: [Make Array: Slot1, Slot2, Slot3, Slot4, Slot5, Slot6]
```

#### What `SetupHotbarSlotWidgets` Does Automatically:
- Loops over the array: assigns `SlotIndex = Array Index` (0 through 5).
- Sets `Container = EInventoryContainer::Hotbar`.
- Assigns the `InventoryComponent` reference to each slot.
- Automatically syncs `InventoryComponent->HotbarSlotCount = SlotWidgets.Num()` (sets it to 6).
- Calls `RefreshSlot()` on each widget to display current items, icons, and counts.

---

### Drag and Drop Handling

The plugin includes built-in drag-and-drop powered by `UInventorySlotWidget` and `UInventoryDragDropOperation`:

1. **Reparent Slot Widget**: Reparent your slot widget Blueprint (`WBP_InventorySlot`) to **`UInventorySlotWidget`**.
2. **Bind Optional Widget Names** (in Designer):
   - `ItemIcon` (`UImage`): Auto-displays item icon texture.
   - `QuantityText` (`UTextBlock`): Auto-displays stack quantity (auto-hidden when quantity <= 1).
   - `KeyNumberText` (`UTextBlock`): Auto-displays hotbar slot numbers (1-6).
   - `HighlightBorder` (`UBorder`): Auto-toggled when slot is selected.
3. **Built-in Hit-Test Invisible Drag Visual**: The dragging thumbnail is rendered with a dedicated `UImage` set to `HitTestInvisible`. It will **never block** mouse drop hit-testing on the target slot beneath it.
4. **Drop Outside to World (`bDropToWorldOnDropOutside`)**:
   - If a player drags an item and releases it outside any valid UI slot (drag cancelled), `UInventoryDragDropOperation` automatically calls `RequestDrop` on the server, spawning the item into the world at the player's feet.

---

## Hotbar Controls: Selecting vs. Executing

The system provides clean separation between **selecting** an item (equipping) and **executing** an item (using/attacking):

```
                    ┌─────────────────────────┐
                    │ Hotbar Key Pressed (1-6)│
                    └────────────┬────────────┘
                                 │
                                 ▼
                     [SelectHotbarSlot(Index)]
                                 │
                    Calls OnItemSelected (GAS)
                    Broadcasts Inventory.Event.Equipped
                                 │
                                 ▼
                    ┌─────────────────────────┐
                    │  Left Click / Attack    │
                    └────────────┬────────────┘
                                 │
                                 ▼
                   [ExecuteSelectedHotbarItem()]
                                 │
                    Calls OnItemUsed (GAS)
                    Broadcasts Inventory.Event.Used
```

### Hotbar Input Action Setup in Character Blueprint:

1. **Slot Selection (1-6 Keys)**:
   - Key 1 ──► `SelectHotbarSlot(0)`
   - Key 2 ──► `SelectHotbarSlot(1)`
   - Key 3 ──► `SelectHotbarSlot(2)`
   - Key 4 ──► `SelectHotbarSlot(3)`
   - Key 5 ──► `SelectHotbarSlot(4)`
   - Key 6 ──► `SelectHotbarSlot(5)`
2. **Cycle Selection (Mouse Wheel)**:
   - Wheel Up ──► `SelectPreviousHotbar()`
   - Wheel Down ──► `SelectNextHotbar()`
3. **Execute Item (Primary Action / Left Click)**:
   - Primary Action ──► `ExecuteSelectedHotbarItem()` (returns `bool` for Branch)
4. **Drop Selected (Q Key)**:
   - Drop Key ──► `DropSelectedHotbarItem(1)`

---

## World Pickups & Interaction

Items in the world are instances of `AWorldInventoryItem`. The plugin provides 1-node pickup helpers that handle line traces, sphere sweeps, and server replication:

### 1-Node Pickup Functions

| Function | Category | Description |
|---|---|---|
| **`TryPickupActor(TargetActor)`** | Pickup | Pass any actor hit by line trace, sphere trace, or interact trigger. Returns `true` if picked up. |
| **`TryPickupInFront(Distance, Radius)`** | Pickup | Sweeps forward from player eyes/camera (default 500 units) to find and pick up the closest item. |
| **`TryPickupNearby(Radius)`** | Pickup | Checks around player within radius (default 300 units) and picks up the closest item. |
| **`RequestPickup(WorldItem)`** | Pickup | Direct pickup request passing `AWorldInventoryItem*`. Server validates distance <= `MaxPickupDistance`. |

> [!TIP]
> If the player's inventory only has space for part of a dropped stack (e.g. room for 15 out of 50 items), the server adds 15 to the inventory and leaves the remaining 35 in the world item actor.

---

## Blueprint API Reference

All action functions return `bool` so you can directly connect Blueprint **Branch** nodes.

### Inventory Setup & Configuration
- **`SetSlotCount(Container, NewCount)`** *(Authority Only)*: Resizes container dynamically.
- **`SetupHotbarSlotWidgets(InWidgets)`**: Configures array of slot widgets, assigning `SlotIndex = Array Index` and syncing hotbar capacity.
- **`SetupSlotWidgets(Container, InWidgets)`**: Configures array of slot widgets for any container.

### Queries (Pure / Const)
- **`GetSlotCount(Container)`** ➔ `int32`: Returns capacity (e.g. 6 for Hotbar, 28 for Inventory).
- **`GetUsedSlotCount(Container)`** ➔ `int32`: Returns count of non-empty slots.
- **`GetSlot(Container, Index, OutItemData, OutQuantity)`** ➔ `bool`: Reads slot contents.
- **`GetAllSlots(Container)`** ➔ `TArray<FInventorySlot>`: Returns copy of all slot structs.
- **`GetSelectedHotbarIndex()`** ➔ `int32`: Current active hotbar slot index (0 to N-1).
- **`GetSelectedHotbarItem(OutItemData, OutQuantity)`** ➔ `bool`: Retrieves active hotbar item.
- **`HasItem(ItemData, RequiredQuantity)`** ➔ `bool`: Checks if player owns at least N items across both containers.
- **`GetTotalItemCount(ItemData)`** ➔ `int32`: Total quantity owned across both containers.
- **`CanAddItem(ItemData, Quantity)`** ➔ `bool`: Validates if items will fit without overflowing.
- **`GetFreeSpaceForItem(ItemData, Container)`** ➔ `int32`: Remaining capacity for this item.
- **`GetContainerFillRatio(Container)`** ➔ `float`: Percentage full (0.0 to 1.0).

### Hotbar Actions
- **`SelectHotbarSlot(Index)`** ➔ `bool`: Selects slot index, activating its ability and equipping the item.
- **`SelectNextHotbar()`** ➔ `int32`: Advances selection forward and returns new index.
- **`SelectPreviousHotbar()`** ➔ `int32`: Advances selection backward and returns new index.
- **`ExecuteSelectedHotbarItem()`** ➔ `bool`: Activates item ability/effect.
- **`DropSelectedHotbarItem(Amount)`** ➔ `bool`: Drops item from active hotbar slot.
- **`TossSelectedItem(ForwardForce, Amount)`** ➔ `bool`: Throws active hotbar item with physics impulse.

### Slot & Container Management
- **`ExecuteSlotItem(Container, SlotIndex)`** ➔ `bool`: Executes item from any slot.
- **`RemoveSelectedHotbarItem(Amount)`** ➔ `bool`: **Manually removes/decrements** the currently selected hotbar item (callable on Client or Server).
- **`RemoveSlotItem(Container, SlotIndex, Amount)`** ➔ `bool`: **Manually removes/decrements** an item from any slot (callable on Client or Server).
- **`ConsumeSelectedHotbarItem(Amount)`** ➔ `bool`: Decrements active hotbar item stack (callable on Client or Server).
- **`ConsumeSlotItem(Container, SlotIndex, Amount)`** ➔ `bool`: Decrements item stack from specific slot (callable on Client or Server).
- **`ClearSelectedHotbarSlot()`** ➔ `bool`: Empties the selected hotbar slot completely (callable on Client or Server).
- **`ClearSlot(Container, Index)`** ➔ `bool`: Empties a specific slot completely (callable on Client or Server).
- **`RemoveItem(ItemData, Quantity)`** ➔ `int32`: Removes a quantity of an item across all containers (callable on Client or Server).
- **`QuickTransferSlot(FromContainer, FromIndex)`** ➔ `bool`: Shift-click transfer between Hotbar <-> Inventory.
- **`SwapSlots(ContainerA, IndexA, ContainerB, IndexB)`** ➔ `bool`: Swaps two slots.
- **`RequestMove(FromContainer, FromIndex, ToContainer, ToIndex)`** ➔ `bool`: Moves, merges, or swaps slots.
- **`RequestSplit(Container, FromIndex, ToIndex, Amount)`** ➔ `bool`: Splits stack into an empty slot.
- **`RequestDrop(Container, SlotIndex, Amount)`** ➔ `bool`: Drops amount into the world.
- **`RequestDropAll(Container)`** ➔ `bool`: Drops all items in container into the world.
- **`RequestSort(Container)`** ➔ `bool`: Merges matching stacks and sorts alphabetically.
- **`ClearContainer(Container)`** ➔ `bool` *(Authority Only)*: Empties container.

### Slot Widget Actions (`UInventorySlotWidget`)
- **`RemoveItem(Amount)`** ➔ `bool`: Directly decrements/removes this slot widget's item (callable on Client or Server).
- **`ClearSlot()`** ➔ `bool`: Directly clears this slot widget (callable on Client or Server).
- **`DropSlotItem(Amount)`**: Drops item from this slot widget into the world.

### Crafting
- **`CanCraftRecipe(Ingredients, Quantities)`** ➔ `bool`: Checks recipe prerequisites.
- **`RequestCraft(Ingredients, Quantities, ResultItem, ResultQuantity)`** ➔ `bool`: Verifies ingredients, consumes them, and grants result item.

### Blueprint Function Library (`UInventoryBlueprintLibrary`)
- **`GetPlayerInventoryComponent(ContextActor)`** ➔ `UInventoryComponent*`: Finds component on Pawn or Controller.
- **`FormatItemQuantity(Quantity)`** ➔ `FText`: Clean numbers (`500` -> `"500"`, `1500` -> `"1.5K"`, `2500000` -> `"2.5M"`).
- **`BreakInventorySlot(Slot, OutIndex, OutItemData, OutQuantity, bOutIsEmpty)`**: Pure break node.
- **`MakeInventorySlot(Index, ItemData, Quantity)`**: Pure make node.
- **`FilterSlotsByItem(Slots, ItemData)`** ➔ `TArray<FInventorySlot>`: Filters slot array.
- **`FindEmptySlotIndices(Slots)`** ➔ `TArray<int32>`: Returns empty indices.
- **`SetupSlotWidgets(InventoryComp, Container, InWidgets)`**: Static slot array initialization helper.
- Native Event Tag Getters: `GetEventTag_Equipped()`, `GetEventTag_Unequipped()`, `GetEventTag_Dropped()`, `GetEventTag_Added()`, `GetEventTag_Removed()`, `GetEventTag_Used()`, `GetEventTag_Consumed()`.

---

## Multiplayer & Replication Architecture

### Fast Array Serialization (`FFastArraySerializer`)
Private inventory contents are stored in `FInventoryArray` (`InventorySlots` and `HotbarSlots`) which inherit from `FFastArraySerializer`.
- **Bandwidth Efficiency**: Only modified slots are replicated over the network.
- **`COND_OwnerOnly`**: Inventory contents only replicate to the owning client. Other players never receive another player's private inventory list.

### Server Validation Flow
1. Client requests an action (move, split, drop, use, craft) via Server RPC.
2. Server validates ownership, slot indices, item tags, and distance (for pickups).
3. Server executes state modification and marks fast array items dirty.
4. Server broadcasts `OnItemEvent` with appropriate Gameplay Tag and context.
5. Replicated fast array changes trigger `OnInventoryChanged` on the owning client, triggering automatic UI refreshes.
