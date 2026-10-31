# Return to Castle Wolfenstein accessibility mode

An accessibility mode for the single-player campaign of **Return to
Castle Wolfenstein**, built into **iortcw**, the game's open-source
engine, so blind players can play it without help. It reads the menus,
briefings and the game aloud through VoiceOver on the Mac and NVDA or
SAPI 5 on Windows, plays sounds placed where things are, and leads the
player through each level from the keyboard.

None of the game's own files are here or in any download: the player
brings their own. The mode, like iortcw, is under the GNU General Public
License, version 3 (LICENSE).

## What is here

- `integration/iortcw/` - the campaign: a series of patches to iortcw,
  applied in order, with a README on each.
- `integration/iortcw-sp/` and `story/` - the story game, a new campaign
  of its own built on the same engine, and `docs/story/`, its story.
- `src/accessibility/` - the accessibility library both are built with:
  speech, navigation, the spoken HUD. It comes from the Wolfenstein:
  Enemy Territory accessibility mode, where it started.
- `release/` - the build scripts, the install guide
  (`INSTALL-iortcw.md.in`), the mode's own menus (`rtcw/ui/`) and sounds.
- `.github/workflows/` - every change is built for the Mac and Windows;
  a push publishes a test build to this repository's releases page,
  which the game's own updater (F12 in the menus) installs. Builds from
  before the move read which build is the newest from the old releases
  page,
  [Wolfenstein-Enemy-territory-accessibility-releases](https://github.com/alexzgeorgian/Wolfenstein-Enemy-territory-accessibility-releases),
  which is kept pointing here.

## History

This work began in the Wolfenstein: Enemy Territory accessibility mode
repository and moved here on its own. Its earlier history, commit by
commit, is there.
