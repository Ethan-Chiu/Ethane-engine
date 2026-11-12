# Engine architecture: Steles, Worlds and Frame Packets

This is the design for how authored content, the running simulation and rendering fit together.
[Roadmap.md](Roadmap.md) tracks which parts are built.

## Overview

```
            commands                      ChangeSets (ordered ops)
 Panels ──────────────▶ STELE ────────────────────────────────┬──────────────▶ network / autosave
 Tools                  immutable snapshots, versions         │
                              │ snapshot (read-only)          ▼
                              └──────▶ panels          WORLD (edit)    WORLD (play)
                                                        runtime storage, systems
                                                              │ extract
                                                              ▼
                                                        FRAME PACKET (immutable) ──▶ Renderer
```

Four layers, each knowing only about the ones below it:

| Layer | Holds | Changes | Knows about |
|---|---|---|---|
| **Schema** | Descriptions of component types: fields, types, flags | At startup (registration) | Nothing |
| **Stele** | Authored data: elements, components, asset references | When someone edits | Schema |
| **World** | The running simulation built from a stele | Every frame | Schema, Stele (ChangeSets) |
| **Frame packet** | Plain data describing what to draw | Rebuilt every frame | Nothing (assets by handle) |

The renderer consumes frame packets only. The editor talks to steles (through commands) and worlds
(through the `World` interface), never to a world's storage.

**Data flows one way.** Only commands change a stele. Worlds never write back to it; the only
exception is an explicit "keep changes from play mode", which itself produces commands.

## Schema

Every component type is a plain C++ struct, described separately in a `TypeRegistry`:

- a stable `TypeId` (hash of the type's name) so saved files and network messages don't depend on
  C++ type identity;
- size, alignment, a version number and construct / copy / destroy functions;
- a list of fields: name, `FieldType`, offset, flags (`RuntimeOnly`, `Hidden`, `ReadOnly`, display
  hints) and metadata (range, drag speed).

Everything generic is written once against the schema: serialization, the inspector, undo, diffs,
network sync and version migration. Adding a component type means one struct plus one registration.

Field types are deliberately limited to values: scalars, glm vectors and quaternions, strings,
`AssetId`, `ElementId` and nested registered structs. Arrays come later.

## Stele

A stele (named after the inscribed stone slabs: the record a world is built from) is one unit of
authored content: a level, a prefab, a preview setup. The things in it are **elements**; a world turns
each element into an entity. A stele holds:

- elements with stable `ElementId`s, each with a name, a parent and an order among its siblings;
- the authored fields of each element's components;
- asset references by `AssetId`;
- (later) prefab instances as a reference plus overrides;
- stele-level settings and the schema versions it was saved with.

It does **not** hold GPU resources, runtime state, or editor state such as the camera, the selection
or the panel layout.

### Snapshots and commits

A stele's state is an immutable **snapshot**. Reading takes a reference-counted snapshot and needs no
locks: panels, autosave, world instantiation and network sync can all read on any thread and always
see a consistent version.

Writing goes through a **transaction**, a list of ops:

```
CreateElement, DestroyElement, SetField(element, component, fieldPath, value),
AddComponent, RemoveComponent, MoveElement(element, newParent, orderKey)
```

`Commit` is the single write point. It validates the ops, builds the next snapshot by copying only the
elements that changed (everything else is shared with the previous snapshot), bumps the version and
publishes a **ChangeSet** (the ops as applied, their inverse and the new snapshot) to subscribers.

Implementation note: elements and components are shared between snapshots, but the index from
`ElementId` to element (and the child lists' index) is copied on every commit, which is O(elements) pointer
copies. That is negligible at editing rates for tens of thousands of elements; if it ever shows up in a
profile, the index can become a persistent hash map without changing the API.

Each committed transaction keeps its inverse ops, which is the undo history. A continuous edit such as
dragging a slider is one transaction that is updated while dragging and committed on release.

### Rules that keep collaborative editing possible

Not implemented yet, but the formats follow these rules from the start:

- `ElementId`s are created without coordination (random 64-bit).
- Ops are fine-grained and safe to apply twice: they set one field, never replace a whole element.
- Sibling order uses fractional order keys, never array indices, so concurrent inserts don't collide.
- Conflicts: the last write to a field (in server order) wins; a delete beats a concurrent edit;
  a move that would create a cycle is rejected at commit.
- Sync model: a server orders ops; clients apply their own ops immediately and rebase unconfirmed ones
  when the server's order arrives. Undo is per user and inverts only that user's ops.

## World

A world is a running simulation of one stele:

```cpp
class World {
public:
    virtual void Apply(const ChangeSet&) = 0;                             // follow the stele
    virtual void Update(const UpdateContext&) = 0;                        // run systems
    virtual void Extract(const ViewDesc&, FramePacketWriter&) const = 0;  // produce what to draw
};
```

The interface is coarse on purpose: no per-component virtual calls, and each implementation stores
data however it likes.

- **Edit world:** always exists for an open stele, follows its ChangeSets and runs only editor systems
  (transform propagation, bounds). Rendering always goes world → frame packet, in edit and play mode.
- **Play world:** instantiated from a stele snapshot (on a background thread if needed), runs gameplay
  systems, and is thrown away on Stop. The stele never changed, so nothing needs restoring.

### The default world uses EnTT, contained

`EnttWorld` stores components in an EnTT registry. EnTT stays an implementation detail:

1. Only `Ethane/World/Entt/` includes `entt.hpp`; no EnTT type appears in any other header.
2. Components are plain structs registered in the schema; references to other elements are `ElementId`s,
   never `entt::entity`.
3. Engine systems (apply, transform propagation, extraction, scripts) are plain functions inside that
   folder that use the registry directly.
4. Panels, the renderer, the serializer and play mode use the stele, the `World` interface or frame
   packets, never world storage.

Replacing EnTT later means rewriting that one folder. A storage-independent system API
(`Query<Read<A>, Write<B>>`, a command buffer) is **not** built yet; it gets added when one of these
happens: gameplay C++ outside the engine, the parallel scheduler (which needs read/write declarations),
or more than about 10–15 engine systems.

## Assets

Steles reference files by `AssetId`, never by path. An `AssetRegistry` maps ids to paths and is saved
as YAML (the editor's is `Ethane-Editor/assets/AssetRegistry.yaml`, committed so ids stay stable), so
moving or renaming a file only changes its registry entry. `AssetRegistry::Import(path)` gives a
file's id, registering it the first time. An `AssetLibrary` loads meshes and textures by id and keeps
them loaded; ids that are unknown or fail to load give null, logged once.

## Frame packet and renderer

A frame packet is plain data with no GPU objects, so any thread can build it:

- the view and the directional lights;
- draw items: a mesh `AssetId`, the world transform, optional material params (base color,
  metallic, roughness, base color texture `AssetId`) and the `ElementId` it came from.

The renderer turns a packet into GPU work for a render target and knows nothing about steles, worlds
or ECS. It resolves asset ids through the `AssetLibrary` and owns the GPU materials: each mesh file's
own, and per-element ones for draw items with material params, which it releases a few frames after the
element stops being drawn (so the frames still in flight can finish with them).

Later: with a render thread, the simulation builds packet N+1 while the renderer records packet N.
Picking draws each item's `ElementId` into an ID buffer. GPU objects in general are destroyed a few
frames late (keyed by frame index) instead of waiting for the device to go idle.

## Engine components

Registered by `RegisterEngineComponents` (the application does it at startup):

| Type name | Fields | Meaning |
|---|---|---|
| `Transform` | Position, Rotation (Euler radians), Scale | Placement relative to the parent element |
| `Mesh` | Mesh (`AssetId`), Visible | Draw this mesh file with its own materials |
| `Material` | BaseColor, Metallic, Roughness, BaseColorTexture | Replace the mesh's material factors |
| `DirectionalLight` | Color, Intensity | Shines along the element's forward axis (-Z after its world rotation) |

## Editor

Panels bind to an `EditorContext`, not to a scene:

- the open steles, each with its edit world and optional play world;
- the selection (a set of `ElementId`s) and the undo history.

| Panel | Reads | Writes |
|---|---|---|
| Hierarchy | A stele snapshot | Commands (create, delete, move) |
| Inspector | The selection, through the schema | `SetField` / add / remove commands |
| Viewport | A world + a camera + a render target | Selection (picking) |
| Stats | Renderer and world statistics | Nothing |

Because the inspector works through the schema, it handles any registered type, including materials
and asset settings, with no per-component UI code.

## Threads

| Thread | Does | Shares data through |
|---|---|---|
| Main / UI | Input, ImGui, building transactions | Commits to steles |
| Jobs | World update, extraction, play-world instantiation | Snapshots in, packets out |
| Render | Recording and submitting GPU work | Immutable frame packets |
| Background | Asset import, autosave, network sync | Snapshots and ChangeSets |

Every hand-off between threads is an immutable value (snapshot, ChangeSet or packet); the only shared
write point is a stele's commit.

## Prior art

- **Bevy:** reflection as the schema, scenes spawned into a separate ECS world, an extract step into a
  render world.
- **Our Machinery:** "The Truth", a copy-on-write object model with undo and collaboration, from which
  runtime entity contexts are built.
- **Figma:** server-ordered, per-property last-writer-wins edits and fractional indexing.
- **Unreal / Unity:** reflection (`UPROPERTY`, `SerializedProperty`) as the schema, and transaction-based
  undo.
