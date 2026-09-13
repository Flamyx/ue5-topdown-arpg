# Capturing Aura from MCP — what the three capture tools actually do

Companion to `aura-ue58-mcp-guide.html` (setup) and the MCP tool map. Each capture described
below was produced from Claude Code against the running 5.8 editor on 2026-07-21, using the
calls reproduced here. Nothing was taken by hand. The PNGs themselves are not versioned
(`Docs/Images/` is gitignored); the descriptions record what each one showed.

## Which capture tool to reach for

There are three, and they are not interchangeable. The differences cost real round-trips to
discover, so they're recorded here.

| Tool | What it captures | Resolution seen here | Use it for |
|---|---|---|---|
| `EditorAppToolset.CaptureViewport` | The **3D world**, rendered from a camera transform you supply. Ignores window layout entirely. | 2117×1222 | Level, gameplay, anything spatial |
| `EditorAppToolset.CaptureEditorImage` | The **topmost editor window**, as-is. No targeting. | 1280×696 | Quick "what's on screen" |
| `SlateInspectorToolset.Screenshot` | **Any named window**, by Slate ref, regardless of z-order. | 2560×1392 | Asset editors — best quality, and the only one that can target |

The practical rule: **use `SlateInspector.Screenshot` for editor UI, `CaptureViewport` for the
world, and `CaptureEditorImage` almost never** — it grabs whatever floating window happens to be
in front, which on this project meant repeatedly capturing a widget designer instead of the level.

Get the window refs first:

```python
r = unreal.ToolsetRegistry.execute_tool(
    "SlateInspectorToolset.SlateInspectorToolset", "Snapshot",
    json.dumps({"ref": "", "maxDepth": 1, "bIncludeSourceLocations": False}))
# -> window "Aura - Unreal Editor" [ref=w1]
#    window "WBP_AttributeMenu" [focused] [ref=w9]
```

---

## 1. The dungeon encounter

The two `BP_GoblinSpear2` enemies placed in `/Game/Maps/Dungeon`, shot from roughly the player's
viewing angle — the camera sits just behind `PlayerStart` at `(-2043, -1050, 92)`.

Getting this readable took two fixes that are easy to miss:

- **Game View off** meant the first captures were dominated by `BlockingVolume` and
  `NavMeshBoundsVolume` wireframes, with everything tinted flat blue. `get_view_mode()` reported
  `"lit"` the whole time — the culprit is show-flags, not the view mode.
- **Auto-exposure** rendered the dungeon almost black. Fixed exposure at **EV100 = −2** matches
  what the level actually looks like in play.

```python
unreal.ViewportService.set_game_view(True)
unreal.ViewportService.set_exposure(True, -2.0)     # EV100; lower = brighter
unreal.ViewportService.set_fov(75.0)

unreal.ToolsetRegistry.execute_tool(
    "EditorToolset.EditorAppToolset", "CaptureViewport", json.dumps({
        "captureTransform": {"location": {"x": -2135, "y": -1135, "z": 1009},
                             "rotation": {"pitch": -50, "yaw": 45, "roll": 0},
                             "scale": {"x": 1, "y": 1, "z": 1}},
        "annotations": NO_ANNOTATIONS}))
```

## 2. The annotation overlay

The same frame with `CaptureViewport`'s annotation overlay enabled. This is the capability with no
equivalent anywhere else in the toolchain: a world-space ground grid labelled in metres, plus actor
labels carrying live world positions — `BP_GOBLINSPEAR2 @(-17,-7,1)`, `BP_FADEACTOR_STAIRS @(-13,-8,0)`,
`BLOCKINGVOLUME4 @(-18,0,3)` — and the X/Y axis indicators.

It exists so an agent can reason spatially about a level ("place the summon circle two metres left
of the near goblin") without guessing at coordinates. It is also the fastest way to eyeball whether
placed actors sit where the data says they do.

```python
annotations = {"gridSpacing": 250.0, "gridExtent": 3000.0, "gridHeight": 90.0,
               "maxLabelDistance": 4000.0, "classFilter": None, "maxLabels": 12}
```

`classFilter` accepts an actor class, so `AuraEnemy` alone can be labelled when a full scene would
be unreadable.

## 3. GA_Meteorite — graph and GAS parameters

`GA_Meteorite`'s EventGraph with the Details panel open, which is the more useful half: the whole
`UAuraDamageGameplayAbility` parameter block is visible in one frame — Damage Type, Debuff Chance /
Damage / Frequency / Duration, Death Impulse, Knockback Impulse, Max Charge Time, and the ability's
tag block.

That panel is the fastest visual check that a Blueprint's damage type matches its VFX and its
`GE_Cooldown*` effect — the class of mismatch that had `GA_ElectrocuteCourse` dealing the wrong
damage type until it was corrected to `Damage.Lightning`.

This one was taken with `CaptureEditorImage` and shows its weakness: 1280×696, and the graph is
framed wherever the editor last left it. `SlateInspector.Screenshot` on `w1` would give the same
content at 2560×1392.

## 4. NS_ElectricBeam

The Electrocute beam system, captured at full resolution by targeting the main window directly. The
emitter stack is legible, which is the point: **BlueElectricBeam** and **BlackElectricBeam** enabled,
**WhiteElectricBeam** and **Sparks001** disabled, and every module row — Beam Emitter Setup,
Spawn Beam, Curl Noise Force, Ribbon Renderer — readable.

Note the `Deprecated Original Initialize Particle` rows in the enabled emitters. That is the residue
of the 5.8 module-upgrade prompt that once recreated `Initialize Particle` with fresh defaults and
collapsed the beam. Keep declining those prompts on this system.

```python
unreal.ToolsetRegistry.execute_tool(
    "SlateInspectorToolset.SlateInspectorToolset", "Screenshot", json.dumps({"ref": "w1"}))
```

## 5. Pickups and enemies under PIE

Captured while a PIE session was live (`BP_AuraCharacter_C_0` had spawned at `(-2043, -1050, 90)` in
`/Game/Maps/UEDPIE_0_Dungeon`), showing both `AuraEffectActor` pickups on their pedestals — the
health crystal glowing warm, the mana crystal violet — with the goblin pair beyond them.

**Caveat worth recording:** `CaptureViewport` renders the *editor* world from the transform you give
it, not the PIE world, so the player pawn and the HUD do not appear even with PIE running. For a
true HUD shot you need the level viewport foregrounded in the main window and
`SlateInspector.Screenshot` on `w1`.

## 6. WBP_AttributeMenu

The attribute menu's widget tree, targeted by ref while it sat behind other windows. The hierarchy
is the documentation: `Overlay_Root → SizeBox_Root → Overlay_Box → WrapBox`, then
`AttributePointsRow` followed by `Row_Strength`, `Row_Intelligence`, `Row_Resilience`, `Row_Vigor` —
one row per primary attribute, parented to `Aura User Widget`.

Adding a primary attribute means adding a row here as well as in C++; this shot is the checklist.

```python
unreal.ToolsetRegistry.execute_tool(
    "SlateInspectorToolset.SlateInspectorToolset", "Screenshot", json.dumps({"ref": "w9"}))
```

---

## Gotchas paid for while producing these

- **`CaptureViewport` has no optional parameters.** Both `captureTransform` and `annotations` are
  rejected if omitted, despite reading as optional. To disable annotations pass zeros with a null
  `classFilter`, not an empty object.
- **Tool results are double-encoded JSON.** `json.loads(r.get_value_as_json_string())` returns a
  *string* for most toolsets; parse twice, or `res["returnValue"]` raises
  `TypeError: string indices must be integers`.
- **Base64 never needs to reach the agent.** Decode and write to disk inside `execute_python_code`
  — a 3 MB viewport PNG through `call_tool` is pure token cost. Reserve `call_tool` for when the
  image genuinely has to be *looked at* in-conversation.
- **`AssetEditorSubsystem` has no `close_all_asset_editors`** in 5.8 Python, and
  `close_all_editors_for_asset` returned 0 without closing a floating asset-editor window.
- **Opening a `World` asset that is already loaded can raise a modal dialog** and hang the game
  thread. MCP then goes silent and every call times out at 300 s, while the process still reports
  as responding. If that happens, look at the editor for a small dialog titled *Message* and dismiss
  it by hand — no MCP call can clear it. Prefer not to reopen the current level just to focus its tab.
