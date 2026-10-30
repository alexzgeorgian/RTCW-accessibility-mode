# Mission 0: The Silent Shock

From the novel's prologue. Revus, a world fifty times the size of Earth
in the Andromeda galaxy, under a blue sun and nine moons. An intruder
from Earth - never named in the book - brings down the energy shield of
the Great Temple in the imperial capital, cuts through the Twelfth
Legion of sentinels, steals a Tettoran supercolony and escapes on a
cloaked warship. The player is that intruder.

Map name: `wl_prologue`.

## Layout

Built to be found by ear: no doors, open doorways, one way forward, and
the next place to go always marked by the beacon. One meter is 36 game
units; north is up the map (the game's yaw 90).

```
                 +---------+
                 | Sanctum |  supercolony
                 +----+----+
                      |
        +-------------+-------------+        +-----------------+
        |                           |  east  |                 |
        |        Great Hall         +--------+  Landing pad    |
        |   6 sentinels, pillars    | corridor  ship, 2 guards |
        +-------------+-------------+        +-----------------+
                      |
          +-----------+   north corridor
          |           |
          |           +------------+
          |           | Generator  |  hum
          |           | room       |
          |           +------------+
          |           |
        +-+-----------+-+
        |  Outer court  |  start, facing north
        +---------------+
```

| Area | From | To | Notes |
|------|------|----|-------|
| Outer court | (-256, 0) | (256, 768) | start at (0, 96), facing north; sky |
| North corridor | (-96, 768) | (96, 1536) | opening east at y 1056-1248 |
| Generator room | (96, 1024) | (608, 1408) | generator at (480, 1216) |
| Great Hall | (-768, 1536) | (768, 2816) | 6 m high, four pillars |
| Sanctum | (-192, 2816) | (192, 3328) | supercolony at (0, 3168) |
| East corridor | (768, 2112) | (2048, 2240) | from the hall's east wall |
| Landing pad | (2048, 1792) | (2816, 2560) | sky; ship at (2560, 2176) |

## Enemies

Sentinels of the Twelfth Legion, played by RTCW's soldiers and elite
guards; the mode calls them by the book's names through
`eta_enemyNames`.

- Great Hall: six sentinels, alerted when the shield falls.
- East corridor and landing pad: two sentinels, waiting.

## Sequence and lines

1. **Start**, in the outer court.
   - Line: "Revus, in the Andromeda galaxy. A world fifty times the size
     of Earth, under a blue sun and nine moons. Night here lasts
     twenty-two hours."
   - Line: "Ahead stands the Great Temple of the Ah'Zur, the Chosen of
     the Lantern, behind its energy shield. Somewhere inside it lies a
     Tettoran supercolony."
   - Objective: "Bring down the temple shield. The generator is in the
     room east of the north corridor. Press F at the generator."
   - Beacon: the generator.
2. **The generator** (F at it).
   - Line: "Charge set. Five seconds."
   - It explodes. Line: "A silent shock wave. The shield flickers once,
     an iridescent bubble, and vanishes into nothing."
   - Line: "The sentinels of the Twelfth Legion are awake, and they are
     firing blind."
   - Objective: "Take the Tettoran supercolony from the sanctum, past
     the great hall to the north."
   - Beacon: the supercolony.
3. **The great hall** (walking in).
   - Line: "The Twelfth Legion forms a wall of shields. Blue plasma
     lights the dark."
4. **The supercolony** (walking onto it).
   - Line: "The supercolony is yours. Every alarm in the capital wakes
     at once."
   - Objective: "Reach your cloaked warship on the landing pad, through
     the east door of the great hall."
   - Beacon: the ship.
5. **The ship** (walking onto the ramp). The mission ends.
   - Line: "Patrol corvettes burst one after another, hunted by a ship
     that refuses to be seen. In the throne room, a single cry for help
     reaches Emperor Quar'rak. He rises, and says one word. War."

## Weapons

The player starts with RTCW's knife, Luger and MP40, and there are
health packs in the court and the hall. The weapons keep RTCW's names
for now.
