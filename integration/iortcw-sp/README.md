# RTCW single-player integration

This wires the accessibility mode into the single-player half of
**iortcw**, the maintained engine for **Return to Castle Wolfenstein**.
It is the engine for the story campaign built from "Bârlogul lupilor:
Sfârșitul terorii" (Wolf's Lair: The End of Terror): RTCW's single
player has what a story needs and Enemy Territory lacks - enemies that
act on their own, scripted scenes, objectives, saving, and levels that
lead into one another.

Base commit: [`iortcw/iortcw@438e7d4`](https://github.com/iortcw/iortcw/commit/438e7d413b5f7277187c35b032eb0ef9093ae778)
(`base.txt`). Like the other integrations, this is a series of patches,
applied in order, each depending on the ones before it.

The game data - RTCW's `pak0.pk3` and `sp_pak*.pk3` - belongs to id
Software and is never shipped or committed. Players use their own copy.

## Building

```sh
git clone https://github.com/iortcw/iortcw
git -C iortcw checkout 438e7d413b5f7277187c35b032eb0ef9093ae778
for p in integration/iortcw-sp/*.patch; do git -C iortcw apply "$PWD/$p"; done
make -C iortcw/SP BUILD_SERVER=0 BUILD_GAME_QVM=0 \
     FEATURE_ACCESSIBILITY=1 ETA_SOURCE_DIR="$PWD"
```

For Windows, the same variables passed to `iortcw/SP/cross-make-mingw64.sh`.

## 0001: build the accessibility mode into cgame and ui

- `FEATURE_ACCESSIBILITY=1` with `ETA_SOURCE_DIR` compiles the library
  (`src/accessibility/`) and links a copy into the cgame and another into
  the ui, as on ET: Legacy.
- Windows links the speech and sound libraries the library calls (OLE,
  winmm, WinHTTP); the Mac links AppKit, AVFoundation and Foundation.
- The library is native code only, so a build with it makes no QVMs.

## 0002: say the menus

Ported from `integration/noquarter/0002`; RTCW's menus are the same
`ui_shared.c` family, and iortcw already moves through them and changes
settings from the keyboard, so this patch only adds the speech.

- Each control says its label and its setting as focus lands on it: a
  switch on or off, a choice by name, a slider as a percentage, a
  control on the controls page by its keys, in words ("page down",
  "backslash"). Text boxes say their contents and each key typed.
- RTCW writes most labels in capitals and ends many with a colon; they
  are said as sentences, without the colon.
- A menu says its heading as it opens ("Game options menu"), a popup
  reads its message, and the main, game, notebook and briefing menus
  say what they are.
- On the controls page, Enter says the row is listening for a key, and
  the result is read back. The arrow keys no longer get stuck on those
  rows: iortcw took every key on them, so the keyboard could never
  leave one.
- F6 repeats the last thing said and F8 stops speech, offered once if
  the keys are free.

On Linux, which no player runs, the library prints what it would say,
so the game can be checked on a build machine.

## 0003: say the game

Most of it is a new file, `SP/code/cgame/cg_accessibility.c`, built
only with `FEATURE_ACCESSIBILITY`; the rest of the cgame calls into it
from marked hooks.

- **As it happens.** The weapon in hand with what is loaded and spare;
  health at each quarter lost, and what a pickup gave; armor; crouching;
  the compass point faced; what was picked up, by name; a hit, as a sound
  from its side; the level's own messages (center prints); a new
  notebook page; death.
- **Enemies.** A soldier coming into sight is named with where he is
  and how far: "Soldier, ahead right, 13 meters, above". One sighting at
  a time, the rest counted. A soft tick plays from the nearest enemy in
  sight, quicker the closer he is, so his place can be heard; an enemy
  the player has seen dying is said ("Soldier down"). Enemies are the
  characters on the Nazi and monster teams that are alive and in a clear
  line from the eyes.
- **Aim (T, held).** Turns the view onto the nearest enemy in sight and
  keeps it on him, with a tone once it is on target; fire as usual. It
  turns the way the ET mode's turning does: by pressing the engine's own
  `+left`, `+right`, `+lookup` and `+lookdown` under key numbers no
  keyboard has. The compass stays quiet while it turns.
- **Keys.** H health, armor, weapon, ammo and facing; Y every enemy in
  sight, nearest first; O the objective; L the last line of the story;
  F6 and F8 repeat and stop, handing over to the menus when one is open.
  The menus offer them as a second generation of binds, on keys RTCW's
  `default.cfg` leaves free.
- **The story.** A level built for this mode tells its story through two
  cvars its scripts set with the AI script's `cvar` action: `eta_line`,
  a line to say, and `eta_objective`. Each is said when it changes, and a
  story line is not talked over by what the game says on its own until it
  has had time to be heard. A line can name a recorded voice,
  `sound/... | the words`, played instead while `eta_storyVoice` is 1.
- **The briefing screen** before a level is read whole, a sentence-sized
  piece at a time (the speech library takes a few hundred characters a
  line), only the current level's text, and Enter or Space starts the
  level. Its button is drawn only under the mouse, and the engine let no
  key but a click through to it; `cl_keys.c` now lets Enter, Space, F6
  and F8 through as well. That is the one engine change, built only
  with the feature.

The cue sounds are the ones `release/make-cue-sounds.py` makes for the
ET mode, packed into `zzz_accessibility.pk3`.

## 0004: find the way and tell the story

- **The goal.** A level's script marks the next place to go with
  `eta_goal`, "x y z name". A soft tone sounds from there every second
  and a bit, placed by the engine's 3D sound; O says the objective and
  the way: "Go to the generator, ahead right, 34 meters"; arriving is
  said.
- **Walking into things.** Pressing to walk without moving plays a thud
  from the side that is blocked.
- **Names from the level.** `eta_enemyNames`, "1=Sentinel;7=Elite
  sentinel", by RTCW's character numbers, so a story's soldiers are
  called what its book calls them.
- **Whose story.** The story values are read only on the level that set
  them: a level names itself in `eta_storyMap` as it spawns. A script
  cannot set a cvar to nothing, so "none" clears one. The mission screen
  reads `eta_briefing` for a level that has no page of its own there.
- **Room for a sentence.** The game's AI scripts kept an action's
  parameters in 64 characters, a file path's worth; a story line is set
  with the `cvar` action, so with the feature they get the 1024 the
  entity scripts already had (`ai_cast_script.c`). Built only with the
  feature, which the game module now gets too.
- The compass waits behind a story line like everything else the game
  says on its own.

## 0005: pass over an unknown weapon name

The prologue stopped before it began on a tester's Mac with
"BG_IndexForString: unknown token 'TT33'": RealRTCW's player animation
script, in the folder every RTCW install shares, loaded over RTCW's own,
and it names RealRTCW's TT33 pistol. The launchers now keep the game out
of that folder (`fs_homepath`), and this is the second guard, the one
the accessibility mode's own RTCW build has carried since a tester hit
the same error there (its patch 0005): a name in an animation script
that the game does not know is passed over with a warning - a pose
missing for one weapon - instead of ending the game. With it, the weapon
table `ammoTable` is sized to every weapon: code that walks them all
read past its end, harmless on Linux, not on the Mac.

Checked: the first mission started with such a script in the home
folder - "unknown 'TT33' passed over", and the level loads; without the
patch, the same start ends at "unknown token 'TT33'".

## 0006: read the Steam and GOG menus

The Steam and GOG releases' sp_pak4.pk3 (2016) carries menus written for
that release's own engine: `scralign`, how it stretches a piece across a
wide screen, which this UI did not know - an error at each - and a video
mode list of 36, more than `MAX_MULTI_CVARS` (32), so the system menus
failed to load. `scralign` and its word are read and set aside (this
engine scales the whole 640 by 480 menu), and the limit is 64. The
accessibility mode's RTCW build has had this since its patch 0029.

Checked with a Steam copy's files: 328 "unknown menu item keyword"
errors as the first mission starts without it, none with it.

