# Wolf's Lair: The End of Terror - the game

A story campaign for Return to Castle Wolfenstein's single player, built
from Alexandru Georgian's novel "Bârlogul lupilor: Sfârșitul terorii"
(2022). It runs on the RTCW accessibility mode
(`integration/iortcw-sp/`), so it is played with a screen reader and
the keyboard, and it is spoken in English.

The novel's own text stays with its author and is not in this
repository. What is here is the game's script: the missions, their
lines in English, and how each level is laid out.

## How a mission tells its story

Each level's AI script sets two values the mode reads aloud:

- `eta_line` - a line of the story, said when it changes. It can name a
  recorded voice first, `sound/story/... | the words`; the recording is
  played while the player keeps recorded voices on, the words are
  spoken otherwise.
- `eta_objective` - the current objective, said when it changes, and
  again on O.

From patch 0004, `eta_goal` places a sound beacon at the next place to
go, so the way can be heard.

## The missions

A draft: one mission per stretch of the book where something is fought
for. The order follows the novel.

| # | Mission | From the novel | Player |
|---|---------|----------------|--------|
| 0 | The Silent Shock | Prologue: the Great Temple on Revus, the shield falls, the supercolony stolen | The infiltrator |
| 1 | Waterway | Chapter 3: the night at the Waterway club | Jason Hank |
| 2 | The Assault | Chapter 5: the Wolf Team's first raid | Jason Hank |
| 3 | Xaren's Last Charge | Chapters 6 and 7: the storming of the imperial palace on Revus | Xaren |
| 4 | A Hot Pursuit | Chapter 8: helicopters over South Africa, the ISO-X missiles | Jason Hank |
| 5 | The Jungle | Chapters 9 and 10: an old friend, Delta Force's crash | Jason Hank |
| 6 | The Vultures' Nest | Chapters 11 to 13: Death Man's "game" in his base | Jason Hank |
| 7 | The Damsel in Distress | Chapter 14: finding Lara in the burning base | Jason Hank |
| 8 | The End of Terror | Chapters 15 and 16: the Vultures' nest, Death Man in his armour | Jason Hank |

The prologue is built first, to test the whole path - the level, the
enemies, the spoken story and the beacon - with a screen reader before
more is made. See `prologue.md`.
