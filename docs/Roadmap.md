# Roadmap: Steles, Worlds and Frame Packets

Steps toward the design in [Architecture.md](Architecture.md). Each step is usable on its own and lands
as its own commits.

## 1. Schema / type registry ✅

- [x] `TypeId`, `FieldType`, `FieldInfo`, `TypeInfo`, `TypeRegistry`
- [x] Registration API for plain structs, including nested structs
- [x] Type-erased field access (`FieldValue`, field paths) and generic copy / compare
- [x] Unit tests (new `Ethane-Tests` project)

## 2. Stele ✅

- [x] `ElementId`, elements with name / parent / order key / components
- [x] Immutable snapshots with structural sharing
- [x] Transactions, ops, `Commit`, ChangeSets and subscriptions
- [x] Undo / redo from inverse ops; transactions that stay open while dragging
- [x] YAML serializer driven by the schema, with schema versions in the file

## 3. Frame packet and renderer API ✅

- [x] `FramePacket`: draw items, lights, views
- [x] `Renderer::Render(packet, target)`
- [x] Port `DemoScene` to produce packets

## 4. World ✅

- [x] `World` interface: `Apply` / `Update` / `Extract`
- [x] `EnttWorld` (EnTT contained in `Ethane/World/Entt/`)
- [x] Engine components: transform, mesh, material, directional light; transform hierarchy
- [x] Edit world following a stele; `DemoScene` replaced by a stele
- [x] Asset registry (`AssetId` → path, YAML) and asset library; packets reference assets by id
- [x] Old `Ethane/Scene` code removed

## 5. Editor context and panels ✅

- [x] `EditorContext`: open steles, worlds, selection, undo
- [x] Hierarchy panel on stele snapshots (tabs, tree, drag to reparent, create / rename / delete)
- [x] Inspector panel driven by the schema (add / remove components; a drag is one undo step)
- [x] Save / open steles (`assets/steles/`, the editor opens `Demo.stele` at startup)

## 6. Play mode ✅

- [x] Instantiate a play world from a snapshot; discard on Stop
- [x] Camera component (the view in play mode)
- [x] Native script components (`Script`, `ScriptRegistry`, a `ScriptEntity` handle with no EnTT types)
- [x] Worlds store every registered component type, so scripts can use game components

## 7. Threads

- [ ] Job system and a parallel system scheduler
- [ ] Render thread with pipelined frame packets
- [x] Deferred GPU resource deletion (also removes the viewport-resize stall)

## 8. Collaborative editing

- [ ] Server-ordered ops, client rebase, per-user undo

## 9. Own ECS (only if profiling calls for it)

- [ ] A second `World` implementation, validated against `EnttWorld` by comparing frame packets

## Later

- Schema migrations: a per-type hook to convert data saved with an older type version
- Persistent hash map for the stele's element index (only if commits show up in profiles)
- Prefab steles (reference plus overrides)
- Array fields in the schema
- Picking via an ID buffer, gizmos
- Native file dialogs (Open / Save as are path popups for now)
- Asking before closing a stele with unsaved changes
