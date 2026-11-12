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

## 0057: F never falls back to the nearest while a thing is chosen

The tester, with a thing chosen with T close by, pressed F and was
turned to the stairs beside them. Three ways F still let go of the
choice:

- the thing close by but not in sight - F's trace to a switch set in a
  wall hit the wall - fell through to the nearest thing. A trace that
  ends within 40 units of the thing now counts as in sight, and one that
  does not leads there as O does (CG_AccessibilityWayOut), never to the
  nearest;
- "Just used" stopped F straight after something else had been pressed
  nearby, though a new thing had been chosen since (ETA_UseThingChosen).

Checked in the first level: F on the button, T to the button for a
door, F: "Button for a door, ahead left, 3 meters. A step closer, and F
uses it"; a step on, F: "Done".

## 0058: a script's unknown weapon passed over, not the end of the mission

The tester heard "Problem: a character is given ..., 12 letters, which
the game cannot find" through the first level, and at its way out was
put out of the game. The level's scripts there name weapons the game
does not have: a remake's scripts, from a pk3 in a main folder whose
name sorts after pak0.pk3 and so wins over the game's own maps/*.ai.
At parse time 0006 already passed over such a giveweapon; the actions
themselves - giveweapon, takeweapon, selectweapon, suggestweapon,
givearmor and giveinventory - still ended the mission with "AI
Scripting: ... unknown weapon". Each now prints a warning, sets
eta_problem, which the cgame says, and goes on (ETA_SCRIPT_UNKNOWN).
Both reports now say the likely cause: a file in the main folder that
is not the original game's.

Checked with a copy of escape1.ai giving the player weapon_realknife,
in an x_test.pk3 in the home main folder (a zz_ name is turned into sp_
by the engine's own sort, and skipped): the mission went on, and
"Problem: a character's script uses weapon_realknife, which this game
does not have. A file in the main folder that is not the original
game's is the likely cause" was said.

## 0059: F10 moves those files aside

The tester did not know which files were the remake's. The files 0056
names are kept, and F10 in the menus moves each into a folder "not the
original game" inside the main folder it is in - the home one or the
game's own - with rename(), so it can be put back by hand. The engine
reads only the pk3s directly in main, so they are no longer loaded once
the game starts again. Only the files the game itself found are moved;
one that cannot be moved is named, to be moved by hand.

Checked with an x_test.pk3 in the home main folder: "F10 moves it into
a folder of its own"; F10: "Moved x_test.pk3 into the folder not the
original game, inside the main folder. Quit the game and open it again
to have the original game"; the file was in main/not the original game,
and the next start said nothing.

## 0060: a ledge said once, until the player moves on

"Ledge ahead, 2 metres down" came again every four seconds while the
tester stood still at it, and again whenever something in front or a
turn broke the probe off it and back. A drop is now said again only
once the player has moved 3 meters from where it was last said, or it
is a different drop (ETA_DROP_SAME_PLACE).

## 0061: the intro back at start

The tester found the intro movie no longer started. The engine plays it
at start only while com_introplayed is 0, and the options' "Reset to
defaults" (resetDefaults, from the multiplayer menus) sets it to 1 for
good. With the mode, resetDefaults no longer does; and once (bind
generation 18), a com_introplayed of 1 is put back to 0 and the intro
is played there and then, Escape still skipping it.

Checked with com_introplayed 1 and generation 17 in the config: "Intro
movie. Escape skips it. Rain falls on dark rubble" at start, and
com_introplayed 0 and generation 18 written back.

## 0062: the alarm, quieter, and its box on F and T

An alarm going gave the tester a headache and drowned out what they
navigate by, and they could not tell whether its box was anywhere on
the lists. The alarm box (ET_ALARMBOX) was on neither.

- F's list has it: "Alarm box, ringing" while it rings, which F
  presses - pressing it switches the alarm off - and "Alarm box" while
  quiet, which F only turns to (locked), so it is never set ringing.
  One destroyed (frame 2) is left out. Its middle is in the wall it is
  set in, so it is seen from 12 units out toward the player.
- T's list has a ringing one, as a switch.
- The alarm's sound - the klaxon an alarm box gives the dlights it sets
  going, played by the cgame as a one-shot each light cycle - is played
  instead as a loop at eta_alarmVolume (default 50 percent; "The game's
  alarm volume" on the Volumes page), which a one-shot cannot be; off,
  not at all; at full, as the game plays it. A loop sound named alarm,
  klaxon or siren on any entity follows the same setting.

Checked in the first level by the alarm box off the start's corridor:
F said "Alarm box, ahead, 1 meter" and did not press it.

## 0063: the builds in the campaign's own repository

The test builds move from the Enemy Territory mode's releases page to
this repository's own, which is public for it: the Mac game's updater
reads rtcw-update.txt from this repository's rtcw-updates release
(UI_UPDATE_MANIFEST_URL), and the workflow publishes there. Builds from
before the move read the file on the old page, which the workflow keeps
updating to name the newest build here; the campaign's builds are taken
off the old page.

## 0064: back to the areas explored, and the status on four keys

The tester finished the first level, but came to its last area with a
rifle still left behind, and no key went back through the areas already
explored. And F1, with everything on it, was long to wait through; they
asked for it split, as 1428 has it.

- Semicolon (bind generation 19, only where empty): the areas explored
  other than this one, those with things left first, nearest first,
  then the rest, round again (CG_AccessibilityAreasBack); each is
  chosen for O, which leads to where the player first stood in it - a
  spot kept as the area is come into, and in accessibility_explored.txt
  - or, for one without, just through a door into it.
- F1: the area, the kind of place, what is left here and in the level,
  and the objectives done.
- H: health and armor. I: the weapon, its ammo, and what is carried.
  V: the facing and what is ahead, as before, and the coordinates in
  meters from the map's middle. All generation 19, only where empty.
  Z stays the game's alternate fire.

Checked in the first level: F1, H, I and V said as above; through the
door from the start's area, semicolon said "1 other area explored, 1
with things left. 1 of 1, area 1, 9 things left, ahead, 61 meters, 24
meters below you. O leads there", and O led there.

## 0065: what unlocks a door with no switch; areas cut at every door

In the second level the tester found a door locked and no switch for it
on T's list - there is none: the door unlocks when the player reaches a
place, or opens another door, through target_locks, relays and the
level's scripts. And T's list there was most of the level, since one of
the compiler's areas is most of the castle.

- cg_accessibility_unlock.c follows each locked door's target_lock, and
  each named door no button, switch or soldier's door trigger opens,
  back through the entities that aim at one another and through the
  alertentity and trigger lines of maps/<map>.ai and .script, to what
  the player can reach: an ai_trigger for the player, a trigger_once or
  trigger_multiple, or another door. Those are on T's list as "Spot
  that unlocks a door" (done once walked onto) and "Door that, opened,
  unlocks another". The second level has 7, the first 6.
- ETA_BspAreas_SplitAtDoors cuts the areas again at every door: the
  map's leaves that touch, in the same area and clear of every closed
  door's box, are one room; a room of fewer than three leaves is none.
  The second level has 16 areas and its start room 11 things, not 18;
  the first 15 areas and its start room 3 things, not 7. The explored
  file is marked "rooms", and one from before is not read.

## 0066: doors the script opens outright, and doors locked for good

Run over the campaign's 26 playable maps, 0065 left 49 of 137 locked or
named doors unexplained. Two causes: a locked door can also be opened
outright by a relay the script sets going, not only unlocked by its
target_lock, so that is now followed too; and 12 locked doors are never
unlocked by anything - a target_lock nothing fires, or none - scenery,
now said as "Door locked for good" by F and "locked for good" by X
(CG_AccessibilityDoorNeverOpens). 28 remain, opened as the story goes on
- an AI's own script, a scene - with no place for the player to go.

## 0067: the areas are the soldiers' rooms

Cut at every door, an area of the second level was still most of the
castle - 60 meters across, several floors, eleven things - since much
of it is joined by archways and corridors with no door; and the leaves'
boxes, which overlap through walls, joined more. The tester: each area
should have its own things.

The game's bots already have the level cut into rooms: the clusters of
maps/<map>_b0.aas, which their compiler divides at every door and every
doorway or narrow opening, a portal between two clusters.
ETA_BspAreas_LoadAas reads its planes, nodes, areas, area settings and
portals (the header unscrambled as be_aas_file.c does); a point's area
is its cluster, found by the bots' own tree, a player's middle above it
for an item on the floor, or the nearest piece within 2 meters. The
portals with no door near them are exits too, "opening to a new area",
led to the middle of the nearest piece of the room beyond; "doors out"
became "ways out". The second level has 37 areas, its start room none
of its things and one opening out; the first level 33, its start cell
the Luger alone. A map with no bots' file keeps 0065's areas. The
explored file is marked "bots".

### 0068 Spots that do nothing yet

The tester, in the second level: a spot T named unlocked nothing when
walked onto, and one T had named went from the list once reached. Many
of those spots are ai_triggers that start switched off (spawnflags 1):
the script wakes them later - in Castle Keep once 10 of the 11
soldiers of the first part are beaten, and 8 of the 10 of the next - and
until then walking onto one does nothing; T had it as done once stood
on.

The game now tells the client each brush model's trigger state in a
configstring of its own, CS_ETA_TRIGGERS (just past CS_MAX), every half
second: 'a' a trigger that works, 'o' one switched off, 'x' anything
else, '-' nothing, a trigger used and gone (G_AccessibilityTriggers in
g_main.c; a trigger switched off has no model index yet, so its "*N"
name is read). The unlock tracer records which ai_triggers start off
and follows what wakes them; the script reader keeps "accum N
abort_if_less_than K" before an alertentity, and the soldiers whose
death calls that event are counted. A spot switched off is said "not
working yet: it works once K of the N soldiers of this part are
beaten", or "something else in the level wakes it first"; it is done
only once its trigger has gone; and "A spot that unlocks a door works
now. T finds it" is said when one wakes.

### 0069 Where the soldiers come in

Loaded from the tester's own save of the second level: all eleven
soldiers of the first part beaten and the keep entered, the next door
waits for 8 of the next 10 - and none of those 10 were in the level
yet. They come in when the player walks into a room (the ai_triggers
calling the player's events that alert them, or a spawner's event that
does). So for each soldier the count waits on, the tracer follows what
brings that soldier in (its spawn, alerted) back to the places the
player walks to, and lists them as "Spot where soldiers come in", done
once their trigger has gone like the others. Where one stands at the
same place as a spot that unlocks a door, the door's comes first.
"Spot " now marks every place walked onto for T's "Walk onto it", and
one that wakes is said by its name.

### 0070 Things in a doorway

The tester, in the keep's upper corridor: T listed one thing there that
could not be reached at all. It was the wine room's "Spot where
soldiers come in", on the floor below: its trigger stands in the wine
room's doorway, a portal between two of the bots' clusters, so in no
area, and a thing in no area was counted in the player's own if within
10 meters - through the floor. CG_AccessibilityAreaHolds now puts a
thing in a doorway in the areas just either side of it, 48 and 96 units
out; only a thing with no area anywhere near is still counted by the
player, and then only on the same floor.

### 0071 Doors as the game has them now

The tester, in the courtyard, an area with ten ways out: each led to the
same locked door. The client knew a door locked only from the map's
"key" -1, read once: a door locked or unlocked later by a target_lock,
or one only a switch or the story opens (KEY_LOCKED_TARGET, the bridge
doors among them), was said as one that opens, and the way was found
through it. The game's configstring of 0068 now carries each door too:
'L' locked, 'S' opened only by a switch or the story, 'd' one that
opens by hand or stands open now. CG_AccessibilityDoorModel answers from
it, so F, X and the way (whose walls are the closed doors that do not
open) all follow the doors as they are. X lists a door that does not
open after the ways that can be gone through, and says one a switch
opens as "opened by a switch or by what happens".

### 0072 Pieces too small to be rooms

The tester: one step out of area seven was another area; should an area
not be a room with things in it? The bots' compiler cuts a cluster at
every narrow opening, so a stair, a ladder's shaft or the landing
between two doorways was an area of its own: ten of the second level's
37 had less than 16 square meters of floor. ETA_BspAreas_MergeSmall
(src/accessibility/map/bsp_areas.c) joins each such piece to the
biggest area it opens onto with no door within 128 units of the
opening, three passes for pieces that open only onto other small ones,
and numbers the areas again. The second level now has 31, its courtyard
seven ways out instead of ten. The explored file is marked "rooms2", so
it starts again once.

### 0073 Explored areas carried over

The tester, told the explored areas would start again with 0072: "but
you have my game saved for that". So a "bots" explored file is no longer
dropped: ETA_BspAreas_FromBefore gives the room each old piece is part
of now, its area and spot lines are moved there (two joined: the first
numbered keeps its number), and the areas are numbered again 1, 2, 3 in
the order they were found. It is saved as "rooms2" from then on.

### 0074 What opens a locked door, from the door

The tester, at a locked door: there is a switch for it, but T lists only
the area's own things, and the switch was in another. X now says what
opens a locked door out of the area and O leads there: "locked, a switch
opens it ... O leads to the switch", or "locked, a spot unlocks it ... O
leads to the spot", with why it does not work yet if it does not (the
unlock tracer now keeps the door each spot was found for,
CG_AccessibilityUnlockSpotForDoor). T lists that switch or spot with the
area's own things. One that is through the door, on its far side and in
another area - a door that unlocks once the player comes to it from
there - is said "locked from this side: what opens it is on the other
side, so find another way round", and not led to; a switch set in the
wall beside the door is on neither side, and is this one's.

### 0075 No way yet, crouch on a key, items not there

Three from the tester, still in the second level:

- Running in circles, with "Finding the way. A moment" again and again:
  from the wine cellar O led to the way out, beyond doors still locked.
  The search ended partial - every reachable square tried (the path
  finder now says so, ETA_PathFinder_AllTried) - and the player was
  turned toward the wall nearest the way out, and the search began again
  every five seconds. CG_AccessibilityMarkerUnreachable: O now says "No
  way to the way out from here yet: a locked door is in between, or a
  part of the level not open yet. X says what opens the doors here, and
  T what is left to do", and does not turn.
- Crouch held could not be done with one hand on the keys that move:
  C crouches and C again stands (accessibilityCrouch, its own key number
  on +movedown, bind generation 20, taken over from +movedown).
  "Crouched" and "Standing" are said as PMF_DUCKED changes, whatever
  made it, and "Too low to stand here" when a ceiling keeps the player
  down after C.
- Ammo T listed that was not there: an item with a targetname is not in
  the level until fired (FinishSpawningItem) - most often from a crate
  broken open - so it is named "..., inside something to break" (or
  "comes later in the level"), "Break what it is in first" within reach;
  and ammo or health the player is full of stays where it is walked
  onto (BG_CanItemBeGrabbed), so it is said "you carry all you can of
  this, so it stays here until you use some".

### 0076 What an area is like; things O cannot reach

The tester: why does T show things that cannot be reached, and why is
there no saying where one is but coordinates? No map of the campaign
names its places (no target_location in any of the 26), so an area is
now said as what it is like: how big, by the floor the bots' file has
to stand on (ETA_BspAreas_Floor: "a small room" under 25 square meters,
"a room", "a large room" under 300, "a very large space"), and how high
up in the level the player is, between the lowest and highest floor of
all its areas ("low down", "halfway up", "high up in the level"). F1:
"In area 3, a large room, high up in the level, of 5 explored", and the
same on coming into a new area.

T lists what the map lays out in the area, some of it out of reach from
where the player is: a health pack on a tower top of the courtyard, 8
meters over the highest square the search reached. When O finds no way
to a thing (0075), the thing is marked for that area for three minutes:
T lists it after the rest, saying "O found no way there from this area:
it may be up high, behind a locked door, or reached from somewhere
else".

### 0077 F at what cannot be taken; where on J

The tester pressed F at a weapon on the floor and nothing happened. The
game only lets a player take ammo, health, or a weapon already carried,
when they are not full of it (BG_CanItemBeGrabbed): F turned to it,
pressed use, and nothing was said. Now F says "You carry all you can of
this, so the game leaves it here. Use some, and come back", and does not
press. And the tester does not use the F keys: where they are (F1) is on
J as well, bind generation 21, J having been let go of in the fifteenth.

### 0078 What soldiers drop goes after 30 seconds

The tester, on the weapon F would not pick up: it was likely no longer
on the floor - its sound was not heard - and the ammo was not full. A
weapon a soldier drops is taken out of the game 30 seconds after it
falls (LaunchItem, G_FreeEntity), and T, which lets a thing go only
when seen gone in plain view, kept it on the list. A thing first seen
as a dropped item (otherEntityNum2 1) is now let go of 30 seconds after
it was first seen, and said with the time it has left: "MP40, dropped,
gone in 12 seconds".

### 0079 The level's next step on T

The tester, past the door toward the wine cellar, could not get anything
to lead there: T lists the area's own things, and the cellar's spot was
in another. T now ends its list - or, with nothing left in the area, is
- the level's next step: the nearest place to walk onto, anywhere, that
brings the soldiers the way on waits for or unlocks a door, working now
(CS_ETA_TRIGGERS), not used, not one O found no way to, and not just
through a locked door out of the area. "In another area, the level's
next step: Spot where soldiers come in, ahead, 11 meters. O leads
there". The far side of a door now counts from past its slab, 20 units
(0074 had 48): the spot behind the keep's locked door, 40 units through
it, was taken for this side's.

### 0080 F opens a door that swung shut again

The tester got to the wine cellar's door and was stuck there. F, within
15 seconds and a meter of what it last used, says "Just used. F twice
quickly uses it again" and presses nothing - made for a button, which
pressed again works its door back. But the castle's doors swing shut on
their own after a moment, and on a slow walk through, the door was shut
again and F would not open it: a walker led there by speech alone took
five presses. The rule no longer holds for a door F used itself. And a
door looked at or walked into is said "Door, F opens it"
(hud_narrator.c), not "Door" alone, which the walker heard 55 times
against a shut one.

### 0081 No spots on T; each level its own explored file

The tester: "Let's not spoil the story of the game with spots." The
places to walk onto that unlock a door or bring soldiers (0066, 0069)
and the level's next step (0079) are no longer on T's list, and a spot
coming to work is no longer said: T is items, switches, things to break
and secret areas again. X still says what opens a locked door.

And the explored areas were lost: one file kept them for the level
played last only, so a save of another level loaded even once lost the
rest. Each level has its own now, accessibility_explored_<map>.txt; one
with none yet takes the old file's if it was that level's, and saves it
as its own straight away.

### 0082 The tram's buttons

The tester, on the last level of the first mission, rode the tram, fell,
and back at its button could not press it again, even pressed quickly.
Two things in the way:

- F's "Just used" (0080 had kept it for buttons): the tram's buttons
  send the tram each way in turn and are meant to be pressed again. It
  now holds only for a switch that works a door
  (CG_AccessibilitySwitchOpensDoorAt), which pressed twice works it
  back.
- The tram's buttons are invisible switches (func_invisible_user) the
  level switches off while the tram is away or on its way, spawnflags 1:
  F at one did nothing, and nothing was said. The game's configstring
  (0068) now has them too, 'U' working and 'u' switched off; F and T say
  "Switch, switched off for now" and F does not press it.

### 0083 The tram

The tester pressed the tram's button on the platform and the tram left
without them: it moves, so the areas and the way have no part of it,
and nothing said where it was. A script mover the level names "tram",
big enough to ride, is now on F's list as "Tram, walk in" (or "Tram,
moving") at its floor's middle - turned to, never pressed at, and not on
T's; J says first "You are in the tram, standing" or "moving. Stay
inside"; stepping in or out says "In the tram", "Out of the tram"; and
its coming and going within 40 meters are said: "The tram has arrived,
12 meters, ahead left. F turns you to it", "The tram is leaving", and
inside, "The tram is moving. Stay inside", "The tram has stopped".

### 0084 F turns to the tram after its button

The tester: is the tram on the list, to follow with F as usual? It was,
but F turns to the nearest, and at the station that is the button: the
tram, a few meters off, was never reached, and F again on the button
would send the tram back. Now within 15 seconds of F pressing something,
and for half a minute after a tram stops near the player, F turns to the
tram instead: "Tram. Walk in, and stay inside, ahead, 5 meters".

### 0085 F stays on what was chosen, the tram or not

The tester: the tram on F took away their key for being turned toward
what they had chosen while walking, as 1428 has it. 0084 turned F to the
tram after any press within 15 seconds, over a thing chosen with T or Y.
Now only when nothing is chosen, and only after a press within 15 meters
of the tram - its own button - or just after it came in.

### 0086 The tram on T, not F

The tester: the tram is a thing to step into, like an area, not one to
use; F is for turning to things as 1428 has it, so leave it so, and put
the tram on T's list - it is only for this level - when it comes. The
tram is off F's list (0083-0085 undone there), and on T's while it
stands near, the player not in it, wherever the player is: "Tram, walk
in, ahead, 5 meters. O leads there"; within reach, "Walk in, and stay
inside". Not counted with the area's things. Its arrival says "T finds
it".

### 0087 T lists what was come across; F uses what is in reach ahead

The tester, after 0086: the tram could no longer be called, and T listed
things in places they could not reach - "findable items spoil the story,
but what you can see when you are around it is closer to investigating".

- The tram on T's list, chosen, took F over: F at the button led to the
  tram (both the far and the near choice paths), and the button could
  not be pressed. Something to use within 3.5 meters and in front, within
  50 degrees (ETA_UseReachAhead), is now used by F whatever T has
  chosen.
- T's things, known from the map since 0049, are listed only once come
  across: within 15 meters and in sight (a trace from the eyes), or seen
  on K and J's list. The counts on F1 and J are of those too.

### 0088 The tram's buttons on T

The tester: easier if the button were on the list like the tram. At the
station the button's trigger fell in the next area, so T did not list
it. A switch within 256 units across, 400 up or down, of a path corner
the level names "tram..." is the tram's button: named "Tram button",
and listed by T within 30 meters wherever the areas take it to be, come
across or not, as the tram is: "1 of 2, Tram button, switched off for
now, ahead, 2 meters", "2 of 2, Tram, walk in, ahead, 5 meters".

### 0089 The tram button stays; T made again when something new is found

The tester: the button was gone from T in the tram, and the weapons of
the soldiers taken down on the tram were not on T. A pressed switch was
taken as done and left off T, and the tram's button is pressed again and
again, each way: it is never taken as done now. T's list was kept for
30 seconds unless the area changed, so things come across after T was
pressed were left off it; a count of things come across now makes the
list again whenever it goes up.

### 0090 A dropped weapon is its own thing

The tester, in the tram: the button was not on T, nor the weapons of the
soldiers just taken down. Something seen within 48 units of a thing
already known was taken for that thing, so a weapon dropped by the
tram's button was taken for the button: it was never listed, and as a
dropped thing goes after 30 seconds, the button went with it. A dropped
weapon now matches only the thing with its own entity, and nothing
matches one that has been dropped but that very weapon, still there.

### 0091 Get in the tram first, then press its button

The tester found the button on the platform, pressed it, and the tram
left without them: it goes a moment after the button is pressed, and
the button is in reach from inside the tram. With the tram stopped by
its button and the player not in it, T says of the button "get in the
tram first and press it from inside: the tram leaves a moment after",
the tram's arrival says "Get in first, then press its button from
inside", and F at the button says so and presses nothing; F twice
quickly presses it from the platform all the same, as calling the tram
from there needs.

### 0092 Ladders on X; the tram's stations named

The tester, at the tram's middle station, had a ladder to climb up and
another down, and found neither on a list; nor could they tell where
they were from "area 4". X had a ladder only as a way not yet tried,
gone from the list once passed by. Now each ladder that joins two
floors (its foot and top, as the way-finder links them) is on X from
whichever end is in the area, or within 15 meters on the player's
floor, with what is new, nearest first: "1 of 15, ladder up to a new
area, behind left, 8 meters. O leads there, then F climbs it", "ladder
down, ahead, 1 meter. F climbs it". A way not yet tried that is one of
these ladders is not listed twice. X's list is made again after a climb
of a floor in the same area. The tram's stations - the two ends of its
way and a corner with a button by it - are named by height, and J and a
new area say "by the middle tram station" within about 20 meters.

### 0093 F goes to the thing chosen, not the ladder beside it

The tester: whatever was chosen on T, F did something else - by a
ladder, F climbed it. A ladder in reach and nearer than anything else
was climbed before F looked at what T, Y or O had chosen. Now a thing
chosen (other than a ladder) is turned to and used first when it is in
F's reach, and a ladder is not climbed while something else chosen is
within 4 meters. With nothing chosen, or a ladder chosen on X, F climbs
as before. At the tram's middle-station ladder, the glass chosen on T
four meters off: F turned to the glass.

### 0094 Rooms, corridors and halls

The tester, told only "Indoors" wherever they were inside, asked for
rooms and corridors said as they walk into them, as the tram is. The
place classifier (src/accessibility/navigation/place_classifier.c), made
for Enemy Territory's streets, now reads a place under a roof by how far
the floor runs each way through the player: long and narrow, a corridor
with the way it runs; wider than 10 meters both ways, a hall; else a
room, with its size: "Corridor, north to south", "Room, 4 by 4 meters",
"Small room, 3 by 3 meters". Rooms and halls are one place for the vote,
a corridor another; a room through a doorway of another size - by more
than 40 per cent and 2 meters - is said too. The map survey still sees
them all as a building, on an upper floor or not; the status no longer
says "Outdoors" before them.

### 0095 Walk tests: glass kept out of the way, doors said with F, places not repeated

Step 1 of the navigation plan: measuring before changing.
`accessibilityNavSweep [pairs] [open] [first]` searches the way from a
place in each area to a place in every other, as O does, and prints an
ETASWEEP line for each: found, partial or failed, the way's length
against the straight line, and the time taken. On Escape1, with every
door taken as open, all 600 ways were found, none over a quarter of a
second: the search is sound, and what goes wrong is on the walk. Walk
tests - a test player that follows only the speech, walking with W held
and stopping at a turn, a wall or a door - found, on the Tram level:

- The way went through breakable glass: the traces see only the map's
  own walls. The player was turned to the window 96 times. Now every
  solid thing to break in sight is in the way, as a door locked shut is;
  only when there is no way round does the way go through it, said "The
  way goes through something to break. Break it to get through: T lists
  it", at most every 20 seconds.
- Walking into a closed door said only "Door", thirty times, and the
  player stood there. The crosshair's door is now "Door, F opens it",
  and the same word again only after 5 seconds.
- Between two rooms of different sizes, "Room, 4 by 10 meters" was said
  thirty times. A place said in the last 20 seconds is not said again.
- "Small room, 1 by 8 meters" was a passage: a place under two meters
  across and at least twice as long is a corridor.

All nine walks reach their target now; one, from the upper station down
past the glass, still turns back and forth at an opening.

### 0096 Turns said in time; past a turn said; open doors not shut

Walk tests on the Tram level, nine ways of 20 to 73 meters, with a test
player that stops the moment it hears a turn. A turn was said 2 meters
before it - a quarter of a second at a run - and walking on past it the
way said "back", then the next turn the other way: round in circles.

- The turn is said half a second before the player reaches it at the
  speed they walk, and never later than 2 meters before.
- Walked on past it, still going the way they were - more than 2
  meters, less than 8 - "Past the turn, 2 meters back. O turns you to
  the way", once. Turned at the corner as told, nothing is said.
- A door standing open, or swinging, is not "A door: F opens it" when O
  turns the player to the way: F would shut it, and the test player
  shut one door in its own face again and again. The crosshair says
  "Open door" or "Door, moving" for them.

All nine ways are reached; the slowest of them, past the upper
station's glass, went from 192 seconds to 99.

### 0097 A hatch shut over a ladder; every switch of a door

The tester, at the tram's middle station: the power is down below, and
another button opens the way down. Under the station a turning door lies
flat over the last ladder down - a hatch - and two switches open it: one
under it, one by the foot of the ladder above. X said that ladder as "F
climbs it" with the hatch shut. Now a door shut across a ladder's top or
foot is said with it: "ladder down, ahead left, 2 meters, a hatch shut
over it: a switch opens it. O leads to the switch, then F presses it",
and the switch is chosen as a thing, so F at it presses it rather than
climbing the ladder beside it; a door's switch chosen by X is a thing
for F too. Only the first thing that opened a door was kept, and the
switch under the hatch was passed over as its own handle: now up to
four are kept, and of those standing apart from the door the nearest
the player is the one. Pressed, the crosshair says "Open door".

### 0098 Stairs that say where they go

The tester: stairs have no numbers, and which one leads where is not
known. X listed stairs only until they were tried, as "new stairs down",
and from the bottom of a flight walked down still said "stairs down".
Now stairs found stay on X once tried, as the ladders do, and each is
followed a step at a time from where it starts until the floor runs
level, or to a wall where it turns. From the end the player is nearer
it says which way it goes, how far, and what kind of place is at the
other end - "stairs up, 4 meters up, to a corridor, north-east to
south-west, right, 5 meters" - with "in area 3" or "in a new area" when
the far end is in another area: both ends of a flight are often in one
area, and the area alone said nothing. O leads to the near end. A place
under two meters each way - a landing, a turn of the stairs - is no
longer said as "Small room, 1 by 1 meters".

### 0099 The first station, the last, and where the level began

The tester: how to know the first building, the middle station, or the
last place to go. J named the middle station, but not which end the
others were, and nothing named where the level begins. The top station
of the line is now "by the upper tram station, the first", the bottom
"by the lower tram station, the last" - the level's way out is past it
- and within 12 meters of the level's start, on its floor, "where the
level began". "The way goes through something to break" was said as the
way through a breakable was looked for, and at the lower station no way
was found after it: it is said now only once that way has been found.

### 0100 Areas far too big are cut into places

The tester, at the middle station: "area 4 was the first building with
the big space; now I am in the middle of the station - another place".
The areas are the bots' rooms (their file's clusters), and a cluster is
cut only at doors the bots must open: on the Tram level one ran from the
first building's yard down the whole line past the lower station, 5,400
square meters. The middle station was the first building's area, and its
doors - a door inside one area joins no two - were on no list.
`ETA_BspAreas_SplitLarge` (src/accessibility/map/bsp_areas.c) now cuts a
room over 600 square meters, or longer than 60 meters, into pieces:
grown from the biggest floor not yet taken, out to 20 meters along the
way, never through a door, again until all of it is in a piece; each
piece is an area, with an opening where two touch. The Tram level has 73
areas, not 24; at the middle station, "New area, area 3, a large room,
halfway up the level, by the middle tram station. 11 ways out", and X
lists its doors, openings, ladders and the hatch. The areas a level
had before are numbered again where they were cut. `accessibilityNavSweep
areas` prints each area's box, floor and ways, `ladders` each ladder.

### 0101 F stays on the thing chosen; a ladder's foot off its face

The tester: F turns to the nearest thing instead of the one chosen, and
O the same. Anything to use within 3.5 meters in front was used before
the choice was looked at - added for the tram's button before the button
was on T - and what O led to was F's for only two minutes. Now the thing
in reach in front comes first only with nothing chosen, or the tram; and
the choice is F's for fifteen minutes, until it is done or another is
chosen.

Below the middle station O said "No way to tram button from here", with
two ladders between. A ladder's foot, where the way-finder steps on and
off it, was 16 units out from its face: the way down the platform's
ladder landed against it, where no step could be taken, and the search
ran out. It is 28 units out now; the way up from below the station is
found, and a test player climbed it to the platform in 35 seconds. The
other walks on the level were reached as before, the one past the upper
station's jump in 34 seconds rather than 180. `accessibilityNavSweep
route x y z x y z` prints one search, its points and their kinds.

### 0102 Out of the power room: ladders on the way, and the hatch from below

The tester: how to know the platform can be reached from below, to get
on the tram again. The map has the way - up through the hatch, up the
next ladder, across the floor above, down the platform's ladder - and
the way-finder finds it, but walking it a test player was stopped at
each step:

- At the foot of a ladder the way takes it heard only "Metal wall":
  "Ladder here: F climbs" came only after holding against it. Now it is
  said as soon as the player reaches the ladder the way takes - "Ladder
  here, the way goes up it: F climbs" - the ladder the way gets on at,
  not the one just climbed.
- F there turned the player to the ladder instead of climbing it. F now
  climbs the ladder the way takes first, from up to 3.5 meters off it,
  the climb walking the player onto it.
- Under the hatch the climb stopped with "Climb stopped", and F on a
  ladder only stopped the climb: nothing pressed the hatch's switch, on
  its underside. Now "A hatch is shut above you: F opens it", and F
  turns up to it and presses it; "Open door", and F climbs on.
- X below the hatch said "a switch opens it. O leads to the switch":
  the switch by the ladder above, out of reach. From below it says
  "ladder up, a hatch shut at its top: climb it, and F at the top opens
  the hatch".

The test player climbed from the power room through the hatch and up to
the floor above the platform in a minute.

### 0103 The edge guard: never over a fall that kills, every way, and steep slopes

The tester: "I am getting lost, falling down continuously." The edge
guard let go of W at a drop that would hurt - "Edge. Drop 20 meters. W
again goes over" - and W pressed again there, out of habit, went over,
whatever the drop. It guarded only walking forward, and nothing at all
on a slope too steep to stand on, where the player slides off with no
say in it: a test player went down a station roof a ledge at a time and
over its edge.

- A fall that would kill - by the game's own falling damage, or with no
  floor found below - is never gone over by pressing the key again:
  "Edge. Drop 20 meters, a fall that would kill you. Stopped". Jump,
  held, still goes over, for the way's own jumps.
- Backing, and side-stepping, are guarded as walking forward is: the
  way the player is going is taken from the keys held, or how they move,
  and every walk key is let go.
- A slope too steep to stand on just ahead, with a fall that hurts below
  it, stops the player as an edge does: "Edge. A steep slope you would
  slide off. Stopped".

### 0104 Stairs numbered; the end of a flight said

The tester: "I don't know which stairs I climbed up or down, and if I
have done it." Stairs are now numbered in the order found - kept with
the ways found, so the same each time the level is played - and X says
"stairs 2 up, used before, 4 meters up, to a corridor". At the end of a
flight walked - off the stairs a moment, on the ground, two meters or
more up or down from where it began - "Top of stairs 2, 4 meters up",
or "Bottom of stairs 1, 4 meters down", and those stairs are marked as
used.

### 0105 The tram's arrival said, with the station

The tester, riding the tram to the middle station: nothing was said
when it got there. Whether the tram was moving was taken from its way's
kind alone, and a way that has ended keeps its kind: the tram stood at
the station still "moving", and its arrival was never said. It is
moving now only while its way is under way. Stopped, inside it: "The
tram has stopped, by the middle tram station. J says where you are"; J
in it: "You are in the tram, standing, by the middle tram station".

### 0106 The tram's door

The tester: going to the tram, "I heard a metal wall instead; it didn't
tell me I was inside." T's "Tram, walk in" and O led to the tram's
middle, and the way-finder, which sees only the map's own walls, led
straight at its side. The tram's way in is now found from its shape:
from outside each side, a body's width at a time, tried into it at its
floor - two widths in a row that get through its wall are its door, the
one nearest the player kept until the tram moves. At the middle station
it is on the platform's side, a meter and a half east of the button. T
and O lead to it; there, "Tram door here. Walk in, ahead"; inside, "In
the tram", and J "You are in the tram, standing, by the middle tram
station". The edge guard's steep slopes are the map's own ground only:
inside the tram its slanted sides stopped the player as a slope.

### 0107 Areas named, not numbered

The tester: finish with area numbers, and use real places from the game
data. The levels name no places - no location entities, only a few
script names no player ever hears - so each area is named by what kind
of place it is (the place classifier, at where the player came into it,
or its middle) and the first of what marks it: the level's start or a
tram station, the way out, a ladder's foot or top, stairs, or how many
ways out it has. "New area: the corridor with stairs down", "Back in the
room with a ladder up", J "In the alley by the middle tram station,
halfway up the level, of 4 areas explored", X "The corridor with stairs
down. 2 ways out...", "door back to the hall with the way out", and
semicolon's list. Two areas of the same name keep their number after
it: "the corridor where the level began, 2".

### 0108 Climbing down a ladder under an edge

The tester: climbing, "it keeps climbing and doesn't stop", and they
did not know whether the top had been reached. On the middle station's
roof, F at the top of the ladder down to the room below backed the
player off the edge at a run: the ladder is under the roof's lip, and
the player dropped a meter past it, missed it, and fell eleven meters,
losing half their health. The climb now backs off such an edge at a
walk, and pushes toward the ladder the moment the feet leave the edge:
the game catches it, and the climb goes on down to "Bottom of the
ladder". "Top of the ladder" and "Bottom of the ladder" hold the other
callouts back a moment, so the next one does not talk over them.

### 0109 The climb says how far

The tester asked why a climb took so long. The game climbs about three
meters a second whoever is climbing, so the middle station's ladders, 9
to 12 meters, take 3 to 4 seconds; with nothing said in between, a climb
sounded endless. "Climbing up" and "Climbing down" now say the height
too, "Climbing up, 9 meters", from where the player stands to the
ladder's other end.

### 0110 Down through the hatch to the power room, and back up

The tester opened the hatch over the middle station's power room and
shot the soldier below through it, but could not get down. Four things
stood in the way, found by climbing it in the test game:

- The ladder is painted on both faces, and a clip brush covers its back:
  the game climbs it only from the shaft. Its foot and top were put on
  the back, behind the wall, so "climb down" walked away from the
  hatch. A ladder's ends now go on the face a trace from in front
  finds as a ladder, the face the game climbs.
- At the top with the hatch open, the crosshair was on the hatch, and F
  used it - swinging it shut - instead of climbing. Standing at a ladder
  with an open hatch at it, F climbs.
- At the foot, held on the ladder by the game, F said "Opening the
  hatch" five meters below it. Only within reach of the hatch now.
- Climbing up to the shut hatch, nothing was said for twenty seconds.
  Held there a second, "A hatch is shut above you: F opens it". F opens
  it while still facing the ladder - turned to the switch, a little
  behind, the player had let go and dropped - and F again climbs on,
  rather than "Just used".

The test player then climbed up and down through the hatch three times,
about two seconds each way, without a fall.

### 0111 The hatch switch on T, named for the hatch

The tester asked why the hatch switch and the power switch were not on
T. The hatch's switch is set in the wall of the room above the power
room, a meter from where the tester stood; its middle, in the wall,
was taken for another area's, and T lists only the area's things. A
thing within ten meters and in sight is now on T wherever the map puts
it. A switch on the far side of a shut door or hatch is not: the one
under the hatch had been listed from the room above, where F could not
reach it. A switch for a door lying flat is "Switch for the hatch".

The power switch is on T once seen, as everything is: from the power
room, or from above through the open hatch.

### 0112 A door's switch comes back on T while its door is shut

The tester: the hatch switch disappeared from T, even after loading a
save with F9. A switch was done for good once pressed, and what is done
is kept for the level, through loads. The hatch is a toggle - it stays
open until the switch is pressed again - and loading a save from before
had it shut, with its switch still gone. A switch that works a door is
now on T while its door is shut and off it while the door is open, as
the game has the door now: tested pressing it, then loading the save
made before, and T had it again.

### 0113 Down through the open hatch

The tester opened the hatch with its switch and still could not get
down to the power room, for the soldier's weapon or the power switch.
Followed as they would, with X, O and F:

- F, once the hatch was open, pressed its switch again and shut it. A
  switch whose door is open now is left out of what F turns to, and
  facing it F goes on to what else is near; F twice quickly still
  presses it.
- X listed the open hatch as "door to a new area", and O stopped the
  player beside it. A hatch over a ladder is no way of its own now -
  the ladder is - and X's ladder says the hatch only while it is shut.
- At the hatch, "Ladder here, the way goes down it: F climbs", and F
  climbed the ladder up to the roof, a meter nearer, twelve meters up.
  F now takes the ladder the way goes by first.
- The hatch opens by standing up on its hinge, a metal wall over the
  ladder's head, where its top had been put: the climb walked into it.
  A ladder's top on a hatch moves across the hatchway, to the side the
  ladder is climbed from: the player steps off that edge facing the
  ladder, and the game catches it.

The test player, from the switch, was led down to the power switch in
two seconds of climbing.

### 0114 From the last tram station to the end: the ladder, the jump, the crawl

The tester, at the last tram station, remembered having to jump over
something and crawl into a hole in the wall, and the guide knew no way
on. The map's own navigation data has it: a ladder up from the corridor
beside the station to a balcony; at the balcony's south end a two meter
gap, jumped across westward into an upper room; on the left at once a
low opening behind a metal fence to break, crouched into; a three meter
drop into an air duct, crawled south; a five meter drop to the floor,
and a low tunnel crawled west, behind a second fence, into the rooms
before the end. The way now finds all of it:

- Gaps are jumped: where the next square has no floor, or only one far
  below, a square two to four further on at about the same height is
  tried for a running jump, with room over the top of the jump
  (navigation_system.c, ETA_PathFinder_SetLeap; ETA_RouteLeap).
- Drops as deep as do no harm - the game hurts a landing past about six
  meters - are walked off, costing as much as a jump, so only when there
  is no other way.
- A ladder's top is taken only where it is reached from the ladder's
  head with no wall between: the balcony ladder's had been put in a
  room behind its wall, behind a door only the soldiers open. Then
  nearer the ladder, for a ledge no wider than its top; and each end is
  moved clear of the walls by the player's own width.
- F climbs the ladder found by touch when it is the way's own, and the
  way's when it is not; a climb found from the map first walks to where
  the map says to get on, then turns to the ladder; and brushed against
  from the side, the climb no longer stops at once with "Off the
  ladder".

The fences are things to break: the way says "The way goes through
something to break. Break it to get through: T lists it".

### 0115 A meal eaten to the plate is not offered

The tester, at 90 health in the village, could not eat the food on a
table, and was told "You carry all you can of this". A hot meal is eaten
in two goes, as much as the player needs each time, and then leaves its
plate where it was: the game keeps the plate as the same item, marked
used up, and never lets it be taken. F turned to the plate and said the
player was full. Used-up meals and drinks are now left out of F, T and
what is said round about. Tested: at 90 health a whole meal is eaten,
to 100; eaten to the plate, F and T go on to what else there is.

### 0116 O goes on when the way out is shut; places named by what is in them

The tester, in the village: O was to lead the way on, but led nowhere,
and they had to use X all the time. The level's way out is shut until
the story opens it, and O said "No way to the way out from here yet".
Now, when the way out cannot be reached, O says "The way out is not
open yet. On to the nearest place not explored" and leads to X's first
entry; O again goes on to the next.

They also asked for every part of a level to be named, as in the Enemy
Territory mode, and where the game names nothing, by how it looks. Areas
are now named by the map's own furniture in them, read from its
surfaces: beds a bedroom, bunks a bunk room, a stove or sink a kitchen,
pews a church, tombs or bones a crypt, wine racks a wine cellar, hay a
barn, a generator a generator room, a desk, radio or telephone an
office, books a study, armchairs or a fireplace a sitting room, tables
and chairs a dining room, crates and barrels a store room. Outdoors the
ground is said - "the snowy square", "the grassy alley" - and a truck,
a cart, hay, sandbags or trees mark the place. A plain room under the
roof is the attic, one above the ground floor upstairs. In the village:
two wine cellars, a crypt, a barn, a study, a generator room, a bedroom.

### 0117 What is explored follows the saves

The tester asked for explored places to be remembered from a save, not
kept on through a death. What is explored - the areas come into and the
ways found - is now copied when the game says it is saved, and loading a
save gives back that copy: what was explored after the save is
forgotten, as the items taken after it are back. Loading a save of the
same level restarts it in place, without the cgame starting again, so
the map restart takes the copy back too. A level entered fresh, not from
a save, is explored anew. Tested: explored two areas, saved, explored a
third, loaded: two.

### 0118 Things not found yet: how many, and O into their rooms

The tester asked how to find the items missed along the way, when they
could not get into some places properly. T lists only what has been come
across, so nothing is given away. Now how many of the map's items and
treasures have not been come across is said too - never what or where:
"3 things left here: 2 items. ... 3 things left in the level, and 28
not found yet". And O, with the way out not open yet, leads first into
the nearest room that still has things not come across - to the room's
middle, not to the things: "The way out is not open yet. On to the
corridor with 6 ways out: 2 things not found yet there". Once in the
room the things come into sight, and T has them. With none left, O goes
on to the nearest place not explored, as before. Secret areas, things
that come later in the level and things shut inside something to break
are not counted.

### 0119 O no longer turns you round in circles

The tester: O rolled them round in circles instead of leading through the
openings. In the village a test player led by O alone went round and round
at the switchback stairs on the way to the wine cellar: with the way's
next corner a step away, O faced the corner and said "1 meter, then back";
turned there, the next O said it again. With the corner closer than about
a meter and a half, O now faces on past it, and says no turn. And O's lead
into rooms with things not yet found goes only to rooms not yet been in: a
room been in whose thing stays out of sight - up high, behind something -
was led to again and again. An area's name is now the same each time it is
said, from the first time: worked out from another spot it had come out
otherwise - "the corridor" while O led there, "the wine cellar" on
arrival. A test player led by O alone then went from the start to the
wine cellar and on into new rooms.

### 0120 Marker forgotten when a save is loaded

The tester: after a death, a marker set on a place that could not be
reached was still there once the save was loaded, and had to be removed by
hand. A save of the same level is loaded by restarting the level in place,
without starting the cgame again, so the marker lived on. Now when the
level restarts the marker and the way to it are forgotten, without a word.

### 0121 The game starts after a crash or a force quit

The tester: after the game froze and was force quit, it would no longer
open at all. The game leaves a file, iowolfsp.pid, while it runs; found at
the next start, it asks in a window of its own whether to start with safe
video settings, and waits for the answer. On the Mac that window can open
behind everything, where a screen reader player does not find it, and the
game seems not to start. The question is no longer asked: the game notes
it in the console and starts as usual.

### 0122 O leads through the openings first again

The tester: O no longer led through the openings as it had, but to a
room with things not found. With the way out shut, O now goes on to the
nearest opening not explored, as X's first, while there is one anywhere;
only when everything has been explored does it lead to the rooms with
things not yet found. And a test player led by O alone in the village
went round in circles three ways, now gone:

- An opening to a new area was "reached" three metres short, still on
  this side, and O led to it again and again. A way into a new area is
  now reached on coming into that area.
- Facing on past a turn a step away cut into a doorway's frame. It now
  faces past the turn only where the straight line round it is clear.
- The way went down through a hole in a floor, three metres, no harm,
  but facing never turns a player off an edge, so O turned them to the
  openings either side. Where the way drops more than two metres just
  ahead, O now faces it and says "It drops 3 meters here, no harm: walk
  on off the edge".

The test player then went from the start through the rooms and the
cellars, down through the hole and on, the village's first objective
done on the way. The tram level's walks still pass.

### 0123 No drop loops

The tester, going for a rifle on a walkway in the village, kept climbing
the same ladder. From the top, the way dropped four metres back into the
street, and from the street went up the ladder again. A drop deeper than
a storey is now taken only when there is no way on foot, as a jump is.
And "It drops here" is no longer said on a ladder, or with the way going
down one.

### 0124 Switches through relays; O on past ways with no way

In the first catacomb level nearly every switch opens its door through a
relay, and the switch was found for one door in ten: X said "opened by a
switch or by what happens", and O could not lead to it. Relays are now
followed, three deep, and all ten doors have their switch. With the way
out shut, an opening O finds no way to is said once, "On to the next",
and put last on X's list; from the same spot, after one round, O stops
and says a jump, a switch or something to break may be what is needed.
Before, it said "No way" a hundred and seventy times. And O can no longer
call itself round without end.

### 0125 Openings on the floor; a start out of the corner

In the second catacomb level an opening's spot was worked out at the
height of its top, four metres up, and no way was ever found to it: it
is now put on the floor beyond. And a player squeezed into a corner,
their square in the walls on two sides, had no way at all from it: a
clear square is now looked for up to two squares away. A test player led
by O alone went through eight rooms of the level.

### 0126 The way keeps to the middle of narrow walkways

The way to the rifle runs along the top of a wall 1.8 metres wide with a
five metre drop beside it, and the way ran along the very edge: the test
player stepped off again and again. A square with a drop within half a
square of its middle now costs three squares more, so the way keeps to
the middle. From the tester's save the test player then reached the
rifle. The tram level's walks still pass.

### 0127 O faces the player across a jump

The tester jumped, as a sighted player, by facing the far side and
holding forward with jump. With a jump just ahead on the way, O now
faces the player straight across it, to where it lands, and says
"Facing across the jump over, 2 meters. Hold W and press space". The
callout coming up to it says the same: "Jump over, ahead. O faces you
across; then hold w and press space". Not at a closed door the way goes
through, which in the first catacomb level was taken for a jump.

### 0128 No turning back after a turn by hand; menu tabs said

The tester wanted no time after which they were turned to something.
The lock by itself turned the view back onto an enemy two seconds after
a turn by hand. Now a turn by hand lets go of that enemy - the one the
lock was on, or the nearest in sight - for as long as it stays in sight;
another enemy can still be locked on to, and U still turns to any.

And the menus: the Controls page shows one tab's settings at a time -
Look, Move, Shoot, Misc - and the tester, reading down it, heard Look's
settings and the other tabs' names, and took that for the whole menu. A
button that shows a set of the menu's items in place of another is now
said as a tab: "Move, tab: Enter shows its settings", or "Look, tab,
open: its settings are below"; and Enter on it says "Move open. Down
reads its settings".

### 0129 The jump on the way is made at the edge

The tester, faced across the spike pit, held W and jumped, and fell in:
the edge guard let go of W three metres short of the lip, and the jump
was made from standing. With the way's next step a jump, and the player
going toward where it lands with W held, the edge guard now leaves them
be, and jump is pressed for them at the right moment: at the lip of a
gap, or at the square before something to jump up onto or over. A jump
already held is let go a frame first, as the game will not jump again
while it is held. O and the callout now say "Hold W: the jump is made
for you at the edge". A door the way's squares took for something to
jump over is no jump: it was faced, and said, again and again.

### 0130 The jump is the player's; O's facing holds

The tester did not want the jump made for them: they jump with space,
held with W, as before. 0129's pressing of jump is taken out; the edge
guard still leaves them be while they go toward the way's jump with W,
and O says "Hold W and press space" again.

What they meant by a time limit was O's facing: turned across the spike
pit by O, they were not kept facing it. Turned to something by O or F,
no enemy's lock took the view before six seconds were up, and then one
could. Now the facing holds until they turn by hand, press U, or walk
four metres from where they were turned. Shooting, U, and the lock
taking an enemy at any other time are as they were.

### 0131 A sound says when to jump

A sighted player jumps a gap by running at it and pressing jump at the
edge; the tester, who played that way, needed only to know when. Running
at a jump on the way - facing where it lands, W held - a short sound,
two quick rising high notes (jump_now.wav, made with the other cues by
release/make-cue-sounds.py), plays once where a sighted player would
press jump: at the lip of a gap, or at the square before something to
jump onto or over, early by the time it takes to hear it and press.
The jump is the player's own. O and the callout say "Run at it with W,
and press space at the sound". It plays at the bump sounds' volume.

### 0132 Ticks say how near an edge is; stairs are walked

The tester heard how deep a ledge was, never how near, and so could not
tell when to jump the spike pit; the jump sound alone did not help.
Walking toward a drop with W held, a soft low tick (edge_tick.wav) now
plays again and again, quicker the nearer the edge: from about three
metres out, every 0.4 seconds, to every 0.07 at the lip - as a car's
parking sensor. It plays at the bump sounds' volume.

And the tester was told to jump up stairs it was simpler to walk up: the
way looks a square at a time, and two risers in one square were taken
for a ledge. A step between, at the square's middle, no more than a
step above the one and below the other, now makes them stairs.

### 0133 The edge said in metres

The tester wanted to be told in metres how near the edge was, and then,
very near, how deep: not a ticking sound, nor a sound to jump at.
Walking toward a drop deeper than a step or two with W held, "Edge in 3
meters", then "2 meters", then "1 meter" is said, each once as it comes
nearer; the drop warning, "Ledge ahead, 4 metres down", says how deep
when very near. The ticks and the jump sound of 0131 and 0132 are taken
out, and their sounds from the pk3. O says "Run at it with W; the edge
is said in meters".

### 0134 Only "1 meter", where it is close enough to jump

The tester wanted one message where they are close enough to jump, not
a count down from three: "1 meter" is now said once, walking toward a
drop with W held; the drop warning says how deep. O says "Run at it with
W, and press space at 1 meter".

### 0135 The lock on enemies as it was

The tester found the lock on enemies broken, and asked for it as it was.
0128's letting go of an enemy turned from by hand, and 0130's holding of
O's facing against the lock until a hand turn or a walk away, are taken
out: a turn by hand keeps the lock off two seconds, and a thing turned
to keeps it off six, as before.

### 0136 T lists every pack; O to a locked door's switch; jumps only over gaps

In the catacombs the tester found health and ammunition that T did not
have. Things within half a meter of each other were taken for one: a
weapon by its clips, two packs side by side, listed as one. An item is
now the same as another only by its entity, or, for one the map put in
and not yet seen, by its name and where it is. And an item T took for
gone - its place in view, the game not sending it - is listed again as
soon as it is seen: the view was looked through doors, so packs behind
a shut door were taken for gone. The view for that now stops at doors.

O, facing a locked door whose switch is elsewhere, says how far and
where the switch is, and O again leads to it; F presses it there.

The way jumped wherever the next square was not walkable: over candle
posts, rubble and a door's sill under the last door of the first
catacomb level, and the tester did not know which jumps were needed. A
jump across is now taken only where there is a gap, somewhere between
with no floor within a step or two below, and never through a locked
door. The jump over the spike pit stays.

### 0137 A pressed switch stays on T

The tester pressed a door's switch, lost their way, and could not find
the switch on T again: once pressed, or once its door was open, it was
taken off the list for good. A switch pressed now stays on T, after the
things still to do, said "pressed already", or "pressed already, its
door is open", and "F presses it again" near it. It is not counted in
the things left. Its door shut again, it is a switch to press as before.

### 0138 O to the switch that opens the way out

In the first catacomb level the tester found the journal and was told
to find the exit; O said the way out was not open yet and sent them
about the places not explored. The last door is shut by a trap: walking
into the room before it shuts its doors, and a switch set in the room's
wall opens them again, and the last door with them.

O, finding the way out shut behind a door only a switch opens, now says
"The way out is behind a locked door. Its switch is so many meters away"
and O again leads to that switch: of such doors the one nearest the way
out, not one whose switch has been pressed already.

A switch set in a wall is led to on the side it is pressed from: O had
led to the corridor behind the room's wall, where F did nothing. And a
door out of sight is taken as last seen, or open if the map starts it
open: the way had taken the trap room's open doors for shut, and found
no way into it.

### 0139 The way out at the start of a level

A check of all 26 levels, O pressed for the way out where each begins:

0138 led to a switch near the way out wherever the way out was shut, and
at the start of the first level that was a switch 128 meters off, at the
far end of a level not yet explored; five levels began so. A switch is
now led to only once it has been come across, as T has it; until then O
goes on to the places not explored, as before.

On three big levels the search for the way gave up before it knew the
way out was shut, and O said "No way found from here yet. O tries
again" each time it was pressed, and never went on. The way out not
found in time is now taken as not open yet: on to the places not
explored.

### 0140 Starts closed in: a grate to break, a truck's tailgate

The base exit level begins in an air duct with a metal grate across it,
and O found no way and went on to rooms it could not reach. Where there
is nowhere new to go and something to break is close by and in sight,
O now says "No way on from here but through something to break", how
far and where, and O again leads to it.

The rocket level begins in the back of a truck, its tailgate opened by
an invisible trigger reaching under the truck's bed. F looked for its
middle, below the floor, and said "Nothing to use within 15 meters". A
trigger whose middle is out of sight is now looked for at its part
nearest the eye, and F finds it: "Door, ahead, 4 meters". Getting near
enough to use it is not yet worked out.

### 0141 Shut in by the trap room

The trap room of the first catacomb level shuts its doors, and the last
door with them, when walked into. The tester, past the zombies, came to
the last door shut and heard no way round: O took it for a door F opens
- the game says so of it - and F at it did nothing; and O did not offer
the switch, not yet come across.

A door the map gives the locked crosshair is now taken as locked: F
does not open it, only what it is wired to. Only this level's four trap
doors have it. And the switch that opens the way out is offered when it
is within 30 meters, come across or not: from the shut last door, "The
way out is behind a locked door. Its switch is 22 meters away", and O
again leads to the room's switch.

### 0142 Shut in a room by a door a switch close by opens

The tester found the first catacomb level's secret room, its door shut
behind them, and O said everything was explored; the button that opens
the door from inside is in the room, two meters off. When the way out
is not open, O now looks first for a door close by, shut, that a
switch close by opens - within 12 meters, its switch within 16 - and
says "Shut in here: the door out opens with a switch", how far and
where; O again leads to it.

And F, at a switch O led to where it is pressed from, turned the player
to that spot on the floor instead of to the switch: the place F turns
to is now the switch itself, the place O leads to where it is pressed
from.

### 0143 The mode's keys in the Accessibility pages

The tester wanted everything the mode adds in its own Accessibility
section of the settings, the game's own pages left as the game has them;
the mode's keys were nowhere in the menus. Three pages are added, KEYS:
GETTING AROUND, KEYS: THINGS AND ENEMIES and KEYS: WHAT YOU ARE TOLD,
each row its key as the game's Controls page has its own: "Health, H".
Enter, then a key, puts it on that key ("bound to H or Q"); Backspace
clears it; Escape leaves it as it was. The mode's commands are added to
the menus' list of commands (ui_shared.c), with the mode's own keys as
their defaults, and the pages to the Accessibility pages Escape steps
back through (ui_main.c).

### 0144 All explored: the switch of a locked door come to

In the second catacomb level the tester had explored every area; O and
X said there was nothing new, and the door on was shut, its switch 44
meters off and seven meters up. When the way out is not open and
nothing is left to explore, O now looks for a door come to and seen
shut that a switch opens, the nearest the player, and says "Everything
open is explored. The way on is a locked door 14 meters from here; its
switch is 44 meters away", and O again leads to where the switch is
pressed from - before the rooms with things not found.

### 0145 The lock on a slower computer, and a U lock that keeps the aim

The tester heard "Locked" and their shots went wide. The lock turns
with the game's own turn keys, held a frame at a time, and fast while
far off. Below about 47 frames a second a fast frame turned further
than the lock let go at, so the keys stopped while it stayed fast: it
stood 12 to 23 degrees off the enemy. It is fast now only while a whole
fast frame falls short of the point. And the last degrees are a nudge
measured to the degree: a press and a release a few milliseconds apart,
which the engine counts as exactly that much turn (IN_KeyUp). At 25, 30
and 60 frames a second the lock now settles within a tenth of a degree.

And the lock the player presses U for turns them as a sighted player
already looking at the enemy with the mouse: while it is on, the game
is told (+wbutton6, WBUTTON_STEADYAIM), and its turn does not widen the
aim's spread (bg_pmove.c). The tester is attacked the moment enemies
come, and cannot wait for the aim to settle. The lock by itself, and
the player's own turns, count as the game has them.

### 0146 The lock leads a moving enemy

The tester still missed at ten metres. The lock aimed where the game
last showed the enemy, a snapshot and more behind where the server has
them when the shot is fired: measured on the server, the view was a
third of a metre behind a zombie coming on, eight metres off. The lock
now aims a tenth of a second ahead along the enemy's movement, as a
sighted player leads a running man: measured again, 0.4 units off at
ten and a half metres, 2.4 at fourteen, and the shots hit.

### 0147 Traps: the saws, the swinging blade, and the pressure plates

The second catacomb level has a room of three saws, set going by
pressure plates in the floor, and a corridor with a swinging blade. The
tester asked which saw is which, and when it is safe. T listed the
plates as switches ("F uses it"), and O walked over them.

A plate - a button the map sets off at a touch (func_button's TOUCH,
spawnflag 8) - is no longer a switch: it is off T. Coming toward one,
the nearest is said with the wall that clears it: "Pressure plate ahead
right: it sets off a trap. Keep to the north wall, on your left". The
wall is found once, from the map: the side of the narrow way whose wall
a body walking against it clears the plate by. In the saw room that is
the north wall for the first two plates and the south wall for the
last. O's way goes round the plates too, between them where a body
fits, its steps moved a little near a plate to land in the narrow
strips beside them.

A trap - a script mover the map names a saw, blade, spike, axe,
crusher or pendulum - is grouped with the others of its kind near it.
Coming near: "Trap ahead: 3 saws, at rest. Saw 1, right wall, ahead
right, 6 meters. Saw 2, ceiling, ahead, 7 meters. Saw 3, left wall,
ahead left, 10 meters", the numbers kept from then on. The three saws
are not one after another: a plate sets all three going at once across
the same stretch, from either wall and the ceiling, so each is told by
where it comes from. Then, while near: "Saws moving", and when they are
back, "Saws stopped, safe to cross". The blade: "Blade swinging"; after
its swing it hangs low across the corridor, "Blade stopped in the way:
wait"; and once drawn back up, "Blade back up, safe to cross" - a blade
drawn back slowly does not cut, so only a fast move counts as moving.
Crossed on that word, without god mode, the test player came through
whole.

### 0148 The way goes round furniture, and points only where a body fits

One of the test walks on the tram level failed every time, and had been
put down to bad luck. It was not: the walk began inside a chair. A chair
is no part of the map's walls - the game sends it as a solid box of its
own - so the way ran straight through it, and from there every step was
blocked while O kept saying "Facing the way".

The way now goes round anything solid the game sends with a box of its
own and that stands still - chairs and the like, not people - by its
outline and a body's half width, as it goes round a pressure plate; a
step that starts inside such an outline may leave it. And the point O
turns the player toward, the furthest one along the way in a straight
line, must have room for the player's full width and standing height
all the way: the probe that finds the way is narrower than a player, so
that doorways are found, but the player walks the line with their body.

The test walk now starts beside the chair, not in it, and the other
walk thought unlucky - it began half inside a wall and a step - starts
clear of them. And the walk test now tries a step each way from its
start first, and says "START STUCK" at once when the player cannot move
at all, so a bad start is never again taken for bad luck.

### 0149 Walls that close on the player

In the second catacomb level the button at the east end of the
crossroads sets two walls sliding in from either side, over the place
the player stands to press it. The tester pressed it and was crushed;
nothing had said so. Tested: pressed and stood still, the player dies;
running straight back within about 0.8 seconds of pressing, they live;
at 1.2 seconds they do not.

What a switch sets going is in the level's own scripts, so it is read
from them: the switch fires a script trigger; the player's AI script
(maps/<level>.ai) answers it by setting script movers going; each
mover's script (maps/<level>.script) sends it to path corners. A mover
as tall as a body, that comes from somewhere clear of the place the
switch is pressed from and ends over it, makes the switch a crusher.
Across all the levels, that is this one button; its own moving part,
the other two buttons' parts, and a table tipped over in another level
are not. T and F say it before it is pressed: "Button: a trap, walls
close on you. Press, then run back".

And the moment any mover - not a door, which opens again on whoever is
in its way, not what the player rides, not the saws and blade - starts
a move that ends over the player, the shortest way out of every closing
mover's path is said, the way first: "Run behind, 5 meters! Walls
closing". Eight ways round are tried, each to a place clear of the
movers' ends, with nothing in the way and floor under it. For two and
a half seconds nothing else is said over it (ETA_Speech_HoldForUrgent):
on the Mac each line cuts off the one before, and a "Done" that came
next had cut it off.

The same level's three hidden buttons open two hidden doors upstairs
only when pressed south, east, west. That order is in the scripts too:
each button's event sets off one event of a counting entity, which goes
on only when its count stands at that button's place ("accum 0
abort_if_not_equal 1", then "accum 0 inc 1"); the pillar's count, which
every button only adds to, gives each the same place and is no order.
T and F give each its place, "Button 2 of 3", and coming up to one says
it all once: "This button is 2 of 3, pressed in order: first the south
one, then this one, the east one, last the west one. In that order they
open hidden doors. Walls close on you here when it is pressed: press
it, then run straight back at once." The ways are from the middle of
the three; "hidden doors" when what the last sets off is, through
relays, doors.

### 0150 The lift, its bridge, and a door only a switch opens

The second catacomb level's last puzzle: once the three buttons are
pressed, the stone pillar in the middle of the crossroads sinks level
with the floor; stood on, it lifts the player to the floor above, and a
bridge slides out slowly to the north. Across it a switch opens the
door on the south side of the upper floor, reached round a walkway at
the side of the shaft (a jump from the pillar across the gap also
lands there; tested). Nothing said any of it, and at that door the
crosshair said "Door, F opens it": F does nothing at a door switches are
aimed at (the game locks it to the player, KEY_LOCKED_TARGET).

Read from the scripts as the crushers are: a trigger for the player
whose event sends a script mover up well past it is a lift; a door its
event sets off, through relays, is its bridge. Said, tested in the
level: "A lift has come down level with the floor, ahead, 6 meters:
stand on it, and it lifts you up. O leads there" (O is set to it);
"Going up. A bridge comes out as you rise: wait for it"; "Bridge
sliding out, ahead: wait for it"; "At the top" (once it has stood still
a second: its moves run on one from another); "Bridge out, ahead: cross
now"; "Bridge pulling back". The door says "Door: a switch opens it,
not F". And a trap on another floor - the blade below the corridor
upstairs - is not announced.

### 0151 Pressing the buttons in order, with O leading

The tester pressed the crypt's three buttons out of order, found that a
pressed button did nothing pressed again, and did not know how to start
over; "2 of 3" did not say which side each was. Each switch of an order
now has its side in its name ("Button 2 of 3, east"), from the middle
of the three, and its own moving part - the mover its event sends a
little way, read from the scripts - shows whether it is pressed,
whenever the server sends it, a loaded game included.

After each press: "1 of 3 pressed. Next, the east one, 33 meters. O
leads there", and O is set to it. Out of order: "Out of order: the
hidden doors won't open this time. It still counts: 1 of 3 pressed.
Press the rest, and the pillar still comes down. To try the order
again, ride the pillar once: the buttons reset when it comes back
down." The last: "All 3 pressed, in order" (or "but not in order: the
hidden doors stay shut this time"). Coming to one that is not next:
"Button 2 of 3, east. Not this one yet: first the south one, 32
meters. O leads there"; to one pressed already: "...pressed already: it
counts once. 1 of 3 pressed. Next, ...". When the lift is back where it
stood, or a part is seen back: "The buttons have reset: press all 3
again, in order. First the south one, 25 meters. O leads there"; parts
sliding back after that are not taken for presses. A message due while
the crushing walls' warning is heard out waits for it to end. Tested
from the tester's own saved game, in order, out of order, through a
ride and a reset.

### 0152 T lists a puzzle's buttons together

T lists what is left in the area the player is in, so in the crypt's
east room it gave one button of the three: "1 of 1, Button 2 of 3". The
tester asked for the puzzle's buttons together. When any switch of an
order is in the area or in sight, T now lists every switch of that
order, first and in its order, wherever each one is - only the buttons: the
tester did not want it called a puzzle. Each says how it stands, from the
order's own record, which knows of resets: "next to press", "not yet:
the south one first", or "pressed already" - and close by, "Pressing it
again does nothing: it counts once", or for one out of turn, "Pressed
now, it counts, but out of order". Away from the puzzle, T is as before.

### 0153 Where a trap kills: stop before it, out of it, and how far across

The tester was killed by the crypt's blade going for the button past
it. The blade kills along six metres of its corridor, at a blow, while
it swings; the mod said when it swung, not where that began. A trap's
reach is now the map's hurt triggers near its movers, in one box. While
the trap is not at rest: walking toward that box, within four metres,
"Stop! The blade's reach starts 4 meters ahead"; inside it, at once and
heard out like the walls' warning, the shortest way out, of eight ways
round, with nothing in the way: "Out! Run behind, 2 meters". Near it,
the swing is said with what to do, "Blade swinging: wait here, cross
when it is back up", and the safe moment with how far to run, "Blade
back up: run across now, 11 meters". Tested from the tester's own
saved game, without god mode, doing what is said: stopped at the first
word, crossed on "back up", pressed the button, and came back across
the same way, unhurt.

And O, leading to the south button, said "Marker reached" three metres
from it - its usual arrival - which is just inside the blade's reach:
the tester stopped there, pressed, and the next swing killed them. O
no longer arrives while the player stands where a trap kills; it leads
on, and arrived at the button two and a half metres from the wall,
clear of the reach. Stopped inside the reach while the trap is at rest
- its first blow comes in a fraction of a second when it next starts,
too soon for "Out!" - is said too: "Don't stop here: you're in the
blade's reach. Ahead, 2 meters, out of it".

### 0154 Clear of the blade at the button, and T's "F uses it" only in reach

The tester was still killed at the crypt's south button. T said "F uses
it" at four metres - F reaches two and a half - and four metres from
that button is inside the blade's reach; and the blade's own body,
swinging, kills what it touches (a blocked script mover kills,
script_mover_blocked), a little past its hurt triggers. T now says "Walk
2 meters closer, then F uses it" until F reaches; and the reach is taken
a metre wider for staying out of it - O's arrival, "Don't stop here",
the way out, and how far to run across. Tested from the tester's save:
O said "Marker reached" half a metre from the button; pressed there,
the player stood through the blade's swings unhurt.

### 0155 Through the blade without stopping

The tester still died at the crypt's south button. Played from their
own save with enemies on, following O, the test player stopped in the
blade's reach each time O turned it to the way ("Facing the way ... 4
meters"); the safe moment ran out, and "Out! Run behind, 3 meters" sent
it back into the blade with the button as near ahead. And standing at
the button, "Stop! The blade's reach starts 1 meter ahead" was said
over and over - the reach was behind.

Now: O, pressed inside a trap's reach, does not stop to face the way:
"Keep running straight ahead, 3 meters, out of the blade's reach". The
way out takes the way faced, then a little either side, and the way
O leads, over a way back no more than a metre shorter. "Stop!" is said
only walking toward the reach, with where it is: "Stop! The blade's
reach is ahead, 4 meters". The safe moment says how: "Blade back up:
run across now, without stopping, 11 meters". The extra metre for the
blade's body is for swinging blades only: at the west button "Don't
stop here: you're in the saws' reach" was said, and the saws do not
reach there. Played twice from the tester's save, enemies on, doing as
said - W held through on "without stopping": the button reached and
pressed, unhurt.

### 0156 Cross for me

The tester still could not get past the crypt's blade to the south
button: judging its safe moment and running dead straight through take
eyes. They asked for a key that does that much, the game's design kept:
the blade, its timing and its danger as they are. Slash (free in the
stock config, where every letter is taken; on the Keys: Getting around
page as "Cross a trap for me"), near a trap whose reach is known - the
blade, the saws - says "Crossing. Waiting for the blade: stay still. Any
move of yours stops it"; waits for the trap to come back to rest from a
move, so the whole safe moment is the run's; says "Now", turns the
player along the narrow way through the reach and runs them straight
across - to what O leads to, if it lies just beyond, a body's width
short, else past the far side - and stops: "Across. Button 1 of 3,
south, ahead: F uses it". F is still the player's. A move of their own
(back, aside, a jump, or forward while waiting) stops it: "Crossing
stopped"; so does being held up. Tested from the tester's own save,
without god mode: across, the button pressed with F, and back across,
unhurt. And a switch of an order pressed already says so when F or T
names it: "Button 3 of 3, west, pressed already".

### 0157 Buttons heard out; zombie warriors warned of

In the crypt's blade corridor the tester heard no buttons on T. They
were on the list, but the first line began "No things left in this
area", and the blade's "swinging" and "back up", every few seconds, cut
the rest off on the Mac before a button was named. Now "No things left"
is not said when the list goes on; and while T's answer is heard, a
trap's coming and going waits - unless the player is in its reach or
walking into it - and is said once the answer is over, as it stands
then; what the cross key goes by is not held back. The puzzle's buttons
are listed too anywhere near them on their floor, once come across,
while any is still to press: past the crossroads' north door T said
"Everything found has been taken or used". And a zombie warrior - the
crypt's come out at the crossroads, and swing blades - is said once,
the first time one is within 25 metres on the player's floor, seen or
not: "Zombie warrior with a blade, ahead left, 14 meters. U locks on to
it". Tested from the tester's save: all three buttons heard in the
corridor, the blade's line after them.

### 0158 The crypt to its end

The rest of the crypt walked from the tester's save, and what was found
on the way. The script brings the pillar down after any three presses,
in any order; the order - south, east, west - only opens the hidden
doors. The buttons' lines said otherwise: "press all 3 again, in order",
"not yet: the south one first". They now say any order brings the
pillar, and the order is for the hidden doors. After every ride the
bridge pulls back and the pillar sinks, resetting the buttons - the
tester found them reset and did not know why. The bridge now says how
long it stays, read from the level's script: "Bridge out, ahead: cross
now. It pulls back in 9 seconds"; still on the lift as it goes, "You're
still on the lift: it goes back down"; across, "Behind you, the pillar
has sunk back, and its buttons have reset: you don't need them again",
and T no longer lists them once you have stood above them. The pillar
was taken for closing walls as it rose - "Walls closing" talked over
"Going up": lifts and their bridges are not walls now. At the east
button, running back on from the walls slides into a spike pit with
slippery edges: the warning says where to stop, from the floor ahead:
"Run behind, 4 meters, and stop: a drop 3 meters past that! Walls
closing"; and walls held up and retrying say it once. The treasure
lies where spikes come up the moment it is taken: T says "Treasure,
spikes come up under it when it is taken... Walk up to it, and the
moment you have it, step straight back" - so taken, unhurt. Spikes that
come up and stay were "stopped: run across now"; now "Spikes up, and
they stay up: keep off them". A zombie warrior is warned of once it is
free - in sight, or on the move - not sealed behind a wall. And the
floor moving under the player is said: "The floor under you is cracking
and tilting: it is giving way", "The floor is falling, and you with
it" - the crypt's end, into the room the way out is from.
The floor drops the player in the room before the way out, behind a
door the way finding does not go through, and O said "The way out is
not open yet": now "The way out is past a door, 2 meters. Turning you to
it: F opens it, then walk straight on through" - and through it, the
level's end. And O on a thing it cannot reach, asked again from the same
spot, goes on to the way out instead of "no way" again and again: after
the fall, it was still on the treasure left above.

### 0159 The blade's safe moment

The tester still died going for the crypt's south button, the cross key
getting them there "by luck". The level's script says why: the blade,
drawn back up, waits 0.5, 1.5, 2.25 or 5 seconds at random before it
swings - the half by the button first - and across from the top of the
corridor is two seconds and more. "Back up: run across now" was a
gamble, lost one time in two. Now the slow draw back is watched, and
the go is 0.8 seconds after it ends: not swung by then, the wait is a
long one. The cross key walks the player up to the edge of the reach
first ("Walking you up to the edge of the blade's reach, 5 meters, then
waiting there for the safe moment"), faces across, and at the go runs at
a sprint. By hand, from the edge: "Now: run across, without stopping,
10 meters"; farther: "Blade back up. Too far to get across in time from
here: walk 4 meters closer, to the edge of its reach, and wait there. Or
slash crosses for you". Tested from the tester's save without god mode:
six crossings and two round trips, button pressed, all alive. A
button's own part pressed in is no longer "Walls closing"; and "Stop!"
is not said while the cross key moves the player.

### 0160 Buttons after a load

Back from dying, the tester was told the crypt's puzzle had to be done
from the beginning - after a save as much as before one. The game keeps
which buttons are pressed in its saves; the mode knows only what it has
seen of them, and kept what it saw before the load: a button pressed
after the save was taken for pressed, then seen not, and "the buttons
have reset: press all 3 again" was said. Now a load forgets what was
seen of the buttons, the lifts and the traps, without a word, and each
is learnt again as it is seen: until then T says "not checked since the
game was loaded", and seen, "pressed already" or not, as the game has
it. A button learnt to be pressed is no longer "Done. Nothing else
found", and the button's state is short enough not to be cut off: "any
order; in order, the south one first".

### 0161 Round the plates

The crypt's west button lies past three saws that cut only when a plate
on the floor sets them off - one plate by the north wall, one by the
south, one in the middle. Told the wall to keep to plate by plate, the
test player following O drifted onto one, and the saws cut it down, as
they had the tester. The cross key now takes the saws too: it finds a
way that touches no plate - the floor on a grid, each plate and a body's
width round it kept off, the walls too - and walks it, stopping at each
turn to face the next: "Now. Walking you round the plates: 5 turns", and
"Across, no plate touched. Button 3 of 3, west, ahead: F uses it". There
and back from the tester's save without god mode, the saws never
started. While it walks, the plate tips and "Don't stop here" are not
said. And a plate stepped on is no longer "The floor is falling".

### 0162 The next button, after a load

After a load, a button not seen since may be pressed already: the
tester was sent on to the crypt's south button, and found it pressed.
The next to press is now one known not to be pressed, if there is one;
only then one not checked, said so: "the south one, 30 meters, not
checked since the game was loaded: it may be pressed already". A press
counts those checked: "1 of 3 pressed, of those checked since the game
was loaded; 1 not checked yet. Next, the west one" - and is not taken
for out of order while some are not known.

### 0163 Quiet at the button past the blade

Arriving at the crypt's south button off the cross key, the tester heard
"Out! Run ahead, 1 meter" - the blade starting its swing while they were
still in the metre kept round its reach, already running out of it -
and, standing there, was never hurt. "Out!" is no longer said to someone
in that margin already leaving it at a run; in the reach itself, or
standing in the margin, it is. And "Now: run across" is said only facing
the blade: at the button, facing the button, it came every swing.
Three crossings from the tester's save without god mode: across, the
button pressed, nothing but what was needed said.

### 0164 A save loaded from the menu

From the tester's save, by the west button, the south one was "next to
press", and pressing the west one, "2 of 3 pressed. Next, the south
one" - as the pillar came down: the game had all three pressed. The
south button's part is out of what the server sends from there, and a
save loaded from the menu was taken for a level begun afresh, where
nothing is pressed. Now a loaded game is known by the save's name the
game keeps while one is played on: the south one is "not checked since
the game was loaded", the west one "next to press", and the press
counts those checked. The pillar coming down marks all three pressed.
And slash at the saws' far end went back across them, away from the
button: it now goes the way O leads, or, in the trap's reach already,
the way faced - from the tester's save, round no plate, to the button.

### 0165 F out of reach says so

F three metres from the crypt's west button turned to it and named it -
"Button 3 of 3, west, ahead, 3 meters" - and pressed nothing: F reaches
two and a half. The tester heard no press, and did not know why. Out of
reach, F now says so and what to do: "Too far to reach from here: walk
1 meter closer, and I say when F reaches it"; walking on, "Button 3 of
3, west, in reach, F to use". From the tester's save, so: the button
pressed, and the pillar down.

### 0166 The pillar tells

From the tester's save the south and east buttons were pressed, and the
west one brought the pillar down - but the mode, not having seen the
south one since the save was loaded, said "Next, the south one... O
leads there": O was taken off the pillar, and the tester took it that
the puzzle was to do again. A press that may be the last now says so and
leaves O alone: "2 of 3 pressed, of those checked since the game was
loaded. If the south one was pressed before the save, the pillar comes
down now: listen for it". The pillar coming down is said, and O leads
to it; only if it has not come down six seconds on is the next button
named. With all of them pressed, T lists them no more, and with nothing
left, says the lift: "Nothing left to take or use here. The lift is
down, behind left, 25 meters: stand on it to go up. O leads there".

### 0167 A save loaded again

The tester pressed the crypt's west button, the pillar came down - and
on the way back to it, through the saws, a plate was stepped on and the
saws killed them. The game loads the last save at once, and says
nothing: the pillar was up again, the west button out, and the mode -
not told of the load: the level's clock and start stay, and health is
as saved - said "the pillar has sunk back, and the buttons have reset".
The tester took the level, or the mode, to be broken. The buttons and
the pillar go back by themselves only after the pillar has been ridden
up; going back without that, the mode now takes it for a save loaded
again: "The last save is loaded again: what you did since it is
undone", and what was seen is forgotten and learnt again. Ridden and the
bridge missed, the reset is said as before. And the saws now say what
gets past them: "Plates on the floor set them off: slash walks you round
them, touching none", and at a plate, "Or slash walks you round".

### 0168 Wider round the plates; the bridge on O

Crossing the crypt's saws back from the west button with slash, the
first plate went down - "Across, no plate touched" said all the same -
and the saws ran: likely how the tester was killed, and the save loaded
again. A body is 36 wide here, not 32, and a plate goes down a touch
before it is reached; the way round kept 22 off, aimed once at each
turn and let drift. Now it keeps as far off as the way allows - 30, 26,
24, then 22 by the middle plate, where a hand's width lies between it
and the wall - with the body's true width against the walls, and is
steered on each turn all the way. Both ways, twice each, no plate went
down. And the bridge is on O: "Bridge out, ahead: cross now. It pulls
back in 9 seconds. O turns you to it" - O then faces its far end:
"Facing the bridge: walk straight on, 5 meters, to its far end", and
"Marker reached" across it.

### 0169 Listen for the pillar

After the west button, the tester heard "2 of 3 pressed, of those
checked since the game was loaded. If the south one was pressed before
the save, the pillar comes down now: listen for it" - and took it, with
its loading and its save, for a game loaded again. A press that may be
the last now says only "Pressed. Listen for the pillar coming down";
the pillar says itself, or, not down six seconds on, the button still
to press is named. Tested loading the save as the tester does, from the
start: no word of a load.

### 0170 Off the bridge

The tester crossed the crypt's bridge and was back among the buttons:
O's "Marker reached, the far end of the bridge" came three metres short
of where it was set - on the bridge itself - and, stopped there, the
bridge pulled back into the wall from under them, and they fell to the
floor below, the pillar reset. The far end O turns to is now set well
past the bridge, so "reached" comes on the ledge; stepping off it is
said - "Across: you're off the bridge. T finds what is next" - and the
bridge pulling back with the player still on it is urgent: "Off the
bridge! Walk on, 2 meters". Tested walking across, and stopping
halfway: both on the ledge.

### 0171 No way found

"Finding the way. A moment" - and the tester: the moment is not time
enough. O waited eight seconds for the way, and then said only "No way
found from here yet. O tries again". It now waits four; and with no way
found to something near, turns the player straight toward it and says
how far and how high - "8 meters above you" says it is another floor -
and that X lists the ways out and T what is left; to something far, the
same, without the turn.

### 0172 Stand still on the lift

The tester rode the crypt's pillar up, did not know how long to wait,
walked about on its small top and fell off it - and the bridge came out
above, out of reach. Rising, every metre was "Ledge ahead, 1 metre
down... 7 metres down", heard as a call to move. Now, stepping on:
"Going up. Stand still on it: the bridge is out in about 14 seconds" -
the time read from the bridge's own move, said once; at the top: "At
the top. Stand still: the bridge is out in 5 seconds". The ledge lines
are quiet on a lift, and its edge is a stop W again does not take the
player over: "Edge of the lift: stay on it, and wait for the bridge".
Tested walking about at the top: stopped at the edge, then across.

### 0173 The lift on F1, X and T

On the crypt's pillar the tester pressed F1 and was not told they were
on it; X and T did not name the pillar; past the bridge, T did not list
the north switch until it had been seen. Now F1 says the lift first, and
X on it says only the lift: "On the lift, going up: stand still. The
bridge is out in about 11 seconds"; "On the lift, at the top. The bridge
is out, ahead right: O turns you to it, then walk across". Down and
waiting, T says it: "The lift is down, behind left, 25 meters: stand on
it to go up. O leads there". And a switch in the area the player is in,
on their floor, is on T before it has been seen - past the bridge,
"Button for a door, ahead right, 9 meters. O leads there".

### 0174 The lift on X; O on the lift is the bridge

The tester asked for the lift on X once it is down, and on the pillar
heard O say there was no way further - O still on something chosen
before, which the pillar's top has no way to. X near the lift down now
says it first: "The lift is down, ahead, 2 meters: stand on it to go
up. O leads there. X again lists the ways out"; X again, the ways as
ever. And on a lift with a bridge, O is the bridge, whatever was chosen
before: before it is out, "On the lift: stand still. The bridge is out
in 4 seconds"; out, "Facing the bridge: walk straight on, 8 meters, to
its far end". Tested with a waypoint chosen beforehand: across.

### 0175 Locked when on target

The tester fired the Mauser the moment they heard "Locked", scope or
none, and missed far zombies again and again. "Locked" was said as the
enemy was taken, before the turn to them: tested, the sights were then
44 degrees off - a miss at any range but point blank. Now "Locked" waits
till the sights are on the enemy and the game's spread lets the shot hit
at their distance (the server's own figure: the client keeps none);
tested, said 0.14 seconds later, 6 cm off, and the shot fired on it
killed the soldier. A scope just raised starts at its least steady and
takes up to 13 seconds to settle; past about 20 metres that is said:
"Locked, Zombie, ahead, 25 meters. Hold still: your aim steadies in 9
seconds, or half that crouched", and "Steady" when it is. Said by 2
seconds whatever, so the key is never silent.

### 0176 Up on the crates

In the church the tester found a large health pack by some crates and
could not take it. Read from the map, one lies on top of a stack of
crates, three metres up - a pallet, crates on it, and one more on top -
and T said "Large Health, ahead, 3 meters. Walk onto it"; a waypoint on
it said "Marker reached" at the foot of the stack, its distance counted
in whole metres coming to three. Now T says it: "Large Health, ahead, 4
meters, on top of something, 3 meters up. Climb up to it: O leads the
way, a jump at a time"; and a marker that is a place to stand - ground
under it - is not reached from more than a jump above or below it. A
switch on a wall, reached by F from the floor, is as before.

### 0177 Onto the health

The health pack the tester could not take, with 36 health, was the one
on the floor in the church's south-west corner, in a gap between a
crate and the wall. Chosen with T and led to by O, it was "Marker
reached" three metres short of it, the crate's corner between; walking
on from there, "ahead left", went into the crate. A thing taken by
walking onto it - health, ammo, a weapon, treasure - is now reached
standing on it: O's way leads right onto it, and "Marker reached" is
said only there. Tested from two starts by the crate: reached 1 metre
off, on it, both times.

### 0178 Keys gone, enemies heard

The tester finished the crypt without the cross key, and asked for it
gone, and found M, the enemy callouts off and on, of no use. Both keys
are given up - let go of once where they are still on the mode's
command, and off the Accessibility pages - and the callouts are put
back on for a player who had turned them off; the trap lines no longer
name slash. The cross key's walking is left in the code, unbound.

They also found the callouts came too late: a sighted player sees an
enemy coming sooner, and is shooting by then. An enemy coming at the
player out of sight - within 25 metres, near their floor, moving toward
them - is now said as one in sight is, and kept while within 32 metres.
The server sends a character's movement extrapolated (TR_LINEAR_STOP),
not interpolated, which the aim's lead had been waiting for in vain.
Tested at the crypt's start with the enemies hunting: "Enemy Zombie,
ahead left, 23 meters", out of sight, where nothing was said before.

### 0179 Crouch stays

The tester pressed C to crouch and stood up the moment they let go: C
was back on the game's own crouch, which lasts only while the key is
held - the game's defaults (default.cfg) put it there, and the mode
moved C to its own crouch only once. Wherever C is on +movedown it is
now moved to the mode's crouch, C and C again, every time the game
starts. And the crouch is let go of only at death or a cutscene, not
for a moment the game holds the player still. Tested with C put back
on +movedown before the start: pressed and let go, "Crouched", and
still down after walking on.

### 0180 Ghosts thrown

The tester found zombies, and Helga at the end of the crypt, throwing
ghosts at them. A zombie stands and draws the light in for up to six
seconds (AIFunc_ZombieAttack2), the ghosts flying at its enemy as it
goes; a sighted player sees the glow gather. The attack lasts only
while it sees its enemy. Now, as one begins at the player: "Zombie
throwing ghosts at you, ahead, 12 meters: get out of its sight". Once
for each attack, and not for one at someone else: tested at the
crypt's start with the enemies hunting, the zombie's first attack, at
the soldier it was fighting, went unsaid, and its next two, at the
player, were said.

### 0181 Crouch as a setting

The tester wanted the game's own controls kept as they were, and the
crouch that stays down as an Accessibility setting instead. C is the
game's crouch again (+movedown): moved back once where it was on the
mode's, and no longer moved to it at every start as in 0179. The new
setting, Moving and aiming, "Crouch key: press to crouch, again to
stand" (eta_crouchToggle, on), makes the crouch key - whichever key the
player's Controls put it on - crouch on one press and stand on the
next: in the engine, a real key's press holds a made-up key in the
button until the next press, and its release does nothing; the mode's
own crouches, on made-up keys, hold only while held. The cgame lets it
go at death or a cutscene. "Too low to stand here" now follows the
player's command, whatever held the crouch. Tested: C tapped, crouched
through a walk, tapped, standing; with the setting off, down only
while held.
