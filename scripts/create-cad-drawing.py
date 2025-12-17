import ezdxf

doc = ezdxf.new(dxfversion="AC1021")  # DXF R2007
msp = doc.modelspace()

# LCD is 22 x 38 mm
# Cloche is 2mm thick glass
# Cloches are 28 x 71 mm
# With 10mm clearance above the LCD, the glass would need to be recessed 71 - 10 - 38 mm = 23 mm
# Clock width with 5 mm gap between cloches and 20 mm gap either side: 7 * 28 + 6 * 5 + 2 * 20 mm = 266 mm
# Clock height is 50 mm inside, 10 mm at the bottom, 10 mm below the cloches, 23mm recess: 50 + 10 + 10 + 23 mm = 93 mm
# Top profile needs to be 25 mm deep and the bottom, side profiles can be 10 mm, and 50 mm inside for the clock itself

NUM_DOMES = 7
GROOVE_WIDTH = 2.5
GROOVE_DEPTH = 23
SEPARATION = 5
STANDOFF = 20
DOME_OD = 28
Y_MAX = 93
INNER_MARGIN_X = 10
INNER_MARGIN_TOP = 35
INNER_MARGIN_BOTTOM = 10


def add_notch(outer_list, x, y_max, width, depth):
    outer_list += [
        (x, y_max),
        (x, y_max - depth),
        (x + width, y_max - depth),
        (x + width, y_max),
    ]


def generate_clock_profile(
    num_domes=NUM_DOMES,
    groove_width=GROOVE_WIDTH,
    groove_depth=GROOVE_DEPTH,
    separation=SEPARATION,
    standoff=STANDOFF,
    dome_od=DOME_OD,
    y_max=Y_MAX,
):
    groove_x_offset_1 = [
        standoff + (2 * ii * groove_width) + (dome_od * ii) + (separation * ii) for ii in range(num_domes)
    ]
    groove_x_offset_2 = [x + dome_od + groove_width for x in groove_x_offset_1]
    x_max = groove_x_offset_1[-1] + groove_width + dome_od + standoff

    outer = [
        (0, 0),
        (0, y_max),
    ]
    for x1, x2 in zip(groove_x_offset_1, groove_x_offset_2):
        for xx in [x1, x2]:
            add_notch(outer, xx, y_max, groove_width, groove_depth)
    outer += [
        (x_max, y_max),
        (x_max, 0),
        (0, 0),
    ]

    return outer, x_max


outer, x_max = generate_clock_profile()
inner = [
    (INNER_MARGIN_X, Y_MAX - INNER_MARGIN_TOP),
    (x_max - INNER_MARGIN_X, Y_MAX - INNER_MARGIN_TOP),
    (x_max - INNER_MARGIN_X, INNER_MARGIN_BOTTOM),
    (INNER_MARGIN_X, INNER_MARGIN_BOTTOM),
    (INNER_MARGIN_X, Y_MAX - INNER_MARGIN_TOP),
]

msp.add_lwpolyline(outer, close=True)
msp.add_lwpolyline(inner, close=True)

doc.saveas("generated/clock_brass.dxf")
