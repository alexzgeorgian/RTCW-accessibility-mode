# Return to Castle Wolfenstein integration

This builds the accessibility mode into **iortcw**, the open-source
engine of Return to Castle Wolfenstein, so the game's single-player
campaign can be played with a screen reader. Only the campaign (`SP/`)
is changed; iortcw's multiplayer (`MP/`) is left as it is.

Base commit: [`iortcw/iortcw@438e7d4`](https://github.com/iortcw/iortcw/commit/438e7d413b5f7277187c35b032eb0ef9093ae778)
("All: Fix compilation on FreeBSD", May 2024, the newest commit).

Like the other integrations, this is a series of patches rather than a
fork. Apply them in order; each depends on the ones before it.

## Licence

iortcw is under the GNU General Public License, version 3, with id
Software's additional terms (`SP/COPYING.txt`). These patches, and the
engine built from them, are under the same licence. The game's own data
- its pk3 files - is not, and is never in a download: the player brings
their own.

## Why this is its own integration

RTCW is the game Enemy Territory grew from, and its menus are the same
`ui_shared.c` ancestor No Quarter's are, so the menu speech follows
`integration/noquarter/` closely. The campaign is different from either:
there are no teams or classes, the player is alone against the AI, and
the mission comes as a briefing, objectives in a notebook, and
cutscenes. And since the campaign runs its game module in the player's
own game, and iortcw is the engine, the download is the whole game
engine with the mode built in, not a mod for another engine.

The accessibility library (`src/accessibility/`) is used unchanged, and
gained two things for this: `ETA_Speech_SayLong`, which says a page too
long for one line as a run of sentences, and `ETA_SPEECH_LOG`, which
copies every line spoken to a file, for testing a build where there is
no one to listen.

## Building

```sh
release/build-iortcw.sh --out out --version rtcw-test-1 [--mac-app iowolfsp.app.tar.gz]
```

It clones iortcw at the commit in `base.txt`, checks it, applies these
patches and cross-compiles for 64-bit Windows with mingw-w64. On a Mac,
`release/build-iortcw-mac.sh` builds the same for Apple Silicon and Intel
and joins them into one `iowolfsp.app` with iortcw's own
`make-macosx-app.sh`; given that, the first script packs a Mac download
too. `.github/workflows/rtcw-test.yml` runs both on every change and
publishes the result as a prerelease, `rtcw-test-N`.

By hand, for Linux:

```sh
git clone https://github.com/iortcw/iortcw
git -C iortcw checkout 438e7d413b5f7277187c35b032eb0ef9093ae778
for p in integration/iortcw/*.patch; do git -C iortcw apply "$PWD/$p"; done
make -C iortcw/SP FEATURE_ACCESSIBILITY=1 ETA_SOURCE_DIR="$PWD"
```

## 0001: build the accessibility mode into cgame and ui

- `SP/Makefile`: `FEATURE_ACCESSIBILITY=1` and `ETA_SOURCE_DIR`, the
  checkout of this repository. When on, the library's files are built
  once, with the game modules' flags, and linked into `cgame` and `ui`,
  each its own copy; on the Mac with the Objective-C speech and its
  frameworks, on Windows with the COM and WinHTTP libraries.
  `ETA_RELEASE_VERSION` names the build.

## 0002: say the menus

In `SP/code/ui/`, the No Quarter menu speech, ported:

- Each control says its name and its setting as the keyboard lands on
  it, and a change of setting says the new value. iortcw already lets
  the keyboard change yes-no and choice settings; a slider's arrow keys
  now move in round steps.
- Text boxes say their contents, each character typed or deleted, and
  the characters the cursor passes; Enter starting and ending the
  editing says so. The save game's name box included.
- The saved games list reads each save: its name, the mission and part
  it was saved in, and when.
- Each menu says what it is as it opens ("Main menu", "New game. Choose
  a skill level", "Game paused. In-game menu"), and a popup is read
  whole: the quit question, the error message.
- Pages of reading - the notebook, the clipboard, the books, the
  credits - are read as they open. F7 reads whatever screen is on top.
  The notebook's page corners, drawn as pictures with no words, are
  "Previous page" and "Next page", and a page turned is read
  (`Script_NotebookShowpage`).
- **The briefing.** Before each mission the game shows its objectives
  and orders and waits for a click on an arrow that only appears under
  the mouse pointer, and the engine (`client/cl_keys.c`) ate every key
  but the mouse button there. Now the page is read as it opens - the
  map's mission and part, the objectives, the orders - and Enter starts
  the mission. Before a cutscene map, the page says it is a cutscene and
  how to skip it.
- F6 repeats, F7 reads the screen, F8 stops: the menus take every key
  and run no binds, so the ui looks up the key's own bind and answers
  the mod's commands itself. The keys are offered once, on keys the
  stock config leaves free (`eta_bindsInstalled`).
- The console opening and closing before a game.

## 0003: say the game

A new file, `SP/code/cgame/cg_accessibility.c`, built only with
`FEATURE_ACCESSIBILITY`; the game's own files call into it from marked
hooks (`CG_Init`, `CG_Shutdown`, `CG_DrawActiveFrame`,
`CG_ConsoleCommand`, `CG_CenterPrint`, `CG_ItemPickup`).

- **F1** (`accessibilityStatus`): health, armor, the weapon in hand and
  its ammunition as the HUD counts it, keys and items carried, and how
  many objectives are done.
- **F2** (`accessibilityObjectives`): each objective, with whether it is
  done. The words are the mission's own: the briefing page's objectives
  file, found in `ui/briefing.menu` beside the map's name, one objective
  to a paragraph, matched to the `g_objective1` to `8` cvars the game
  sets as each is met. F7 in a mission says the same.
- **As it happens**: the weapon changed to and its ammunition; health
  falling past 75, 50 and 25, and dying; what was picked up and how
  much, in the names `text/pickupnames.txt` gives; what the crosshair
  is on when it is something to use (the server's cursor hint: door,
  locked door, ladder, button, alarm, the way out and whether the
  objectives allow it yet); each objective done, by name; a new
  notebook page; what the game prints mid-screen; a cutscene starting
  and ending; the console opening and closing.
- F6 and F8 work in a mission, and hand over to the menus when one is
  open, as in the other modes.

Getting around came in 0010.

## 0004: build on today's Mac

The zlib iortcw bundles, and the copy inside its freetype, take
`TARGET_OS_MAC` to mean classic Mac OS: they define `fdopen` away and
leave out the `Byte` type. Today's macOS SDK defines `TARGET_OS_MAC` on
every Mac, so the build stopped with errors. Only `MACOS`, the classic
Mac's own define, is tested now (`zlib-1.2.11/zutil.h`,
`freetype-2.9/src/gzip/zutil.h` and `ftzconf.h`).

## 0005: keep the campaign going past an unknown weapon name

- `game/bg_animation.c`: a weapon an animation script names that the
  weapon list does not hold is passed over with a warning in the console,
  where it stopped the game with "BG_IndexForString: unknown token". On
  the tester's Mac, choosing a skill level stopped the campaign at once,
  at line 10 of the player's own script (his pistols), with the same files
  Linux and a Mac test of the lookup read without complaint.
- `game/bg_misc.c`: `ammoTable` is sized to every weapon. It stopped five
  short, and `Fill_Clip`, run for every weapon as each character of a
  cutscene is spawned, read past its end (found with AddressSanitizer);
  what lies there differs from one build and machine to the next.

## 0006: say what the game could not find, and carry on

On the tester's Mac, with only RTCW's own files, escape1 then stopped as
it started with "RegisterItem: NULL": a weapon an AI script gives
(`giveweapon weapon_luger` and the like) was not found. Linux, and a
Mac test linking the game module's own objects, find every one.

- `game/ai_cast_script.c`: a weapon the script gives that is not found
  is passed over with a warning, not a fatal error.
- That warning, and the animation scripts' from 0005, also go into the
  cvar `eta_problem`: what was not found, its length in letters, and
  where. `cgame/cg_accessibility.c` says it once, "Problem: ...", so a
  tester who cannot read the console can pass it on.

## 0007: find the objectives on any machine

On the tester's Mac, F2 said there were no objectives for escape1, where
Linux reads both. `cgame/cg_accessibility.c` now looks harder:

- The map's name comes from the server, else the `mapname` cvar, without
  a `maps/` folder or `.bsp` ending.
- The map's page in `ui/briefing.menu` is matched letter by letter in
  the file itself.
- If the menu still names no page for the map, a table of the
  campaign's 26 maps names it, as the menu does (escape1 is
  `text/EnglishUSA/m1_l1_br_p1a.txt`).
- Asking again, F1 or F2, looks again if nothing was found the first
  time.
- When none are found, F2 says why: the map's name and its length,
  the menu's size and how many map pages it holds, or the objectives
  file that could not be read. A tester can pass that on.

## 0008: say a page as one line

On the tester's Mac the briefing was silent but for its last words.
VoiceOver keeps only the last of several announcements made at once, and
the page went as its heading, its text in pieces, and the key to press.

- `ui/ui_main.c`: the briefing's heading, page and "Press Enter" are one
  line, as are a page of reading's name and its words.
- `src/accessibility/speech`: on the Mac, `ETA_Speech_SayLong` sends the
  whole text as one line rather than in pieces.

## 0009: describe the cutscenes

Audio description: what is seen in a cutscene is said when it happens,
in the quiet between the lines the people in it speak - the place, who
is there, what is shown on the briefing room's screen. The intro movie
that plays when the game starts is described too. A shot with nothing
new to see, the same faces talking, says nothing, and nothing is said
over a character speaking.

- `cgame/cg_accessibility_cutscenes.h`: the lines, written by watching
  each cutscene. In the cutscene maps (`cutscene1` to `cutscene19`) a
  line is keyed to the shot's number, counted as each camera starts: the
  game plays them in the same order every time, and uses the same camera
  again for another moment. In a mission, which a saved game may be
  loaded into part way, to the camera's own name.
  Each line has a delay into its shot, so it is said when what it
  describes is seen (a man coming in, not before he does), and is timed
  against the cutscene's dialogue, which the scripts play the same every
  time, to end before the next line spoken - so most lines are short.
- `cgame/cg_consolecmds.c`: `CG_StartCamera` tells the module each shot
  as it starts (`CG_AccessibilityCamera`), which queues its lines. The
  first one said comes with "Cutscene. Escape skips it", as one line, for
  VoiceOver.
- `cgame/cg_sound.c`, `cgame/cg_event.c`: a sound script's voice line,
  or a general sound, played during a cutscene tells the module
  (`CG_AccessibilityVoice`), which reads its length from the WAV header;
  a waiting line is held until the speaking stops. Sound effects on the
  voice channel - a knock, a slide changing, thunder, a growl - do not
  hold it.
- The intro movie: `client/cl_cin.c` names the movie playing
  (`eta_cinematic`) and how long it has played
  (`CIN_AccessibilityElapsed`), and `client/cl_scrn.c` passes that to the
  ui module each frame of a full-screen movie
  (`UI_ACCESSIBILITY_CINEMATIC`, added to `ui/ui_public.h`), which says
  the description's lines in `ui/ui_accessibility_intro.h` as their times
  come, clear of the four times the movie speaks.
- For writing more: `eta_cameraShots 1` takes a picture as each shot
  starts and every two and a half seconds after, named for the map, the
  shot's number and its camera, and the console notes each camera.

## 0010: get around

The ET: Legacy mode's getting around, by way of its No Quarter port
(`integration/noquarter/0007`), in three new files in `SP/code/cgame/`,
built only with `FEATURE_ACCESSIBILITY`.

- `cg_accessibility_world.c`: which way the player faces, as it changes;
  what is in the way when they walk into it, and a thud or knock as they
  bump into it; steps, stairs, drops and how far a fall would hurt, and
  water and its depth; the kind of place (`navigation/place_classifier`)
  and the ways opening and closing to the sides; furniture named as the
  player comes up to it, from the map file (`map/bsp_props`); enemies in
  sight, where and how far, and friends close by; a tick while the
  crosshair is on an enemy; and the sounds that stay where things are -
  on each door, each ladder (`map/bsp_ladders`), and the health,
  ammunition, weapons and treasure lying close by. F3 says what is
  around: indoors or out, enemies in sight, the ways out, and the things
  close by.
- `cg_accessibility_keys.c`: a tap of a turn key turns to the next
  compass point and says it; held, the key turns as the game's own does.

Where the campaign differs from Enemy Territory:

- **Doors.** The campaign's server works out a cursor hint only for what
  the player looks at, and stamps no door on its entity, so the doors
  are read from the map's own entity list (`func_door`,
  `func_door_rotating`): a door's brush model is the number after the
  `*` in its "model" key. One set locked in the map (`key -1`) has the
  locked creak; one a script unlocks later is not known to be locked
  until the crosshair says so.
- **People.** Every character is the AI's: friend or enemy by the side
  the game gave it (`teamNum`, AITEAM_*; the player is with the Allies),
  named by its kind (`aiChar`): Soldier, Elite guard, Zombie, Loper,
  Partisan and so on. Friends are "Friend", not "Teammate"
  (`ETA_HudNarrator_SetFriendWords`). A fallen friend is not said: there
  is no medic to wait for.
- **Sounds.** RTCW keeps one looping sound to an entity number. The
  marking sounds take numbers of their own for the frame, counting down
  from just under the world's, where the maps never reach. A one-off
  sound has no volume of its own, so the bump is at full volume.
- **No place names.** The campaign's maps name no places and have no
  command map, so where the player is is the kind of place, and whether
  it is open sky.
- Things that are Enemy Territory's alone - flags, construction and
  dynamite sites, mines, disguises, cabinets, the command map - are left
  out of the copy.

`cg_accessibility.c` gains the settings (`eta_facingCallouts`,
`eta_doorCues` and the rest, and their volumes) and the looping sounds,
which `release/make-cue-sounds.py` makes into the download's
`main/sp_zaccessibility.pk3`: named `sp_` so the campaign's file system,
which loads only its own kinds of name, takes it, and after the game's
own. The world is read on the map's first frame, and nothing is said in
its first second and a half, which a cutscene may be about to take.

The menus offer a second generation of keys: F3, O, and the Left and Right
arrows - only where they are still bound to the game's own `+left` and
`+right`, which they keep doing while held.

- `cg_accessibility_route.c`, from the No Quarter port's
  `cg_accessibility_map.c`: the marker and the way to it round the walls
  (`navigation/navigation_system`'s path finder over half-metre squares,
  up the map's ladders), heard as a ping to turn toward - its sound by
  where it lies, ahead, to a side or behind - and the stairs, jumps and
  low places on the way said as they come. Without the command map there
  are no squares to put it on: **O** (`accessibilityWayOut`) puts it on
  the level's way out, the player's own `ai_trigger` aimed at `endmap`,
  read from the map's entity list, and says how far and which way;
  pressed again on the way, which way to turn. A map that ends by what
  happens in it, not by walking out, says so.

## 0011: turn to things, climb, and lock on

The rest of the No Quarter port's keys (`integration/noquarter/0007`),
in `cg_accessibility_keys.c`, and the lists they work from, in
`cg_accessibility_world.c`:

- **The use key** (`+accessibilityActivate`): with something under the
  crosshair the game offers to use, it is used; with nothing, the
  nearest thing to use within fifteen metres is turned to and said -
  name, direction, distance, "1 of 3" - and used on arrival when in
  reach. Further off, the view is kept on it while the player walks, and
  "in reach, F to use" said there; a thing on the ground the game picks
  up by itself as the player steps on it is let go quietly. Pressed
  again within four seconds, the next one along. At a ladder it climbs
  it, up or down, pressing the walk and look keys for the player under
  key numbers no keyboard has, and says the top or the bottom.
- **What there is to use** (`CG_AccessibilityUseTargets`): doors, from
  the map's own list as before (a locked one is said and never pressed
  at); buttons, which the server sends as movers like any door, known
  from the map's `func_button`s; the switches and levers a map hides in
  a `func_invisible_user` the server never sends, from the map's list
  too, named by their `cursorhint` or by what they work; mounted guns;
  things on the ground; the map's ladders; and flights of stairs.
- **The enemy lock** (`+accessibilityAimLock`, held): the nearest enemy
  in sight is said and followed by the turn and look keys; the next is
  taken when one falls. In Enemy Territory it ran only in a game the
  player hosted, being aim assistance against other people; the campaign
  has only the game's own soldiers, so here it always may. It also locks
  on by itself to an enemy coming into sight within forty metres
  (`eta_autoLock`, on by default), silently, and lets go for two seconds
  after a turn by hand. A cutscene stands both down.
- `accessibilityUseNext` and `accessibilityUsePrev` step through the
  list without turning, for F to take the one chosen; they are not bound
  by default, since the game's own `[` and `]` change weapons.

The menus offer a third generation of keys: F and Enter, only where
they are still bound to the game's own `+activate`, and Insert, free in
the stock config.

## 0012: lead the way

A tester, in the first mission's dungeon, heard the walls and openings
and still could not get out: they got lost between the walls. So the
way the marker's ping already follows can now lead them.

- **The walk guide** (`cg_accessibility_keys.c`, `eta_walkGuide`): with a
  marker set - O puts it on the way out - and the walk key held, the
  view is turned along the way by the same turn keys the use key and the
  enemy lock press, to the point the ping is on
  (`CG_AccessibilityMarkerSteerPoint`). Only while walking forward; a
  turn by hand takes the view back for two seconds, and the lock, F and a
  climb come first.
- **Doors on the way** are opened with use as they are reached
  (`CG_AccessibilityDoorAhead`, a door the server sends or one the map
  opens through an invisible trigger). A door opened from elsewhere - a
  button on the wall beside a gate - is gone round by: the map's list
  says which button or trigger opens which door (`eta_doorOpener`, read
  in `ETA_ReadMapEntities`), the player is taken to it, and it is
  pressed as F would. A switch that cannot be come any nearer - the
  dungeon has one behind its own door - makes that door no way through.
- **Stuck**: walking and getting nowhere, the spot just ahead is kept
  clear of from then on (`CG_AccessibilityMarkerBlocked`, until the map
  changes) and the way looked for round it - the search lets a low
  stone block through that the player cannot get past.
- Also: a person in the way is no longer said by the obstacle callout,
  which took turns with the callouts for people, and a character met
  head-on is named by what they are and which side - the client's team
  field, which Enemy Territory's players have, is the same for every
  character in the campaign, and called a guard a teammate.

## 0013: choose, and shoot back

The tester, who plays 1428: Shadows over Silesia, asked for things to
use and enemies to work the same way there: a key to step through what
there is, and a key to snap to the one chosen. And in gunfights they
were dying while still turning to find who was shooting.

- **Things to use**: `accessibilityUseNext` and `accessibilityUsePrev`,
  already there, are bound to K and J; F turns to the one chosen.
- **Enemies** (`accessibilityEnemyNext`, `accessibilityEnemyPrev`, on I
  and U): every enemy in clear sight all round, nearest first, each said
  with where and how far, without turning; the lock (Insert) takes the
  one chosen. The lock asked for by a key now takes an enemy anywhere
  round, not only in view - a blind player has no glance over the
  shoulder - while the lock by itself keeps to the view.
- **Shot**: a hit says who by and where from, from the game's own
  `PERS_ATTACKER`, and with the lock by itself on the aim is turned onto
  them wherever they are, if they can be seen. A lock the player holds on
  someone else is left alone.
- **Faster**: the keys' quick turn goes at eight times the game's turn
  speed, not five - a half turn in about a sixth of a second.
- A character is named by what they are in the lock and the list -
  "Soldier", "Elite guard" - and two of the same are never one entry.

The menus offer a fourth generation of keys: I, J, K and U, free in the
stock config.

## 0014: keep the guide on the way

The tester, led by the walk guide, found it walking them round by
itself and getting stuck - as lost as before. Traced in the first
mission, the time went in three places, each fixed:

- **A spiral stair.** The point of the way the player was taken to be
  at was the nearest in a straight line - on the next turn of the stair,
  straight overhead - and the point steered to was one the straight line
  reached only up the stairwell. The nearest point now counts a floor
  above or below as far as it would be to walk, and a point ahead is
  steered to only if the straight line is not much shorter than the way
  there (`ETA_RouteFollow`). The stair took over a minute; it takes
  seconds.
- **Jumps, low places and ladders** were said - "Jump up, press space",
  "Ladder up, F climbs" - and left to the player, who was being led and
  did not press them. The guide now presses jump at the edge of a jump
  (`CG_AccessibilityMarkerJumpNow`), holds crouch while the way goes
  under something low (`CG_AccessibilityMarkerCrouchNow`), and climbs a
  ladder the way goes up or down when the player is at it
  (`CG_AccessibilityMarkerLadderNext`, the use key's own climb).
- **Stuck.** Held against something, a jump is tried before the spot is
  kept clear of; and rocking back and forth against a wall, which moved
  the player just enough to never look stuck, now counts as stuck after
  three seconds in one spot. Standing on the way's next point no longer
  lets the guide go quiet without checking.

## 0015: the way told, the walking yours

Led by the walk guide, the tester asked not to have their autonomy
taken away entirely. So the guide's steering is off unless asked for
(`eta_autoSteer`, a new name, so an archived `eta_walkGuide 1` does not
keep it on), and the way is told instead while the player walks it
(`ETA_WayCueFrame`):

- a turn of the way is said as it comes - "Way goes left", "right",
  "back" - once it has held a moment, and not again until it changes;
- held still while walking, what the way needs there is said with its
  key - a door to open with F, a jump, a ladder, a low place - or, facing
  the way and held against something, it is kept clear of and the way
  found round it (`CG_AccessibilityMarkerBlocked`);
- a door that opens from a switch elsewhere says so and chooses the
  switch for F; one whose switch cannot be seen from this side - behind
  the door itself - is no way through from here.

O, pressed again, turns the player to face the way once
(`CG_AccessibilityFaceTheWay`) and says how far is left, in place of the
long "which way to turn" it said before.

## 0016: waypoints, and the way faced only once it is known

- O pressed straight after the way out was set turned the tester to face
  the exit through the walls - the way round them was still being
  searched for, and until it is found the ping points straight at the
  marker - and W took them the wrong way. Now O waits for the way
  (`CG_AccessibilityMarkerWayKnown`), says "Finding the way. A moment",
  and turns them as soon as it is found.
- Waypoints, as 1428: Shadows over Silesia has them: P
  (`accessibilityWaypointPlace`) marks where the player stands, up to
  nine; L (`accessibilityWaypointNext`) steps through the way out and the
  waypoints, each said with how far and where, and puts the marker on
  the one chosen, so the ping, the way told while walking and O all lead
  there. A new map starts with none. The menus offer a fifth generation
  of keys: P and L, free in the stock config.

## 0017: update from the menus, on the Mac

Every test build downloaded with a browser is quarantined, and the
tester had to find Open Anyway in System Settings for each one. A build
the game downloads itself is not. So a few seconds into the menus the ui
reads `rtcw-update.txt` from the downloads page's `rtcw-updates`
release, which the test build workflow writes - the newest build's
number, its Mac download, the download's SHA-256 and size - and, for a
build with a higher number than its own (`ETA_RELEASE_VERSION`, baked in
by the workflow; a build made by hand is "dev" and never checks), says
"Test build N is ready. F12 installs it". F12 downloads it beside the
game files, checks the size and the checksum, and hands it to a helper
(`ETA_MacUpdate_ScheduleAppReplace`, in the library's
`platform/macos/macos_update.m`) that waits for the game to close,
unzips it with ditto, puts the new app in place of the running one -
keeping the old one until the new one is in - copies the download's
sounds pk3 and guide beside it, and opens it. An app the Mac is running
from a translocated copy, never moved out of where it was unzipped,
cannot be replaced there, and the player is told to move it first.

Mac only for now: on Windows nothing asks before a new build runs.

## 0018: breakables, the facing key, and the stair keeper

- **Breakables on the use list.** The map's `func_explosive` entities
  with health (the game's default is 100) are read from the entity list
  by brush model, named by their `type` - "Breakable wood", "Breakable
  glass" - or by spawnflag 32 or 64, "Breakable, needs explosives",
  "Breakable, needs dynamite". The server sends them as `ET_EXPLOSIVE`,
  and while one is still there it is on the F, K and J list. F turns to
  it, aimed at its middle, to shoot or knife; it is never pressed at, as
  a locked door is not.
- **H says which way you face.** The compass point, the view tipped up
  or down, stairs going up or down, and what the crosshair is on with
  how far. A sixth generation of keys offers H, free in the stock config.
- **The stair keeper.** A turning stair a sighted player follows with
  the mouse; the tester, with W alone, walked into the outer wall at
  every turn. On stairs - the player rose or fell 14 units or more in
  the last 0.7 seconds, walking - with W down and no strafing, the way
  ahead is tried as the game would walk it: up to eight 16-unit strides,
  each stepping up a riser and down onto the next, with an edge where no
  floor is within 40 units below. Straight on going on up (or down), it
  does nothing; straight into a wall or off an edge, the view is turned
  the least way, up to 110 degrees either side, that keeps on the
  steps - the way the flight last bent first - and "Stairs bend left" is
  said at the bend. More than 0.6 seconds since the last step, only a way
  toward more steps counts, so it lets go on the flat floor at the top
  and the foot. Never while the arrows turn, another key has the turn
  keys, or the walk guide steers. `eta_stairKeeper 0` turns it off.
  Traced on the dungeon's square tower stair in the first mission: with
  it, W alone climbed the whole stair in about five seconds, turning at
  each corner, and came back down; without it, the player stopped at the
  first wall.

## 0019: keep what was chosen

The tester chose a thing with K, pressed F, and was turned to something
else close by. Two causes. F found the choice by looking it up on the use
list, which holds only things in sight from where the player stands now;
walked a little, round a door frame, the chosen thing was off the list,
and F went on to the nearest one instead. Now F never goes on: a chosen
entity the game still has, or a chosen switch or ladder, within about 25
metres, is turned to where it is - "Door, out of sight from here" - and
stays chosen; otherwise F says it is no longer close by, and that K
chooses again. And the way told while walking chose a door's switch for
F when the player was held at the door, over the player's own choice;
a choice made with K or J is now never taken over, and the switch is only
said. A breakable or a locked door, never pressed at, now stays chosen
after F too, so a second F stays on it.

## 0020: things and enemies apart

Patch 0019 did not cure it: the tester chose with K, pressed F, and was
still taken off what they chose. Things and enemies shared one aim, and
the lock that takes an enemy in sight by itself took it as soon as F's
turn was done, as did being shot. Now, while a thing chosen with K or J
is held, while F keeps the player facing a thing being walked to, and
for six seconds after F turns to one (`ETA_ItemFocused`), the lock does
not take the aim by itself, and being shot is said - "Hit by Soldier.
Insert turns to them" - without turning. Insert still locks on at once.

## 0021: U locks on

The tester plays on a laptop, which has no Insert, and asked for the
enemies to have a key of their own the way F is for things. U now locks
on - to the enemy chosen with I or Y, or else the nearest in sight - and
the lock stays without holding the key, onto the next when one falls,
until U again ("Lock off"), a turn with the arrows, or no enemy left in
sight ("Lock lost"). Stepping back through the enemies moves from U to
Y, free in the stock config. A seventh generation of keys offers both;
U is taken over only where it is still on the enemy step it had. Insert,
held, still locks while held.

## 0022: F, the nearest only

The tester asked that F be the nearest thing only, the ones further off
being K's and J's. F pressed again within four seconds went on to the
next one along, which took them off the thing they were on; now it is
the nearest every time, and said without "1 of 3". And F was heard
naming enemies: an enemy further than ten metres was said only once in
view, so turning the player - with F, or the lock - brought them into
view and their names with them. Enemies in sight are now said whichever
way the player faces; I, Y and U are the enemy keys.

## 0023: M switches the enemy callouts

The tester asked to be spared the console, and the mode has no page in
the game's menus. M switches `eta_playerCallouts` off and on, said either
way and archived. An eighth generation of keys offers it; M is free in
the stock config.

## 0024: O says which way, and never turns

O pressed again turned the player onto the way, and turned the tester
into a wall. The Enemy Territory mode never turns the player toward the
marker, nor does 1428: a ping leads, and the player turns. Now O again
says it in the Enemy Territory mode's words, through the same
`CG_AccessibilityMarkerWhere` - how far, "Turn left toward the marker",
"The way goes east", "Marker lies north, you are facing north" - waiting
for the way round the walls first, as before, and turns nothing.

## 0025: a quieter ping, and Backspace takes the marker away

The tester asked to rethink getting around: the ping gave them a
headache, and the Enemy Territory mode let them take the marker away
(Keypad star), which RTCW could not. Backspace now takes it away -
`CG_AccessibilityMarkerClear`, "Marker removed" - offered as a ninth
generation of keys, since a laptop has no keypad. And the ping is quiet
while the player walks the way: when the sound that would play is the
one for ahead and the player is moving, it is not played; it sounds
when the way is off to a side or behind, and when they stand still.

## 0026: turned to an opening, and waypoints with names

The tester explained 1428's N: it turns the player toward the waypoint,
and always to an opening, never into a wall. So O again turns the player
toward the way once more, but the way's direction is first tried as the
game would walk it (`ETA_OpeningToward`, the stair keeper's probe):
where a wall or an edge is in the way within about three metres, the
nearest yaw either side that is open, up to a quarter turn, is faced
instead - "Facing the opening nearest the way". With nothing open, it
only says.

And waypoints have names. P names one by the thing to use within three
metres - "Waypoint 2 set, by the button". A locked door F turns the
player to, or one the way finds opens only from the other side, is kept
as a waypoint by itself, once, on the floor in front of it - "Waypoint
3: locked door. L finds it again" - for the tester's case: the switch
found far off, and the door no longer remembered. L says each with its
name.

## 0027: F for what is close, and the facing said less

The tester asked again that F be for what is close and K and J for the
rest, up to fifteen metres. F alone now turns only to a thing within four
metres (`ETA_F_CLOSE_METERS`); further off it says "Nothing close. K
finds things further off" and presses use, as the plain key would. A
thing chosen with K or J is still turned to at any distance.

And the narrator said too much about facing. The facing callouts, said
by themselves as the view turns, are off by default - changed once for
an existing config through `eta_settingsVersion`, so a player who turns
them back on keeps them. A tap on an arrow still says the point it
lands on, now whatever that setting, and H says it on asking.

## 0028: U put back on the lock

The tester pressed U to be turned to an enemy and heard only where the
enemy was. U had kept the step back through the enemies it was given
first, in a config where the seventh generation of keys did not move it.
Now, at every start, wherever U is still on `accessibilityEnemyPrev` it
is put on `accessibilityEnemyLock`, and the step goes to Y where Y is
free. And stepping through the enemies with I or Y says what turns to
them - "Soldier, 1 of 2, behind left, 12 meters. U turns to them" - so
choosing is not taken for being turned.

## 0029: the Steam and GOG menus, and O said short

The tester heard menus that were not the game's. Their sp_pak4.pk3 is the
Steam and GOG releases' (2016): its menus carry `scralign` - how that
release's own engine stretches a piece across a wide screen - which this
UI did not know, an error at each, and a video mode list of 36, more than
`MAX_MULTI_CVARS` (32), so the system menus failed to load. `scralign`
and its word are now read and set aside (this engine scales the whole
640 by 480 menu), and the limit is 64. Checked with the tester's own
game files: the menus load without an error.

And after O turns the player toward the way, only where it leads is
said - "Facing the way. The way out, 143 meters, 24 meters above you" -
not the marker's whole report.

## 0030: one use, waypoints chosen before they are gone to

The tester opened a door with its button and heard it lock again, each
time a key went near F. G is the game's kick, beside F, and a kick works
a button as use does; a switch with no entity of its own also stayed
chosen after use, and the next F pressed it again. Now a chosen thing is
let go once used, whatever it is; F pressed again within fifteen seconds
and a couple of metres of what it just used says "Just used. F twice
quickly uses it again" and presses nothing, unless pressed twice
quickly or the player has chosen something with K or J; and a tenth
generation of keys lets go of G where it is still on `+kick` (X still
kicks).

And waypoints are chosen before they are gone to. L stepping through
them started the marker and its ping at each; now L only says each -
"Waypoint 1, by the luger, 5 meters, ahead. O leads there" - and O sets
the marker on the one chosen. A waypoint is named for the thing to use
nearest it within six metres ("by the stairs up"), else a thing the map
names in the room within four, else the kind of place when it is more
than "indoors".

## 0031: the button chosen again with K, and G kicks again

The tester's report was of J and K, not G: the button they had just
pressed was still the nearest thing, so K or J chose it next, and F -
which let a thing chosen with K or J through - pressed it again. Now the
one F has just used (its entity, or where it is within 48 units) is held
back even when chosen, and K and J say "just used" of it. Checked at the
first mission's cell switch with the tester's own game files. The kick
goes back onto G (an eleventh generation of keys, where G is empty).

## 0032: getting closer, and where the player has been

The tester asked how to know they were going the right way, and not
round in circles. Two things answer it. With a marker, every twelve
seconds of walking (six metres or more) how far is left is compared
with before - along the way round the walls once it is known, as the
crow flies until then, and never one against the other - and said:
"Getting closer", "Getting further", "Not getting closer". And a trail
is kept, a point every two seconds a metre or more on from the last,
twenty minutes of it: walking back onto a point a minute old or more is
said with how long ago, "You were here 3 minutes ago", not again for
half a minute nor until the player has walked fifteen metres; coming
back to a waypoint is said by its name, "Back at waypoint 1, by the
luger". Checked in the first mission with the tester's own game files.

## 0033: the lock lets go, and things heard where they are

Three from the tester. The lock pressed on with U stayed on a dead enemy
- or went on to the next - until U again; now an enemy fallen under the
lock ends it: "Down. Lock off" (a lock taken by itself ends silently).
Checked on a soldier in the first mission.

And two sounds, from the games the tester plays. As 1428 sounds its
chosen point, the thing chosen with K or J plays `chosen_beacon.wav` - a
soft two-note beep, quicker and gentler than the marker's ping - from
where it is, out of sight too, until the player is in reach, uses it, or
chooses another. As BROK the InvestiGator plays its fights, each enemy
in clear sight within 25 metres, all round, plays `enemy_presence.wav` -
a low double pulse, unlike the crosshair's bright tick - from where they
stand, at half the crosshair cues' volume; looked for ten times a
second, six at most. `eta_enemyCues` turns it off, and M switches it
with the enemy callouts. Both sounds are made by
`release/make-cue-sounds.py` with the rest.

## 0034: one beep for a choice, and what each thing is for

1428 had no beep all the way to a chosen point, the tester said; 0033's
was. The thing chosen with K or J now beeps once, from where it is, as
it is chosen. And, from BROK the InvestiGator, where each thing in a
scene says what it is for: the map's links, read for the door openers,
are kept the other way round too (`eta_opensDoor`, a switch's or a
button's brush model to the door it works), so a button or switch on the
list is named with its door and where that door is - "Button for a door
ahead right 4 meters" - and a door worked from a switch elsewhere is
"Door that opens with a switch". Checked at the first mission's cell
switch. Waypoints named for such a thing keep only "button".

## 0035: waypoints onward or back

The tester went round in circles, not knowing which waypoint led on
through the level and which led back. L now says of each waypoint
whether it is "on the way out" - nearer the level's way out than the
player is, by more than five metres - or "back the way you came",
measuring as the crow flies with height counted double, since the way
out is often a floor or two up. Checked in the first mission: a waypoint
by the start said "back the way you came", one on the tower stair "on
the way out".

## 0036: ways on

The tester kept getting lost: the waypoints, the marker and the things
on the list each said where something was, and none said where it led.
A sighted player entering a room sees its ways out at once, and which
one they came in by. F3 now says that. From where the player stands the
floor is spread over in 32-unit squares, each step asked of the game's
collision as the walk guide's search does, as far as a walk of 14
metres. Where the spread is still going at that distance there is a way
on, and the squares it reached there side by side are one way - cut in
quarters when they spread wide, so open ground is not one way all round,
and joined again when their walks pass the same place half way, so a
corridor into a room is one. A door the spread reaches is a way too, even
into a dead end. Each way is named by what its walk goes through first:
a door (locked, or opened by a switch elsewhere), a rise of 40 units
(stairs, or a slope when no square rises a step on the last), walls
closer than 112 units each side (a doorway, or a corridor when it runs
128 units or more), or a ladder's foot or top; otherwise an open way.
Whether the player has been through a way is read off the trail of where
they walked; the one walked most recently is "where you came from".
"Toward the way out" goes on the one way whose far end is nearest the
level's way out, by more than five metres than the player is. F3 again
within ten seconds goes through them and chooses each for O. A look also
runs by itself, a few milliseconds a frame, each time the player has
walked five metres, and a new place - the trail not near it for half a
minute - with three ways on or more is said: "New place, 3 ways on".

The walk guide's step refuses a stair that rises two steps in one square
as a ledge to jump; the spread walks it in two halves, and climbs a step
it can walk back down. Squares are kept by their place on the grid, not
by where the step landed after being nudged to fit, which had a stair
come back on itself. Checked in the first mission - "Door that opens with
a switch ahead right, 3 meters" by the cell switch, "Stairs up ahead, 1
meter" at the tower's foot, "Stairs down behind, where you came from"
after walking up - and in the village: the cellar's stairs up, and a
double door said once, from both sides.

## 0037: the Accessibility menu

The tester asked for the settings to be in a menu, as the ET mode keeps
them under ACCESSIBILITY, rather than on keys or in the console. The
pages are the mode's own file, release/rtcw/ui/accessibility.menu, in
its pk3 with its sounds: a first page and four more - what you are told,
sounds, volumes, and moving and aiming - each a column of yes or no
rows, the volumes a choice of full, 75, 50, 25 percent or off. The
button that opens them goes into the game's own menus as they load
(UI_AccessibilityMenus), from release/rtcw/ui/accessibility_tabs.menu:
after OPTIONS in the main menu and after SYSTEM among the paused game's
tabs. The game's menu files are the game's and are not shipped, so the
button is one item parsed into the loaded menu and moved after the item
it follows, where the arrows come to it. Not among the Setup tabs OPTIONS
opens: that screen gives the keyboard to its Controls page at once, and
its tabs are out of the arrows' reach. The two names in each tab line
are bare: the menu reader joins quoted strings side by side into one,
which first made "main" "options" into "mainoptions".

The settings and their defaults are in one list,
cg_accessibility_settings.h, which the cgame registers from as before
and the ui now registers from too, so the pages show the right value
before any mission has loaded the cgame. The cgame now re-reads them
every frame - before, a setting changed during a mission took effect at
the next one. Escape on an Accessibility page goes back a page, and from
the first page to the menu it came from, rather than out of the paused
game's menus altogether as the game's own pages do. Checked from the
main menu and from the paused first mission: each page said on opening,
each row said with its value, a change said and kept in the config.

## 0038: readable menus and HUD

A sweep for what is on screen and never said.

In the menus: the Controls page took every key on a binding row, so the
arrows were stuck on its first row, and Escape too; a row now takes only
Enter (and the next key while it listens), and passes the rest to the
menu. Each binding is said in words - "Page Down", not the "PGDN" the
screen shows and a screen reader spells - and labels lose the colon and
spaces the screen puts before the value. A list - the saved games, the
mods - took Up and Down at its ends, so the buttons under it could not be
reached: Down from the last row and Up from the first now go on to the
next control. A button the game draws twice, the copy lit for the
pointer (BACK and back_alt), was said twice for one press; the same
words on the same screen within a quarter of a second are said once.
Escape on a page of the main menu's Options goes back to its tabs, which
the keyboard could not otherwise reach. The load screen's autosaves
switch, whose choices have no words, says yes or no, and the books' page
corners say what they are.

In a mission: the exit's mission figures (time, objectives, secret areas,
treasure, attempts, from CS_MISSIONSTATS) are said with "Exit", and again
with "Mission complete" when g_reloading says the next map is coming;
secret areas and treasure are counted as each is found; the item made
ready with the item keys, the stamina bar run out, a gun's heat bar near
the top, the weapon in hand out of ammo, and the dynamite's timer as it
is set, are said. The game keeps no air bar, but drowns a player twelve
seconds under water (g_active.c); with the head under, "Air running out"
is said at seven seconds and "Drowning" at twelve.

Checked: Controls walked from end to end and its Move tab; the Load
screen through to its buttons and back; Options' tabs, System and Game
options read; "Out of breath" after sprinting; the stats string parsed
as the game writes it.

## 0039: skill levels and headings

The new game's skill levels are said with what each is - easy, medium,
hard - after the game's jokes for them, by the items' names in
play_menu. A button with nothing to do - no action, no cvar, no focus
script, such as the Save screen's title "Save", said just before the
Save button - is passed over by the arrows. Checked: a game saved from
the paused first mission by the keyboard alone, "Game Saved" said.

## 0040: Escape back to the pause menu

In the paused game, Escape on one of its pages - Save, Load, Controls,
Options, System, Leave - closed every menu and put the player back in
the fight. It now closes the page and returns to the paused game's
menu, on the tab the page was opened from; Escape there resumes the
game, as before. Checked from the first mission: Controls and Options,
each left with Escape, the next Down landing on the tab after.

## 0041: the ways on with L

The tester asked why a way on needed F3 to choose, when L already went
through the places to go and they have names. L now goes through one
list: the ways on from where the player stands, nearest first, said as
F3 says them, then the way out, then the waypoints; semicolon, the key
beside L and free in the stock config, goes back through it (bind
generation 12). Choosing a way sets where O leads, as a waypoint does.
The list stays as it is while the player stands still, however long
the choosing takes, and starts again from its first way once they have
walked somewhere else (the look the ways come from, CG_AccessibilityWaysList,
is made again when the player is two metres from where it was made). F3
still says all the ways, and now ends "L and semicolon go through them";
F3 pressed again no longer picks. Checked in the first mission: L
through three ways on, the way out and a waypoint just set, semicolon
back, O leading to the door chosen.

## 0042: the ways numbered by kind

In 1428 the tester had what there was to find numbered, and asked to
know here how many doors there are. F3 now begins with the count of
each kind - "3 ways on: 2 doors, 1 doorway" - and so does the new-place
callout; each way of a kind there is more than one of is numbered,
nearest first - "Door 1 of 2, behind, 8 meters, new" - in F3's list and
as L and semicolon go through them. Checked in the first mission by the
soldier's corridor: two doors and a doorway, so said.

## 0043: explore with V

The tester asked how to move freely without getting lost, and chose two
things: a key that goes where they have not been, and how far they have
got. Every way on a look finds - F3's, L's, and the look taken as the
player walks - is now kept for the whole level, one entry for each way
however many looks see it (within 128 units of where it starts or where
it leads), and is tried once the player has been within 72 units of
either; a way the trail had already been through when it was found is
tried from the start. V leads to the nearest way not yet tried, wherever
it is - nearest as the crow flies, a floor counting double - by setting
the marker on it, so the ping and the walk guide take the player there
round the walls; V again within eight seconds, to the next nearest.
Reaching it is said with what is left. The progress - new ways left,
ways tried, and the level's doors gone through (within 56 units of a
door's middle) - is said with V and at the end of F1. V is free in the
stock config (bind generation 13).

Checked in the first mission: by the soldier's corridor, V named the
nearer of its two doors, V again the other; at the door, "New way
reached. 2 new ways left", and V then led to the next.

## 0044: the things left, and back after a fight

The tester wanted every item and everything there was to use, and lost
track of them: a fight took them away from a room, and nothing
remembered what was left in it. Everything K and J have on their list -
things on the ground, buttons and switches, things to break, not doors
(V's ways), ladders or stairs - is now looked at twice a second and kept
for the whole level (cg_accessibility_things.c). An item or a breakable
is done when its place is in plain view within 15 metres and the server
no longer sends it; a switch, when F presses it. T leads to the nearest
thing left by the marker's ping, T again within eight seconds to the
next; one within four metres is said with what to do with it instead
of a ping. The things left are said with T, on reaching one done, and
at the end of F1. T is free in the stock config (bind generation 14).

A fight begins when an enemy comes into sight after ten seconds with
none, and is over when none has been in sight for six; if the player
ends it more than four metres from where it began, "Fight over. O leads
back to where it began", and that place is chosen for O, as L chooses.

Checked in the first mission: T named the Luger at the start, "Walk
onto it"; walking onto it, "Done. Nothing else found to take or use".
By the cell switch, T listed the switch, a breakable and an item left.

## 0045: the things from the map

The tester nearly finished the first level, and could not find the
lever at its end after a fight: T knew only what had been on K and J's
list, within 15 metres and in sight, and the fight had kept them from
ever coming near it. The map's own list of what it holds now goes into
T's as the level starts (CG_AccessibilityMapThing): every item it places,
by its origin and the game's name for it, every switch or lever in an
invisible trigger that is not a door's own handle, and every button,
one entry where several share a place. A seeded item takes the entity
number of the item K and J later see there; until then it is done when
its place is in plain view within 15 metres with no item on it. The
first mission's list: 29 items and 7 switches. Checked: from the start,
T names the Luger, then switches across the level; from near the end,
"Switch, ahead, 12 meters" - the lever.


## 0046: O leads on, numbered places, and no way through a locked door

The tester was still getting lost near the end of the first level: the
guidance led them wrong, they could not tell where they were, and there
were too many keys. Played there with only what is said, as they play -
the walk guide off, O, W and F - a test player went round and round
between three rooms. Four causes, each fixed:

- Every key that leads somewhere shared O. Pressed after T or V, O
  turned the player to the thing they had chosen, not on. Now O is the
  way on: the way out, unless a place was chosen with L in the last 20
  seconds; a marker T, V or anything else set is replaced by the way
  out, and O again faces the way only on O's own marker. A fight over
  keeps where it began as a waypoint for L, instead of taking O.
- A door the map locks for good ("key" -1, which only the soldiers' own
  door triggers open) was no wall to the way, which uses the map's
  collision alone: from one side the way went through it, from the door
  itself round, and back again through. Closed locked doors are now
  walls to the way (ETA_RouteThroughLocked); one standing open, as a
  soldier left it, is not.
- Standing on the step before a door, the search's first square could
  take no step, and O said "No way found from here yet" for good. The
  search now starts where the player stands or a little off it, wherever
  a first step can be taken (ETA_RouteStartAt).
- O said how far the way out was, not where to turn, and the player
  walked past the corner. O now says the next turn - "Facing the way. 6
  meters, then left. The way out, 35 meters" (CG_AccessibilityMarkerTurn)
  - and walking up to it, "Left", a couple of metres before it. A
  closed door just ahead along the way is faced: "Facing the way. A
  door: F opens it".

Places are now numbered as they are come to - a look for the ways on
that finds three or more, or a switch - and named by a switch in them,
else a thing lying there, else their ways: "New place 4, by the switch.
3 ways on: 2 doors, 1 doorway", and coming back, "Back in place 4, by the
switch". F1 says which place the player is in. And the crosshair word is
deduplicated by the word: a door and its handle are two hints, both
"Door", and walking through doors said it eight times over.

Checked: a test player that only follows the speech - O at a turn word
or a wall, F at "F opens it", W in steps - reached the exit from the
rooms where the tester was lost in 49 seconds. Before, walking there
with O and W alone went round the same three rooms until the test ended.

## 0047: T turns you toward its thing; treasure and secret areas

The tester followed T's ping as they follow O's, pressed O to be turned
toward it - and, O now leading on, were turned to the way out. T now
turns the player toward the way to the thing it chose, as O does to the
way out (CG_AccessibilityFaceTheWay), with the next turn said; T again
within eight seconds of the last press chooses the next nearest, and T
later, while that thing is still the marker's and not done, turns the
player toward it again. V, the same for the way it chose.

Treasure is now its own kind on T's list: counted - "1 treasure, 2
items" - and named "Treasure" (the game's own name for it is "Treasure
Item"). The level's secret areas are read from the map: a trigger of the
player's own aimed at a "secretN" of the script, at its brush's middle;
or a script trigger aimed at one, at whatever fires it - a wall to break
("Secret area, through something to break"), or nothing of its own,
where it stands. One fired by picking up a treasure is that treasure, on
the list already. One just behind a door a switch opens says so: near
the end of the first level, the wall its lever lowers. A secret area is
found when the mission's figures count one more while the player is
within twelve metres of it. Within ten metres of one not found, "A
secret area is near, 8 meters, left" is said, once.

eta_secretsLead, in the Accessibility menu's Moving and aiming page as
"T leads straight to secret areas", is on by default; off, T leaves the
secret areas out of what it leads to and only the hint is said. A secret
area never names a place.

Checked in the first mission: from the room with the end lever, T lists
"5 treasure, 24 items, 7 switches, 4 secret areas", the secret behind
the lever's wall as "Secret area, behind a door a switch opens", and is
said near as the room is come to; with the setting off, T goes from the
switch to the button, past the secret.

## 0048: down the round stair, and stopped at the edge

The tester, going down a turning stair, still fell. The round stair by
the radio room in the first level showed why: walked straight, a round
stair carries the player out to its outer wall; the stair keeper turned
them only once the way ahead was shut, so they slid down along that
wall, and halfway down the gap between the wall and the steps caught
them - stuck in it, in the air, falling. Four changes to the keeper
(cg_accessibility_keys.c):

- The way on is tried a little slimmer than the player (6 units each
  side, not 2) and a hair above the feet: sliding along the curved wall
  the player's own box was already a fraction inside it, and every way
  tried from there began "in the wall", so the keeper never turned.
- It is tried first as a body 12 units wider each side, so the turn
  comes before the wall rather than at it, and as the player's own
  width only where the wide body fits nowhere.
- A wall close beside the player - found by a thin post at waist to
  chest height, square to each side and half way to ahead, so the treads
  stepping up beside the player on a round stair are not taken for one -
  is steered away from, up to 30 degrees, whether or not the way ahead
  is open, and nothing is said for it.
- On stairs is felt sooner: a step's worth up or down over 300 ms, not
  700; and held against a wall with W down, the steps going on to one
  side or the other count as stairs (the top landing of the round stair
  meets its wall two steps down). Held still against a wall on stairs,
  the keeper stays on.

And the edge guard (ETA_EdgeGuardFrame, eta_edgeGuard, "Stopped at the
edge of a drop" in Moving and aiming): walking forward on the ground,
the way ahead is tried as the keeper tries it, and where nothing is
underfoot within the distance the player needs to stop, and the drop
there would hurt (the drop warning's own fall-damage height) - or, on
stairs, is more than a few risers - the walk key is let go: "Edge. Drop
4 meters. W again goes over". W again there goes over. Never with jump
held, nor into water.

Checked on the round stair by the radio room, walking down from its top
with W alone, three times: before, each walk was caught against its
outer wall halfway down; now one went all the way to the floor at its
foot, and two left it by the radio room's doorway on the way. The tower
stair and the round stair walked up as before. The edge guard stopped
no walk down either stair; it has not yet been tried at a real drop.

## 0049: fewer keys

The tester, with some fifteen keys for getting around, still got lost,
and asked for fewer, merged. Getting around now takes O (go on), L
(choose where to go), T (things), F (use), U (fight) and F1 (where you
are). What the others did is folded in:

- F3's ways on are L's: the first L in a new place says how many of each
  kind there are, then the first way (CG_AccessibilityWaysSummaryText).
- V's nearest way never tried, anywhere in the level, is on L's list
  after the ways on from here and before the way out, unless it is one of
  them (CG_AccessibilityNewWayNearest): "Nearest way not yet tried: Door,
  behind left, 30 meters. O leads there".
- Semicolon's step back is L going round.
- H's facing is at the end of F1's place: "In place 3, by the switch, of
  5 found, facing west".
- K and J's things to use are T's further off and F's close by; I and
  Y's enemies are U's nearest.

The retired keys (bind generation 15) are let go of once, and only where
each is still on the mode's own command; the commands stay, for a player
who binds them. The spoken hints that named them now name L, T or F.

Checked in the first level: from the start of the room before the end,
L said "3 ways on: 1 open way, 1 locked door, 1 doorway. Open way
behind, new", then the doorway, the locked door, "Nearest way not yet
tried: Doorway, behind, 107 meters", and the way out; F1 ended its place
with "facing west"; and a config with generation 14 had F3, V, K, J, I,
Y and H let go of, and kept L, O, T and U.

## 0050: areas to explore, as in 1428

With the six keys of 0049 the tester was more lost than before: L had
the ways on, a new way, the way out and the waypoints all on one list,
and T had every thing in the level. They asked for the level to be
treated as areas to explore, as 1428: Shadows over Silesia has, with a
key set of its size - not too many, not too few.

The map compiler already cuts every map into areas, split only where a
door closes the way (an area portal). src/accessibility/map/bsp_areas.c
reads the map's planes, nodes and leaves, forward only as the ladders
are read, and finds a point's area by walking the tree down to its leaf.
cg_accessibility_areas.c finds each door's area on either side
(CG_AccessibilityDoorBox: the model's box moved to a turning door's
origin), merges the halves of a double door, and:

- says the area come into, once the player has been in it 0.7 seconds:
  "New area, area 3. 2 doors out, 1 to a new area. 4 things left here",
  or "Back in area 3. 1 thing left here". Areas are numbered as first
  come into; the first is not said over the start. The menu's "New area
  and its doors out" (eta_waysCallouts) keeps it quiet, and the numbered
  places are not said where a map has areas.
- X (bind generation 16, taken over only from +kick, which G keeps): the
  doors out of this area, nearest first, each "to a new area" or "back
  to area 2", "where the way out is" for the one into the way out's
  area, "locked" for one that will not open by hand; the marker goes
  just through it, and the player is turned toward the way, as T does.
  X again within 8 seconds and within 80 units of where it was pressed
  is the next door; X while walking toward one turns toward it again.
- T keeps to the area: "3 things left in this area", and with none,
  "Nothing left in this area. 12 things in other areas. X finds the
  doors out". A thing whose point is in a wall is looked for a little
  way off; one with no area found counts as here within 400 units.
- L is the waypoints alone; O has the way out and X the doors out.
- F1 says "In area 3, of 5 explored. 2 doors out, 1 to a new area" in
  place of the place and the ways tried, and the things left in the
  level.

Checked in the first level: 13 areas and 18 doors between them; at the
start X said "Area 1. 2 doors out, 2 to new areas. Door 1 of 2, to a
new area, ahead right, 59 meters, 10 meters above you" and turned the
player toward the way; T said "7 things left in this area: 4 items, 3
switches" of the level's 40.

## 0051: choose, then O leads, for everything

The tester, used to 1428, where keys go forward and back through the
items, as through the waypoints, and one more leads to the one chosen
while W walks, pressed T to choose and walked; T again on the way went
on to the next thing. So every list now only chooses, and O leads:

- T and Y (bind generation 17, Y only where it is still empty) go
  forward and back through the things left in the area, nearest first:
  "7 things left in this area: 4 items, 3 switches. 1 of 7, Luger,
  ahead, 4 meters. Walk onto it", "2 of 7, Switch, ahead right, 54
  meters. O leads there". The list keeps its order while it is gone
  through; it is made again in another area, or 30 seconds after the
  last press. Things done since are passed over.
- X goes round the doors out of the area the same way, and chooses the
  point just through the door.
- L and K (generation 17, K only where empty) are the waypoints, forward
  and back.
- O takes the choice, puts the marker on it and turns the player toward
  the way at once (CG_AccessibilityFaceTheWay); O again turns toward it
  again, as for the way out. The choice no longer lapses after 20
  seconds: it stands until O takes it, another is made, or Backspace
  lets go of it with the marker (CG_AccessibilityChosenClear).

Checked in the first level: T, T, T, Y said items 1, 2, 3 and 2 of 7;
O then said "Facing the way. 5 meters, then right. Switch, 54 meters, 6
meters above you"; X and X said doors 1 and 2 of 2; Backspace and O led
to the way out; Y, K and X were bound in a config from before.

## 0052: the new ways on X

The tester found it hard to find new ways to explore: with the areas,
L no longer had the ways on or the nearest way not yet tried, and X had
only the doors the map counts between areas - and an area can be a
whole floor of rooms and corridors. X's list is now what is new, nearest
first - the doors to areas not yet explored, and the ways found in this
area and never tried (CG_AccessibilityNewWayAt, from the frontier V
kept), less any at one of its doors - then the doors back to areas
explored. With nothing new in the area, the nearest way not yet tried
in another area (CG_AccessibilityNewWayNearest) comes last. A way tried
since the list was made is passed over. The first X in an area says
how many new ways there are: "Area 1. 2 doors out, 2 to new areas. 1
new way here. 1 of 3, new doorway, ahead, 4 meters. O leads there".

## 0053: what was explored, kept through a reload

The tester died or loaded a save, and X then called doors "to a new
area" that led into places they knew: the areas explored and the ways
found lived only as long as the cgame. They are now written, two
seconds after a change at most, to accessibility_explored.txt in the
home folder - the map's name, each area come into with its number, and
each way found with whether it was tried (CG_AccessibilityFrontierGet) -
and read back when the map's areas are read (CG_AccessibilityFrontierPut).
A file from another map is ignored, so coming to another level starts
fresh.

Checked in the first level: through the door from the start's area,
then the level started again: X said "2 doors out, 1 to a new area" and
"door back to area 2", and F1 "In area 1, of 2 explored".

## 0054: F turns to what T chose

The tester chose a switch with T, went there with O, and pressed F: F
turned them to the thing it had last been kept on, or to whatever was
nearest. F knew nothing of T's choice. Now:

- a new choice with T, Y, X, L or K lets go of what F was kept on
  (CG_AccessibilityUseChoiceClear);
- where O last led to a thing T or Y chose, for two minutes, F turns to
  the thing F finds within 40 units of it, within 4 meters
  (CG_AccessibilityLedTo); a door X chose is not a thing, its point
  being in the room beyond;
- and where F finds nothing there - a switch the map hides in an
  invisible trigger, which T knows from the map but F's look round does
  not - F aims at the point itself: used at once within reach, or turned
  to with "A step closer, and F uses it".

Checked in the first level, between a button and the button for a door
3.5 meters apart, with a door that opens with a switch beside them: F
on the button, then T to the button for a door and O; F had turned to
the door, and now said "Button for a door, ahead left, 3 meters. A step
closer, and F uses it"; a step on, F used it: "Done. T chooses the
next".

## 0055: F for what T chose, straight away

0054 had F turn to T's choice only once O had led there. The tester
chose with T and pressed F straight away, and F still turned to the
nearest thing. Now a thing chosen with T or Y, not yet gone to, or the
one O last led to (CG_AccessibilityThingChoice), is F's: further off
than F's 4 meters, F leads there as O does (CG_AccessibilityWayOut takes
the choice) or, once led, turns toward the way again - led again if the
marker has gone; close by, F turns to it and uses it, as in 0054. When
the thing is done, F lets go of it (CG_AccessibilityLedClear).

Checked in the first level: T to a switch 54 meters off, then F: "Facing
the way. 5 meters, then right. Switch, 54 meters"; by the buttons, T to
the button 2 meters off and F: "Button, right, 2 meters", then "Done".

## 0056: game files that are not the game's own, named

The tester heard the weapons reload, the footsteps and the main menu as
a remake has them, with the menu's theme gone: a pk3 that is not the
game's own, left in a main folder, wins over the original's sounds and
menus. Nothing in the mode ships or changes them, and the tester could
not look through the folder. Nine seconds into the menus, once, the
pk3s the game finds (trap_FS_GetFileList "" "pk3") are checked against
the campaign's own five, the multiplayer mp_ ones and the mode's
sp_zaccessibility.pk3, and any other is named, with where to move it
from (UI_AccessibilityExtraPaksFrame).

Checked with a z_realrtcw_test.pk3 put in the home main folder: "Your
game's main folder has a file that is not the original game's:
z_realrtcw_test.pk3. It can change the game's sounds, music and menus.
To have the original game, move it out of the main folder beside the
game"; without it, nothing is said.
