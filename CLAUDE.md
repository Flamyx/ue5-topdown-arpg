# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project overview

"Aura" is an Unreal Engine 5.8 top-down action RPG built around the Gameplay Ability System (GAS). The C++ module (`Aura`) provides the gameplay framework — attribute sets, gameplay abilities, execution/modifier calculations, widget controllers — while `Content/Blueprints` supplies data assets, Blueprint subclasses, animations, and UI built on top of the C++ base classes. Most gameplay tuning (abilities, damage effects, character class defaults) lives in Blueprint/data assets that derive from the C++ classes in `Source/Aura`, so a change is often incomplete without checking the corresponding Blueprint in the editor.

## Build, run, and iterate

There is no CLI test suite or linter in this repo — it's a game project built through Unreal's toolchain. There are no Automation Spec/unit tests defined in `Source/Aura`.

- **Compile C++ changes**: build the `Aura` target (Development Editor, Win64) via the generated `Aura.sln` in Visual Studio/Rider, or regenerate project files first with the Unreal Editor if `.h`/`.cpp` files were added/removed (right-click `Aura.uproject` → "Generate Visual Studio project files").
- **Run/test in-editor**: open `Aura.uproject` in Unreal Editor 5.8 and Play-In-Editor. There is no headless run path documented here — verifying gameplay changes means opening the editor and testing the relevant map/ability by hand.
- **Hot reload**: for small C++ changes, "Live Coding" (Ctrl+Alt+F11) from the editor is faster than a full recompile+restart.
- When adding new `UCLASS`/`USTRUCT`/`UENUM` types or new source files, project files must be regenerated before the IDE will pick them up.

## Architecture

### GAS layering

`AAuraCharacterBase` (implements `IAbilitySystemInterface` and `ICombatInterface`) is the common base for `AAuraCharacter` (player) and `AAuraEnemy`. Each owns a `UAuraAbilitySystemComponent` (ASC) and `UAuraAttributeSet`. Attribute defaults are applied via three layered `UGameplayEffect` classes set per-character: `DefaultVitalAttributes`, `DefaultPrimaryAttributes`, `DefaultSecondaryAttributes` — primary attributes come from per-class data (see below), vital/secondary are shared, applied in `AAuraCharacterBase::InitializeDefaultAttributes`.

- **`UAuraAttributeSet`** defines Primary (Strength/Intelligence/Resilience/Vigor), Secondary (derived: MaxHealth, MaxMana, Armor, crit stats, regen, resistances), Vital (Health/Mana), and Meta attributes (`IncomingDamage`, `IncomingXP` — write-only pipes, not persisted state). `PostGameplayEffectExecute` intercepts writes to the meta attributes and turns them into `HandleIncomingDamage`/`HandleIncomingXP`, which is where damage is actually subtracted from Health, debuffs/knockback/death are triggered, and floating combat text + XP events are fired. Never expect `IncomingDamage`/`IncomingXP` to hold a resting value — treat them as "apply and consume in the same frame."
- **`FAuraGameplayEffectContext`** (in `AuraAbilityTypes.h`) extends the stock GAS effect context with custom-execution-calculation output: crit/block/debuff flags, debuff damage/duration/frequency, damage type tag, death/knockback impulse vectors. `UAuraAbilitySystemLibrary` exposes Blueprint-callable getters/setters over this context so Blueprint execution calculations and abilities can read/write it without touching C++.
- **`UExecCalc_Damage`** is the single custom execution calculation for all damage: captures resistance/armor/crit attributes from source+target, applies armor mitigation, rolls crit and block, applies per-damage-type resistance, and writes debuff info into the effect context. Damage-type-to-resistance and damage-type-to-debuff mappings are native `FGameplayTag` maps in `FAuraGameplayTags`.
- **`FAuraDamageEffectParams`** (built via `UAuraDamageGameplayAbility::MakeDamageEffectParamsFromClassDefaults`) is the standard payload passed into `UAuraAbilitySystemLibrary::ApplyDamageEffect` to actually spec and apply a damage `UGameplayEffect` between source/target ASCs — this is the path essentially every offensive ability uses instead of applying effects directly.

### Gameplay tags: two sources of truth

Gameplay tags come from two places and both must stay in sync when adding new tags:
- **Native tags** (`Source/Aura/Public/AuraGameplayTags.h` / `.cpp`) — the `FAuraGameplayTags` singleton, registered via `FAuraGameplayTags::InitializeNativeGameplayTags()`, called from `UAuraAssetManager::StartInitialLoading()`. Use these for tags referenced from C++ (attributes, input, abilities, damage types, statuses, cooldowns).
- **Config tags** (`Config/DefaultGameplayTags.ini`) — tags only ever referenced from Blueprint/animation assets (event tags for anim notifies, `GameplayCue.*` tags, UI message tags). Add here when a tag never needs a C++ `FGameplayTag` reference.

### Ability class hierarchy

`UAuraGameplayAbility` (base: input tag, description helpers) → `UAuraDamageGameplayAbility` (adds damage type/magnitude, debuff params, death/knockback impulses, `CauseDamage`/`MakeDamageEffectParamsFromClassDefaults`) → spell-specific subclasses:
- `UAuraProjectileSpell` → `UFireBolt` (multi-projectile spread + homing), `UMeteoriteProjectileSpell`.
- `UAuraDamageGameplayAbility` → `UAuraMeleeAttack`, `UElectrocute` (spawns `AElectrocuteActor` beam, has `InputReleased` override), `UElectrocuteCourse`.
- `UAuraSummonAbility`, `UAuraBuffAbility` for non-damage abilities (minion summons, passive buffs).

Abilities are granted/tracked through `UAuraAbilitySystemComponent`, which also owns the input-tag-to-ability dispatch (`AbilityInputTagPressed/Held/Released`), spell slot equip/upgrade RPCs (`ServerEquipSpell`/`ClientEquipSpell`, `ServerUpgradeSpell`), and attribute point spend (`ServerUpgradeAttribute`). Ability/input/status/type tags for a given ability are looked up via `UAuraAbilitySystemLibrary::FindAbilityTagFromSpec` and friends, backed by the `UAbilityInfo` data asset (`Content` side) that maps ability tags to icons, descriptions, and level requirements for the spell menu UI.

### Data-driven character classes

`UCharacterClassInfo` (a `UDataAsset`) maps `ECharacterClass` (Elementalist/Warrior/Ranger) to per-class primary-attribute GE, startup abilities, behavior tree, and XP reward curve, plus shared vital/secondary GEs and a damage-calculation coefficient curve table used by `UExecCalc_Damage`. `UAuraAbilitySystemLibrary::InitializeEnemyAttributes`/`GiveStartupAbilities` read from this asset — enemies are initialized purely from data, not hardcoded per-enemy-class C++.

### Input

`UAuraInputConfig` (data asset) maps `UInputAction` → `FGameplayTag` pairs. `AAuraPlayerController` resolves Enhanced Input actions to gameplay tags and forwards to the ASC's `AbilityInputTagPressed/Held/Released`. The controller also owns click-to-move (spline-based auto-run) and cursor-trace enemy highlighting (`IEnemyInterface`).

### UI: WidgetController pattern

UI never reads attributes/ASC state directly. `AAuraHUD` owns three `UAuraWidgetController` subclasses — `UOverlayWidgetController` (HUD vitals/XP/level), `UAttributeMenuWidgetController`, `USpellMenuWidgetController` — each constructed lazily from an `FWidgetControllerParams` (PlayerController/PlayerState/ASC/AttributeSet) via `AAuraHUD::Get*WidgetController`, or from Blueprint via `UAuraAbilitySystemLibrary::Get*WidgetController`. Widget controllers bind to ASC/attribute delegates in `BindCallbacksToDependencies` and push initial state in `BroadcastInitialValues`; widgets bind to the controller's delegates, not the ASC.

### Interfaces

- `ICombatInterface` — implemented by all combat-capable actors (level, combat sockets, hit react montage, death/knockback, per-attack montage selection, minion count tracking).
- `IEnemyInterface` — enemy-only concerns (highlight on hover, combat target).
- `IPlayerInterface` — player-only concerns (XP/level/attribute-point queries).

### Networking

Attributes replicate via `OnRep_*` on `UAuraAttributeSet` (standard GAS pattern). Ability equip/upgrade and effect-applied notifications go through explicit `Server`/`Client` RPCs on `UAuraAbilitySystemComponent` and `AAuraPlayerController` (e.g. `ServerEquipSpell`/`ClientEquipSpell`, `ClientEffectApplied`, `ShowDamageNumber`) rather than relying solely on GAS's built-in replication, so multiplayer-facing changes need matching authority checks (`HasAuthority()`) and RPC pairs.

<!-- BEGIN VibeUE (v5.0) — generated by VibeUE.GenerateAgentConfig; re-run to refresh -->
# VibeUE — AI agent guide (Unreal Engine 5.8)

VibeUE **extends Unreal 5.8's native AI toolset system** — its services, tools, and skills register
into the engine's `ToolsetRegistry` and are reachable through the MCP tools you already have.

**ALWAYS use the MCP tools / Python API for Unreal operations — NEVER read `.uasset` files from disk.**

---

## 1. The efficient interaction model (read this first)

There are two ways to act on the editor. Pick the cheap one:

- **`execute_python_code` — your workhorse.** Runs an arbitrary Python script in the editor in **one
  round-trip**. Every VibeUE service is exposed to Python (`unreal.BlueprintService.add_variable(...)`)
  and sits next to the whole native `unreal.*` API in the same script. **Batch aggressively** — do a
  whole multi-step task (create + edit + compile + verify) in a single call, and `print()` only what
  you need back.
- **`call_tool` — one tool per round-trip.** Genuinely needed only for **skills**
  (`AgentSkillToolset`) and for the few Epic tools whose **result the MCP layer must surface for
  you** (image returns like `CaptureViewport`). **Everything else from Epic's engine toolsets is
  also reachable from inside `execute_python_code`** via `unreal.ToolsetRegistry.execute_tool(...)`
  (see §2), so batch engine-toolset calls with your Python instead of spending a round-trip. Don't
  use `call_tool` for work `execute_python_code` can batch.

**Speed + tokens:** **avoid `describe_toolset` as a habit** — it dumps the full JSON schema of every
tool in a toolset (the most token-heavy thing here); reach for a **skill** plus a narrow
`discover_python_class('unreal.BlueprintService', method_filter='variable')` instead.

---

## 2. Tool roster — what's where

**VibeUE MCP tools (call directly):**
- `execute_python_code` — run Python (must start with `import unreal`). The workhorse.
- `discover_python_module` / `discover_python_class` / `discover_python_function` — inspect the API
  (use `unreal` lowercase; narrow with `name_filter` / `method_filter`).
- `list_python_subsystems` — list editor subsystems.
- `deep_research` — web search / page fetch / geocode (see §5).
- `terrain_data` — real-world heightmaps + water splines (see §5).

**VibeUE services (call from Python inside `execute_python_code`):** `unreal.<Name>Service.<method>()`
— Blueprint, BlueprintGraph (via BlueprintService), Material(+Node), Widget, Skeleton, AnimSequence,
AnimMontage, AnimGraph, Landscape(+Material), Foliage, MetaSound, SoundCue, Niagara(+Emitter,
+ScratchPad), StateTree, Input, EnumStruct, UVMapping, RuntimeVirtualTexture, MapBlockout,
GameplayTag, Viewport, Actor, Engine/ProjectSettings, **Performance** (`unreal.PerformanceService.frame_timing()`).
These overlap-trimmed services keep only what the engine lacks — for plain asset/actor/blueprint
basics the engine's own tools may be simpler (below).

**Calling Epic's engine toolsets from Python (`execute_tool`).** Epic's engine toolset *classes*
exist as `unreal.*` (e.g. `unreal.EditorAppToolset`) but their AICallable functions are **not**
exposed as Python methods — `unreal.EditorAppToolset.get_selected_assets()` fails. Invoke them
through the registry instead (same dispatch `call_tool` uses, but in-process and batchable):
```python
import unreal, json
res = unreal.ToolsetRegistry.execute_tool(
    "EditorToolset.EditorAppToolset",   # registered (namespaced) toolset name
    "GetSelectedAssets",                # tool name
    "{}")                               # args as a JSON string
assert res.is_complete and not res.error, res.error
out = json.loads(res.get_value_as_json_string())     # -> {"returnValue": ...}
```
Discover exact names/schemas from Python: `unreal.ToolsetRegistry.get_all_toolset_json_schemas()`
(all of them) or `get_toolset_json_schema("EditorToolset.EditorAppToolset")` (one). Names are
namespaced — `EditorToolset.EditorAppToolset`, `NiagaraToolsets.NiagaraToolset_System`, etc. (a bare
`"EditorAppToolset"` returns "Toolset not found"). Note `execute_tool` returns an **async** result:
the editor tools above complete synchronously (`is_complete=True`), but for a long-running tool check
`is_complete` / bind `on_completed` rather than assuming `value` is ready.

**Use the engine's native tools for these (VibeUE intentionally doesn't duplicate them):**
- **Assets** (find / save / move / delete / duplicate / metadata): native Python
  `unreal.EditorAssetLibrary` / `EditorAssetSubsystem` inside `execute_python_code` (batchable), or
  Epic's `AssetTools` toolset (via `execute_tool` in-Python, or `call_tool`).
- **Screenshots / vision**: Epic's `EditorAppToolset` — `CaptureViewport` (returns a PNG, and can
  overlay a world grid + actor labels), `CaptureEditorImage`, `CaptureAssetImage`. **Use `call_tool`
  for these** so the MCP layer surfaces the image for you to view (`execute_tool` would only hand
  back a base64 string).
- **PIE**: `EditorAppToolset.StartPIE` / `StopPIE` / `IsPIERunning` — batchable via `execute_tool`
  (`"EditorToolset.EditorAppToolset"`), or `call_tool`.
- **Logs**: `LogsToolset.GetLogEntries` — via `execute_tool` or `call_tool` (or read the `.log` file).
- **DataTables / DataAssets / enum-struct basics**: Epic's `DataTableTools` / `DataAssetTools` /
  `ObjectTools` (via `execute_tool` or `call_tool`). (VibeUE keeps only `EnumStructService` for
  create/edit of user enums & structs.)

---

## 3. Skills — native `AgentSkill` (lazy domain knowledge)

VibeUE's ~88 skill packs are registered as Unreal **AgentSkills** and served by the engine's
`AgentSkillToolset`. Skills tell you **what to do and why**; they do **not** replace discovery of exact
signatures.

**Discover + load (both are `call_tool` on `ToolsetRegistry.AgentSkillToolset`):**
```
call_tool(tool_name="ListSkills", toolset_name="ToolsetRegistry.AgentSkillToolset")
  → { "/VibeUE/Python/init_unreal_PY.VibeUE_blueprints": "Create and modify Blueprint assets…", … }

call_tool(tool_name="GetSkills", toolset_name="ToolsetRegistry.AgentSkillToolset",
          arguments={"skillPaths": ["/VibeUE/Python/init_unreal_PY.VibeUE_blueprints"]})
  → full markdown for that pack
```
- `ListSkills` returns **summaries only** (cheap) — call it once per session to see what exists. VibeUE
  packs are `/VibeUE/Python/init_unreal_PY.VibeUE_<name>`; the engine's own skills appear alongside.
- `GetSkills` returns full instructions **lazily** — request only the packs you need.
- **Sub-docs are their own skill entries** (e.g. `…VibeUE_blueprint_graphs__build_graph`,
  `…VibeUE_state_trees__api_reference`) — load them by path the same way; no `skill/section` argument.

**When to load a skill:** the user names a domain ("create a blueprint", "build a state tree"), or you
hit a non-obvious workflow. Then: read the pack → `discover_python_class` the classes it names → write
the Python. Don't reload a pack you already loaded this session.

---

## 4. Python basics

```python
import unreal  # lowercase

# Editor subsystems:
sub = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)

# VibeUE services are static classes, called directly:
info = unreal.BlueprintService.get_blueprint_info("/Game/MyBP")

# Batch a whole task in ONE execute_python_code call, printing evidence as you go:
bp = unreal.BlueprintService.create_blueprint("BP_Enemy", "Actor", "/Game/Blueprints"); print("CREATED:", bp)
unreal.BlueprintService.add_variable(bp, "Health", "float"); print("ADDED: Health")
unreal.BlueprintService.compile_blueprint(bp); print("COMPILED:", bp)
```

---

## 5. When to use `deep_research` and `terrain_data`

**`deep_research`** — when you need information that isn't in the editor:
- `action="search"` / `action="fetch_page"` — research a UE topic, API, or technique before writing code.
- `action="geocode"` / `action="reverse_geocode"` — turn a place name into lat/lng (feeds `terrain_data`).

**`terrain_data`** — when the user wants terrain from a **real-world location**:
- `preview_elevation` → use the suggested `base_level`/`height_scale` → `generate_heightmap`
  (`resolution` MUST match the landscape) → import via `unreal.LandscapeService` → `get_water_features`
  for rivers/lakes.

**The real-world-terrain chain:** `deep_research(geocode "Mount Fuji")` → `terrain_data(generate_heightmap, lng/lat)`
→ `LandscapeService` import → `terrain_data(get_water_features)` → landscape splines. Load the
`terrain-data` and `landscape` skills for the resolution formulas and water workflow.

---

## 6. See what you built (screenshots)

After any **visible** change, capture and actually look before claiming success:
```
call_tool(tool_name="CaptureViewport", toolset_name="EditorToolset.EditorAppToolset")
```
It returns a PNG (base64) and can overlay a world-space grid + actor labels for spatial awareness. For
a running game, `StartPIE` first. **Open/read the image, judge it against the request, fix, re-capture.**

---

## 7. Diagnose performance

`PerformanceService` is VibeUE's net-new capability (the engine has no perf tooling). **STEP 0 is
always CPU-bound vs GPU-bound** — optimising the GPU does nothing on a CPU-bound frame:
```python
import unreal, json
print(unreal.PerformanceService.frame_timing())            # game/render/gpu ms + bound verdict — RUN FIRST
unreal.PerformanceService.start_trace("cap", "")           # Unreal Insights trace
# … reproduce the workload (ideally under PIE / standalone) …
unreal.PerformanceService.stop_trace()
print(unreal.PerformanceService.analyse("both", ""))       # frame stats + worst frames + log hitches
```
Load the `profiling` and `frame-rate` skills for the full drill-down.

---

## 8. Build & launch

When asked to rebuild / relaunch / test, use the project script — not manual `Build.bat`/editor commands:
- `./Plugins/VibeUE/BuildAndLaunchGame.ps1` (stops the editor, builds, relaunches).
- `-StrictRebuild` for a full plugin recompile under warnings-as-errors; `-Clean` to wipe artifacts;
  `-SkipBuild` to relaunch only.

---

## 9. Critical rules (evergreen)

- **Log every change for rollback.** Python has no auto-rollback — `print("CREATED:/ADDED:/MODIFIED:/DELETED:", path)`
  after each op so a mid-script failure can be undone.
- **Idempotent: check before create.** Use the service `*_exists()` (or `unreal.EditorAssetLibrary.does_asset_exist`)
  before creating, to avoid duplicates.
- **Compile after structure changes.** `unreal.BlueprintService.compile_blueprint(path)` after adding
  variables/functions/components.
- **Verify success with evidence.** For Blueprint/Widget/Material/AnimGraph/StateTree edits, a successful
  tool call isn't proof — re-read the asset (`get_nodes_in_graph`, `get_connections`, compile result)
  and report brief evidence.
- **Non-destructive.** Never remove-and-recreate to change a value, clear data to make a write succeed,
  or replace a whole object to change one field. Discover the supported setter; if none exists, report
  the gap. (StateTree reparenting: `move_state`, never remove+add.)
- **Loop prevention.** Track *outcomes*. Never repeat the same call with the same args >2× when output
  is unchanged; after 2 failed attempts at a goal, stop and report — don't try a 3rd variation.
- **Never** use modal dialogs, `input()`, blocking ops, long `time.sleep()`, or infinite loops.
- **Full asset paths** (`/Game/Blueprints/BP_Name`). **Colors are 0.0–1.0** (`{"R":1.0,"G":0.5,"B":0.0,"A":1.0}`).
- **`unreal.EditorLevelLibrary` is deprecated** — use `EditorActorSubsystem` (`get_all_level_actors()`
  + `isinstance` filtering; `get_all_level_actors_of_class` does not exist).

---

## 10. Communication & working style

- **Be concise** — this is an IDE tool. Before each tool call, one sentence on what/why; after, 1–2 on
  the result. Execute multi-step tasks straight through — don't pause for "continue".
- **Discover before you call.** Method signatures come from `discover_python_class`, not memory or skill
  prose. Skills say *which* class and *why*; discovery gives the exact call shape.
- **Commit at milestones** if the project is a git repo, so a bad experiment reverts cleanly.
- **Living gotchas:** when you solve a real problem, append a one-line gotcha+fix to this file so the
  next session doesn't relearn it.
<!-- END VibeUE -->

## Living gotchas (append one-liners here)

- Epic MCP server (`:8000/mcp`) auto-start: `bAutoStartServer=True` under `[/Script/ModelContextProtocolEngine.ModelContextProtocolSettings]` in `Saved/Config/WindowsEditor/EditorPerProjectUserSettings.ini` (set 2026-07-13); launch flag `-ModelContextProtocolStartServer` also works.
- `BuildAndLaunchGame.ps1` does not exist in this project — build with `& "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" AuraEditor Win64 Development -Project=<uproject> -WaitMutex` (stop the editor first).
- `build_graph` node type `spawner_key` returns None — create K2Node_LatentAbilityCall nodes (ability tasks) via `create_node_by_key` first, then wire their GUIDs in `build_graph`; result struct field is `success`, and compile via `BlueprintEditorLibrary.compile_blueprint` (BlueprintService has none).
- FGameplayTag BP properties CAN be set from Python via `unreal.BlueprintService.set_property(bp_path, "DamageType", "Damage.Fire")` even though `unreal.GameplayTag` itself is read-only.
- NEVER `CompileLiveCoding` after adding a new UCLASS/source file — the patch links fine, then the editor fatal-crashes (EXCEPTION_ACCESS_VIOLATION in `Z_Construct_UPackage__Script_Aura` during /Script/Aura reload; confirmed 2026-07-14). New reflected types = stop editor + full Build.bat. If the editor crashed, kill `CrashReportClientEditor.exe` before rebuilding — it holds UnrealEditor-Aura.dll and the link fails with LNK1104.
- `execute_tool` results are DOUBLE-encoded JSON — `json.loads(r.get_value_as_json_string())` yields a *str* for most toolsets; parse twice or `res["returnValue"]` raises `TypeError: string indices must be integers`.
- Screenshots: `SlateInspector.Screenshot` targets any window by ref (`Snapshot` with `ref:""` lists them: `w1` = main editor) at full 2560x1392 — always better than `CaptureEditorImage`, which grabs whatever floating window is topmost at only 1280x696. `CaptureViewport` renders the **editor** world from a supplied transform (never the PIE world, so no player pawn/HUD even during PIE) and requires BOTH `captureTransform` and `annotations` explicitly — no defaults; disable annotations with zeros + `classFilter: None`.
- Decode capture base64 to disk inside `execute_python_code`; a 3 MB PNG through `call_tool` is pure token cost. Use `call_tool` only when the image must be *seen* in-conversation.
- Level shots need `ViewportService.set_game_view(True)` (otherwise BlockingVolume/NavMesh wireframes dominate and everything reads flat blue — `get_view_mode()` still says "lit", it's show-flags) plus `set_exposure(True, -2.0)`, or the Dungeon renders near-black.
- `OpenEditorForAsset` on the ALREADY-LOADED World can raise a modal dialog titled *Message* that hangs the game thread: MCP goes silent, every call times out at 300 s, and the process still reports Responding=True. Only a human can dismiss it — don't reopen the current level just to focus its tab. `AssetEditorSubsystem` has no `close_all_asset_editors` in 5.8, and `close_all_editors_for_asset` returns 0 without closing floating asset-editor windows.
- **`execute_python_code` globals PERSIST between calls, and a leftover UWorld/Actor ref fatal-crashes the editor** on the next `new_level`/`load_level` ("World Memory Leaks: 1 leaks objects and packages", EditorServer.cpp:1951; confirmed 2026-07-28). Before any level switch: delete non-builtin globals (or at least the world var) and `gc.collect()`. Never leave `w = ...get_editor_world()` sitting in globals.
- **`unreal.Rotator(...)` positional args are `(roll, pitch, yaw)`, NOT (pitch, yaw, roll).** `Rotator(-40, 0, 118)` on a DirectionalLight sets roll=-40/pitch=0 → sun exactly on the horizon and the whole scene renders sky-lit and near-black. Use keywords or `Rotator(0.0, pitch, yaw)`.
- Actors have no `set_mobility` — use `actor.root_component.set_mobility(unreal.ComponentMobility.MOVABLE)`. `set_level_viewport_camera_info` requires `viewport_config_key` positionally (pass `""`).
- 5.8 property-name traps: SkyAtmosphere `aerial_pespective_view_distance_scale` (engine typo, no 'r'); fog `enable_volumetric_fog` + `directional_inscattering_luminance` (not `volumetric_fog` / `..._color`); SkyLight brightness is `intensity` (no `intensity_value`). `LandscapeLayerInfoObject.layer_name` is read-only — duplicate the LayerInfo asset instead of renaming.
- `CaptureViewport`'s JSON is `{"image":{"mimeType","data"}}` — read `rv["image"]["data"]`, not a top-level string key. Always decode to disk inside `execute_python_code`; routing a 2-4 MB PNG through `call_tool` blows the token budget in one shot.
- **Authoring terrain from Python is fast and is the right tool:** `LandscapeService.set_height_in_region` / `set_weights_in_region` in ~128-row chunks rewrite a full 1017² landscape (1.03 M verts) in ~3 s each. There is NO numpy in the editor interpreter — precompute value-noise fields at half resolution and bilinearly sample them per vertex to stay fast. `get_height_at_location` is ~0.1 ms, so ground-snapping a few thousand placed actors in one script is fine.
- **PCG graphs ARE authorable from Python** (no VibeUE `PCGService` needed): `graph.add_node_of_type(unreal.PCG<X>Settings)` → returns `(node, settings)`; `graph.add_edge(from, "OutPin", to, "InPin")`; `node.settings_interface` reads settings back (there is no `settings` property); `graph.remove_nodes()` takes NO arguments — loop `remove_node(n)`. Three traps that each cost a silent no-op: (1) the **output node's input pin is labelled `"Out"`, not `"In"`** — `add_edge(sms,"Out",OUT,"In")` fails silently and the whole graph produces zero points; (2) feeding the component Input pin into Surface Sampler's `Surface` does NOT deliver landscape data — add an explicit `PCGGetLandscapeSettings` node → `Surface`, and leave component Input → `Bounding Shape` (set `PCGComponent.input_type = PCGComponentInput.ACTOR`); (3) **generation is async and ticks the game thread** — `pc.generate(True)` then checking instance counts in the SAME `execute_python_code` call always reports 0. End the script and check in the next call.
- PCG scatter idioms that worked: `NormalToDensity` (normal 0,0,1) turns slope into density, then two `DensityFilter`s split flat vs steep; `Distance` (Source=flat, Target=steep, `set_density=True`, `maximum_distance`) + a `DensityFilter` gives a clean "near the cliff edge" mask with no spline needed. Mesh list: `spawner_settings.get_editor_property("mesh_selector_parameters").set_editor_property("mesh_entries", [PCGMeshSelectorWeightedEntry...])` (`mesh_selector_instance` is the deprecated alias). Verify by counting `InstancedStaticMeshComponent.get_instance_count()` on the volume actor.
- `PCGVolume` spawned from Python has brush half-extent **100**, and `get_actor_bounds` right after spawn reports the already-scaled value — compute `scale = desired_half_extent / 100`, don't derive it from that first reading.
- Landscape paint layers must exist before a layered material shows anything: `add_layer(landscape, <LayerInfo asset path>)` for each layer, then bulk `set_weights_in_region`. An unpainted landscape renders flat/untextured no matter how good the material is.
- **PCG meshes tilt on slopes because `PCGTransformPointsSettings.absolute_rotation` defaults to False** — Surface Sampler points inherit the landscape normal as their rotation, and a non-absolute rotation range is *composed* onto it. For upright buildings set `absolute_rotation=True` with `rotation_min=Rotator(0,0,0)` / `rotation_max=Rotator(0,0,360)` (yaw only). Leave it False for rocks/foliage that *should* hug the slope.
- **`PCGAttributePropertyInputSelector.import_text` needs the full wrapper**: `import_text("PCGBegin($Position.Z)PCGEnd")`. A bare `"$Position.Z"` returns True but silently leaves the selector on `@Last`, so the filter reads the wrong attribute. `set_point_property()` alone also doesn't stick. Always read back `export_text()` to verify.
- **PCG `Distance` clamps out-of-range points to density 1.0**, so a downstream DensityFilter with `upper_bound = 1.0` passes *everything* and the filter looks like a no-op. Keep the upper bound below 1.0 (e.g. 0.95).
- `Distance` measures in **3D**. On tall terrain a radius big enough to reach a canyon rim also reaches far across the flat plateau. Trick: insert a TransformPoints that lifts the *reference* points to mid-height (here floor +10750) so vertical error is symmetric and the test behaves horizontally.
- **Footprint flatness test = `PointNeighborhood` (SET_AVERAGE_DENSITY) straight after `NormalToDensity`**, then a tight DensityFilter (0.975..1.0). Per-point normals only describe one vertex; averaging slope over a radius rejects cliff-edge sites. The sampler must be dense enough that the search radius actually contains neighbours — at 0.0022 pts/m² (~21 m spacing) a 2200 uu radius often catches zero and the test silently passes everything; 0.0055 pts/m² + 3200 uu radius (~19 neighbours) works.
- `PCGGraph` has **no `get_nodes()`** — use `get_editor_property("nodes")`; `remove_edge(nodeA,"OutPin",nodeB,"InPin")` mirrors `add_edge`. `PCGComponent.cleanup()` requires a positional `remove_components` arg — `generate(True)` alone is enough to force a rebuild. PointFilter's output pins are **`InsideFilter` / `OutsideFilter`**, not "Out".
- Viewport exposure for the DesertCanyonCity level: **EV100 = 0.0**. `-2.0` blows the desert to flat white (and near-identical PNGs are the tell), `+9.0` renders black.
- **NEVER `CompileLiveCoding` while PIE is running** — the patch load fatal-crashes the editor with "Trying to modify UObject map (FindOrAdd) that is currently being iterated" (UObjectHash.cpp:650; confirmed 2026-09-06). `StopPIE` and confirm `IsPIERunning` is false in a *previous* call, then compile. Live Coding also intermittently returns `CompileNotStarted`/"Live coding failed" with no detail — that usually means a real compile error (the errors go to the Live Coding console window, NOT the editor log); a plain retry succeeds when it was a transient collision.
- `ASC->AddGameplayCue(Tag)` **silently discards any FGameplayCueParameters you built** — that overload takes an FGameplayEffectContextHandle and rebuilds params from an empty context (`AbilitySystemComponent.cpp:1543`), so the cue sees null TargetAttachComponent/SourceObject and zero Location. Use `AddGameplayCue(Tag, CueParams)`. Symptom: "SpawnNiagaraEmitterAttached: NULL AttachComponent" then "Accessed None trying to read property <FX var>" from the cue's WhileActive.
- Blueprint graphs are readable even though `VibeUE.BlueprintService.list_graphs` returns `[]` here: use Epic's `editor_toolset.toolsets.blueprint.BlueprintTools` — `list_graphs {"blueprint":{"refPath":"/Game/X.X"}}` then `read_graph_dsl {"graph":{"refPath":"/Game/X.X:WhileActive"}}` returns a compact S-expression of the whole graph (~1 KB, far cheaper than `get_nodes_in_graph`).
- **`EditorActorSubsystem.get_all_level_actors()` returns `[]` while PIE is running** — a scan over it silently finds nothing, which reads as "nothing there". During PIE use `unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()` + `unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor)` (also catches runtime-spawned components like the player's). Component bounds are NOT a Python attribute: use `unreal.SystemLibrary.get_component_bounds(comp)` → `(origin, extent, radius)`. The custom cursor channel is exposed as `unreal.CollisionChannel.ECC_TARGET` (named, not `ECC_GAME_TRACE_CHANNEL2`). Never wrap such scans in `except: pass` — it hid both mistakes.
- **`ECC_Target` (cursor channel) defaults to Block**, so every collision-enabled component eats cursor clicks unless told otherwise. Confirmed 2026-09-13: BP_AuraCharacter's SpringArm-attached `FadeCollisionBox` (thin slab along the camera ray) and `CharacterMesh0` both blocked it — causing motion-warp pivots toward the camera when the cursor was on the screen-vertical line through Aura, and Electrocute self-targeting. Set `Target`=Ignore on the BP (not in `AAuraCharacterBase`, which would also make enemies unclickable). `a.MotionWarping.Debug 1` logs the warp target's `Dist2D`/`Z`; a `Z` far above character height means the cursor hit something in the air.
- **A single `FGameplayTag` read from Python always prints as `<Struct 'GameplayTag' ... {}>` — even when it holds a tag.** It does NOT mean the tag is empty. Read it with `tag.export_text()` → `(TagName="Damage.Fire")`. (Tag *containers* do print their contents.) Confirmed 2026-09-13 after repeatedly misreporting GA_Meteorite / GA_ElectrocuteCourse `DamageType` as unset.
- No `python` on PATH in the Bash tool on this machine (the WindowsApps stubs only) — use `C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/ThirdParty/Python3/Win64/python.exe` for host-side JSON when driving the MCP HTTP bridge.

## Field notes: UE 5.8 / VibeUE MCP (session-tested 2026-07-13)

### What worked
- `execute_python_code` is the workhorse and is cheap: CDO reads/writes, asset saves, graph edits all ran in **15-350 ms**; a full-graph dump or asset-registry scan ~2 s (but the *first* registry scan right after editor boot can queue for minutes behind startup asset scanning - wait for the editor to settle).
- `unreal.BlueprintService.set_property(bp_path, "DamageType", "Damage.Fire")` sets **FGameplayTag** properties from plain strings; verify by re-reading the CDO (`get_default_object(cls).get_editor_property("damage_type")`). Donor-copying a tag struct from another CDO also works.
- Graph editing pipeline that works end-to-end: `discover_nodes` (get spawner keys) -> `create_node_by_key(bp, graph, "SPAWN K2Node_LatentAbilityCall|Wait Input Release", x, y)` for **ability-task/latent nodes** -> `build_graph` for plain nodes (`function_call`, `variable_set`) passing explicit `unreal.GraphNodeDesc`/`GraphConnectionDesc` structs, referencing the latent nodes' GUIDs in connections -> `disconnect_pin`/`connect_nodes` for entry rewiring -> `set_node_position` -> `add_comment_around_nodes(bp, graph, text, node_ids)` -> `BlueprintEditorLibrary.compile_blueprint(load_asset(path))` + `EditorAssetLibrary.save_asset(path)`.
- Epic toolsets via `call_tool`: `LiveCodingToolset.LiveCodingToolset / CompileLiveCoding` (body-only C++ changes, ~1-2 s, returns full compile output) and `ToolsetRegistry.AgentSkillToolset / ListSkills+GetSkills`.
- Raw MCP-over-HTTP fallback when the harness drops the MCP tools mid-session: init handshake captures `Mcp-Session-Id` header; `tools/call` responses come back as **SSE** (`text/event-stream`) - use `curl.exe` and parse `data:` lines. PS 5.1 `Invoke-WebRequest` buffers SSE forever and hangs.

### What did NOT work
- `build_graph` with node type `spawner_key` -> whole call returns `None`, nothing created. Latent nodes must go through `create_node_by_key`.
- `BlueprintService.compile_blueprint` / `save` do not exist; `BuildGraphResult.b_success` doesn't either (field is `success`).
- `unreal.GameplayTag` cannot be constructed/mutated in Python (`tag_name` read-only, no `request_gameplay_tag`); `unreal.AbilitySystemBlueprintLibrary` not exposed.
- `LevelEditorPlaySettings.PlayNetMode` unsettable from Python and RELOADCFG won't reload it - networked PIE must be set by hand in the Play dropdown.
- Dicts passed where `Array[GraphNodeDesc]` is expected fail silently (None return) - build the structs explicitly.

### Token weight of operations (approximate, plan around these)
- `describe_toolset`: **~130 KB** - never use; `discover_python_class` with `method_filter` is 2-4 KB for the same answer.
- `ListSkills`: ~34 KB (once per session max). `GetSkills` for one pack: ~35 KB - load only when actually doing that domain.
- Raw `get_nodes_in_graph` + `get_connections` on a ~50-node graph: **~65 KB / 37K tokens** - never print raw; summarize in Python (id[:8] + title + from->to lines) for ~5 KB.
- Viewport/asset image captures: **1+ MB base64** - only via harness `call_tool` (which renders the image); useless through a text bridge.
- Typical well-scoped `execute_python_code` result: < 2 KB. Print only what you need; batch a whole task per call.
