#!/usr/bin/env python3
"""Write wl_prologue.map: the Great Temple on Revus, mission 0 of the story.

The level is laid out in docs/story/prologue.md. It is generated rather
than drawn in an editor so it can be read, reviewed and changed as text,
and rebuilt by the release script.

How it is built
---------------
The ground plan is a grid of 64-unit cells. A cell is open if its centre
falls inside one of the areas below, and solid otherwise; every row of
solid cells becomes one wall brush from the floor to the sky. So walls
and doorways cannot disagree, and the map is sealed by construction: a
floor slab under everything, the sky over everything, and solid cells
all round. An indoor area's ceiling is a brush from its height up to the
sky, so nothing is left hollow above it.

Usage:  make_map.py <output.map>
"""

import sys

CELL = 64
SKY_Z = 640
FLOOR = "castle_wall/castle_m03"
WALL = "cathedrale_wall/cathedrale_c06"
CEILING = "castle_wall/castle_c19"
METAL = "metal_misc/ametal_m01"
SKY = "skies/stars_arena7"
CAULK = "common/caulk"
TRIGGER = "common/trigger"

# name: (x0, y0, x1, y1, ceiling or None for open sky)
AREAS = {
    "court":     (-256, 0, 256, 768, None),
    "corridor":  (-96, 768, 96, 1536, 192),
    "generator": (96, 1024, 608, 1408, 224),
    "hall":      (-768, 1536, 768, 2816, 384),
    "sanctum":   (-192, 2816, 192, 3328, 256),
    "east":      (768, 2112, 2048, 2240, 192),
    "pad":       (2048, 1792, 2816, 2560, None),
}

# Solid inside open areas: the hall's four pillars.
PILLARS = [(-448, 1856, -320, 1984), (320, 1856, 448, 1984),
           (-448, 2368, -320, 2496), (320, 2368, 448, 2496)]

# The whole map, one cell of wall beyond the furthest area.
X0, Y0, X1, Y1 = -832, -64, 2880, 3392


def inside(x, y, rect):
    return rect[0] <= x < rect[2] and rect[1] <= y < rect[3]


def open_cell(cx, cy):
    x, y = cx + CELL / 2, cy + CELL / 2
    if any(inside(x, y, p) for p in PILLARS):
        return False
    return any(inside(x, y, a) for a in AREAS.values())


def brush(x0, y0, z0, x1, y1, z1, tex, sides=None):
    """An axis-aligned box. sides overrides the texture per face:
    keys 'top', 'bottom', 'north', 'south', 'east', 'west'."""
    sides = sides or {}

    def face(p1, p2, p3, name):
        t = sides.get(name, tex)
        return "( %d %d %d ) ( %d %d %d ) ( %d %d %d ) %s 0 0 0 0.5 0.5 0 0 0" % (
            p1 + p2 + p3 + (t,))

    return "\n".join([
        "{",
        face((x0, 0, 0), (x0, 1, 0), (x0, 0, 1), "west"),
        face((x1, 0, 0), (x1, 0, 1), (x1, 1, 0), "east"),
        face((0, y0, 0), (0, y0, 1), (1, y0, 0), "south"),
        face((0, y1, 0), (1, y1, 0), (0, y1, 1), "north"),
        face((0, 0, z0), (1, 0, z0), (0, 1, z0), "bottom"),
        face((0, 0, z1), (0, 1, z1), (1, 0, z1), "top"),
        "}",
    ])


def entity(classname, keys, brushes=()):
    lines = ["{", '"classname" "%s"' % classname]
    lines += ['"%s" "%s"' % kv for kv in keys]
    lines += list(brushes)
    lines.append("}")
    return "\n".join(lines)


def world():
    brushes = []
    # Floor under everything, sky over everything, and the outer shell.
    brushes.append(brush(X0, Y0, -32, X1, Y1, 0, CAULK, {"top": FLOOR}))
    brushes.append(brush(X0, Y0, SKY_Z, X1, Y1, SKY_Z + 32, CAULK, {"bottom": SKY}))
    brushes.append(brush(X0 - 32, Y0, -32, X0, Y1, SKY_Z + 32, CAULK))
    brushes.append(brush(X1, Y0, -32, X1 + 32, Y1, SKY_Z + 32, CAULK))
    brushes.append(brush(X0 - 32, Y0 - 32, -32, X1 + 32, Y0, SKY_Z + 32, CAULK))
    brushes.append(brush(X0 - 32, Y1, -32, X1 + 32, Y1 + 32, SKY_Z + 32, CAULK))

    # Walls: each row's runs of solid cells.
    for cy in range(Y0, Y1, CELL):
        run = None
        for cx in range(X0, X1 + CELL, CELL):
            solid = cx < X1 and not open_cell(cx, cy)
            if solid and run is None:
                run = cx
            elif not solid and run is not None:
                brushes.append(brush(run, cy, 0, cx, cy + CELL, SKY_Z, WALL))
                run = None

    # Ceilings: indoor areas filled from their height to the sky.
    for x0, y0, x1, y1, ceiling in AREAS.values():
        if ceiling is not None:
            brushes.append(brush(x0, y0, ceiling, x1, y1, SKY_Z, CAULK, {"bottom": CEILING}))

    return entity("worldspawn", [("message", "The Great Temple, Revus"),
                                 ("_color", "0.6 0.7 1.0"), ("ambient", "12")], brushes)


def lights():
    out = []
    for name, (x0, y0, x1, y1, ceiling) in AREAS.items():
        z = (ceiling or 256) - 48
        for x in range(x0 + 128, x1, 256):
            for y in range(y0 + 128, y1, 256):
                if any(inside(x, y, p) for p in PILLARS):
                    continue
                out.append(entity("light", [("origin", "%d %d %d" % (x, y, z)),
                                            ("light", "220"), ("_color", "0.7 0.8 1.0")]))
    return out


def soldier(name, x, y, angle, kind="ai_soldier"):
    return entity(kind, [("origin", "%d %d 24" % (x, y)), ("angle", str(angle)), ("ainame", name)])


def trigger(x0, y0, x1, y1, target, classname="trigger_multiple", extra=()):
    return entity(classname, [("target", target), ("wait", "-1")] + list(extra),
                  [brush(x0, y0, 0, x1, y1, 128, TRIGGER)])


def script_trigger(targetname, event):
    """Hands a touch or a use to the player's AI script, as trigger <event>."""
    return entity("target_script_trigger", [("targetname", targetname), ("ainame", "player"),
                                            ("target", event), ("origin", "0 0 64")])


def speaker(origin, noise, keys=()):
    return entity("target_speaker", [("origin", origin), ("noise", noise)] + list(keys))


def relay(targetname, target):
    """What a script's alertentity reaches: a speaker cannot be alerted
    itself, but a relay can, and it switches the speaker."""
    return entity("target_relay", [("targetname", targetname), ("target", target),
                                   ("origin", "0 0 64")])


def main(path):
    ents = [world()]
    ents.append(entity("info_player_start", [("origin", "0 96 24"), ("angle", "90")]))
    ents += lights()

    # The generator: a metal block that bursts when its script says so,
    # and around it the space where F sets the charge.
    ents.append(entity("func_explosive", [("targetname", "generator"), ("scriptname", "generator"),
                                          ("type", "metal"), ("dmg", "0")],
                       [brush(448, 1152, 0, 544, 1280, 96, METAL)]))
    ents.append(entity("func_invisible_user", [("target", "t_sabotage"), ("cursorhint", "HINT_ACTIVATE"),
                                               ("targetname", "generator_use")],
                       [brush(384, 1088, 0, 608, 1344, 128, TRIGGER)]))
    ents.append(script_trigger("t_sabotage", "sabotage"))
    ents.append(speaker("496 1216 64", "sound/scenaric/general/misc_hum1.wav",
                        [("spawnflags", "1"), ("targetname", "generator_hum_speaker")]))
    ents.append(relay("generator_hum", "generator_hum_speaker"))

    # Walking into the hall, onto the supercolony, onto the ship's ramp.
    ents.append(trigger(-768, 1536, 768, 1664, "t_hall"))
    ents.append(script_trigger("t_hall", "hall"))
    ents.append(trigger(-64, 3104, 64, 3232, "t_colony"))
    ents.append(script_trigger("t_colony", "colony"))
    ents.append(trigger(2496, 2112, 2624, 2240, "t_ship"))
    ents.append(script_trigger("t_ship", "ship"))

    # Sound to place by: wind in the open, the alarm once the colony is
    # taken, the ship's engines on the pad.
    ents.append(speaker("0 384 128", "sound/world/amb_wind.wav", [("spawnflags", "1")]))
    ents.append(speaker("0 2176 256", "sound/scenaric/assault/fire_alarm_loop1.wav",
                        [("targetname", "alarm_speaker"), ("spawnflags", "2")]))
    ents.append(relay("alarm", "alarm_speaker"))
    ents.append(speaker("2560 2176 64", "sound/scenaric/general/misc_hum2.wav", [("spawnflags", "1")]))

    # The Twelfth Legion: six in the hall, two on the way to the ship.
    for i, (x, y, a) in enumerate([(-576, 2048, 270), (576, 2048, 270), (-576, 2624, 270),
                                   (576, 2624, 270), (-192, 2688, 270), (192, 2688, 270)]):
        ents.append(soldier("sentinel%d" % (i + 1), x, y, a))
    ents.append(soldier("guard1", 1600, 2176, 180, "ai_eliteguard"))
    ents.append(soldier("guard2", 2304, 1984, 180, "ai_eliteguard"))

    # Health and ammunition.
    for x, y, cls in [(160, 576, "item_health"), (-640, 1728, "item_health"),
                      (640, 2752, "item_health"), (-160, 576, "ammo_9mm"),
                      (0, 3040, "ammo_9mm"), (1984, 2176, "item_health")]:
        ents.append(entity(cls, [("origin", "%d %d 16" % (x, y))]))

    with open(path, "w") as handle:
        handle.write("\n".join(ents) + "\n")


if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    main(sys.argv[1])
