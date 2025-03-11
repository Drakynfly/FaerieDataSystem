# FaerieDataSystem

A collection of modules implementing a multi-purpose complex inventory, equipment, and crafting system.
FaerieDataSystem (or FDS), is a core plugin developed for Project Faerie, my (Drakynfly) as of yet unreleased personal game project.

Primarily because I am tired of trying to describe to people how to create inventory systems, and since this plugin is fairly applicable to many genres of games, I figured I'd make it public since I don't really care about keeping the workings of my game secret.

FDS is primarily designed for:
- Single-player, local-multiplayer, or limited online multiplayer (2-8 players). This is not a database-friendly inventory for MMOs. (though it could probably made to be with some modification that I don't care to make myself)
- Complex, mutable, or highly specific item logic. This is kinda overkill if all your items consist of is 3-4 floats, and a mesh :)
- The currently implemented equipment system is designed for (A/J)RPG, Survival, or other Action-Adventure genres.
  - For other genres, more customization beyond what I provide will be necessary the further you stray from the norm, although I'd like to believe that FDS is flexible enough for most purposes.

## Modules

### FaerieItemData (+ Editor)
- The core module of FaerieDataSystem, that implements the item data classes, that underpin every other system in the plugins.
  - Important types include: `UFaerieItem`, `UFaerieItemToken`, `IFaerieItemDataProxy`.
  - Items are UObject based, completely modular, and support replication.
  - Item ownership is trackable, and enforced via the use of proxies that respect ownership so-as to only allow permitted access to modification rights. See `IFaerieItemOwnerInterface` for this.
  - Item data is sortable and filterable via a set of classes `UFaerieItemDataFilter` and `UFaerieItemDataComparator`, and `UFaerieItemTemplate` is used to define "abstract" item definitions that can be compared with to describe item "types" such as 'Equipment', 'Consumable', etc.

### FaerieItemGenerator (+ Editor)
- A collection of classes and systems for generating new Items through scriptable crafting systems, including:
  - Weighted table drops for genesis of new items
  - Slot-based crafting systems, combining existing items into new ones
  - Mutation of existing items, upgrading or changing existing items
- All Crafting systems are async, networked, and deterministic.
- This module is the most "sus" piece of work in the plugin. It is in need of some refactoring and has many known bugs.
  - Its also the most complicated, by my standards, due to it being kinda all-powerful.

### FaerieInventory (+ Editor)
- Everything to do with storage of items is implemented here.
  - Important types are `FInventoryEntry`, `FInventoryContent`, and `UFaerieItemContainerBase`.
  - Items are stored efficiently with a custom keying system, and are fully replicated.
  - Item containers are extendable via `UInventoryExtensionBase`, which can add any custom storage behavior, such as weight limits, item type restrictions, custom metadata (e.g favorite items), on a per container basis.

### FaerieInventoryContent (+ Editor)
- A menagerie of extra classes and types that add extra features and extensions to the base Inventory classes.
  - (I am likely going to split this module up later, its kinda a grab bag of extra stuff I make)

### FaerieItemMesh
- A subsystem for creating dynamic meshes, `FFaerieItemMesh`, which represent an item visually in the world.

### FaerieItemCard
- A subsystem for creating UMG widgets for items in a somewhat modular way.

### FaerieEquipment (+ Editor)
- Simple slot-based equipment manager. In an early stage of development, but a solid, simple prototype.

### FaerieDataSystemEditor
- Common shared editor utils. Nothing super important.

## General Notes
- Main compiles against UE 5.5. I will update to future versions as they release. I don't typically support older versions, but previous commits will work on 5.4 through 5.1 with minimal effort.
- This is essentially just the inventory system used by my game, Project Faerie, so there might be *some* lingering inexplicable quirks and oddities in some files. I try to remove these and make this plugin as generic and multi-purpose as a I can, but bare with me as I continue to develop the plugin :/
- FDS is still extremely WIP. Use at your own risk. Nothing is guaranteed to function. Everything is subject to change. This plugin is essentially in Alpha, as it is still in active development.
  - This means two things:
    - Expect bugs. Please let me know of any you find, tho.
    - Please ask me about any missing feature you think might belong in this system. I will either implement it myself, or direct you how you might best implement it yourself.
- Use of this plugins requires decent technical proficiency, both in Blueprint and C++. I will explain anything about the system on my discord, as long as it pertains to something I've done, but I cannot teach you BP, C++, or general "Unreal-isms".
- No documentation is provided (yet). Reading the source code is your friend. I comment everything of note as best I can.
- Content assets are committed via LFS, meaning that this repo *must* be cloned in order to correctly retrieve them. Downloading this repo as a zip file will *not* work.

## Plugin Dependencies

- This plugin depends on two other free plugins I've made, which can be found here:
  - Squirrel - A random-noise generator: https://github.com/Drakynfly/SquirrelUE (This plugin provides the "determinism" that I mention above.)
  - Flakes - A serialization backend: https://github.com/Drakynfly/Flakes
  - These dependencies are annoying, because I prefer to keep my plugins separated, but I don't want to include them as part of FDS itself, because they're off-topic. As some point, I'd like to remove this by changing the internals of FDS to use some sort of feature injection, so that people can use whatever APIs they like. *Another thing on the long long list.*

## Engine Plugin Dependencies
These default plugins are forcibly enabled by FDS:

### GameplayTagsEditor
  - Prerequisite for making children of FGameplayTag.

### GeometryScripting `&&` SkeletalMerging
  - Required for implementing FaerieItemMesh. If, for some reason, you take issue with this requirement, reimplement `UFaerieMeshSubsystem` however you want to.

### UMGWidgetPreview
 - This is an editor-only plugin, used to render UMG widgets inside an asset editor. While this is considered a Beta plugin by Epic, its editor-only, so even if it has issues, they shouldn't effect shipping builds.

## Links
Discord:      [![Discord](https://img.shields.io/discord/996247217314738286.svg?label=&logo=discord&logoColor=ffffff&color=7389D8&labelColor=6A7EC2)](https://discord.gg/AAk9yNwKk8) (Drakynfly's Plugins)

Demo:         https://github.com/Drakynfly/FaerieDataSystemDemo


## Acknowledgments
- [Sythenz's AssetEditorTemplate](https://github.com/Sythenz/AssetEditorTemplate): Adapted for my own uses.


Aaaaand, that's it folks!
