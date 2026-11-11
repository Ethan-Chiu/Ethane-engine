# Roadmap: Bricks, Worlds and Frame Packets

Steps toward the design in [Architecture.md](Architecture.md). Each step is usable on its own and lands
as its own commits.

## 1. Schema / type registry

- [ ] `TypeId`, `FieldType`, `FieldInfo`, `TypeInfo`, `TypeRegistry`
- [ ] Registration API for plain structs, including nested structs
- [ ] Type-erased field access (`FieldValue`, field paths) and generic copy / compare
- [ ] Unit tests (new `Ethane-Tests` project)

## 2. Brick

- [ ] `ObjectId`, objects with name / parent / order key / components
- [ ] Immutable snapshots with structural sharing
- [ ] Transactions, ops, `Commit`, ChangeSets and subscriptions
- [ ] Undo / redo from inverse ops; transactions that stay open while dragging
- [ ] YAML serializer driven by the schema, with schema versions in the file

## 3. Frame packet and renderer API

- [ ] `FramePacket`: draw items, lights, views
- [ ] `Renderer::Render(packet, target)`
- [ ] Port `DemoScene` to produce packets

## 4. World

- [ ] `World` interface: `Apply` / `Update` / `Extract`
- [ ] `EnttWorld` (EnTT contained in `Ethane/World/Entt/`)
- [ ] Engine components: transform, mesh, light, camera; transform propagation
- [ ] Edit world following a brick; `DemoScene` replaced by a brick

## 5. Editor context and panels

- [ ] `EditorContext`: open bricks, worlds, selection, undo
- [ ] Hierarchy panel on brick snapshots
- [ ] Inspector panel driven by the schema
- [ ] Save / open bricks

## 6. Play mode

- [ ] Instantiate a play world from a snapshot; discard on Stop
- [ ] Native script components

## 7. Threads

- [ ] Job system and a parallel system scheduler
- [ ] Render thread with pipelined frame packets
- [ ] Deferred GPU resource deletion (also removes the viewport-resize stall)

## 8. Collaborative editing

- [ ] Server-ordered ops, client rebase, per-user undo

## 9. Own ECS (only if profiling calls for it)

- [ ] A second `World` implementation, validated against `EnttWorld` by comparing frame packets

## Later

- Prefab bricks (reference plus overrides)
- Array fields in the schema
- Picking via an ID buffer, gizmos
- Removing the old `Ethane/Scene` code and the vendored EnTT outside `World/Entt`
