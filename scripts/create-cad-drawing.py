import ezdxf

NUM_DOMES = 7

# Domes are 28mm with 2mm glass but allow 0.5mm clearance
DOME_OD = 28.0
GLASS_THICKNESS = 2.0
GROOVE_WIDTH = 2.5
GROOVE_DEPTH = 23.0

SEPARATION = 5.0  # Gap between the domes
STANDOFF = 20.0  # Standoff left and right of the domes
BORDER = 10.0  # Inner border wall thickness
CLOCK_INTERNAL_HEIGHT = 50.0  # Internal height of clock cavity

Y_MAX = BORDER + CLOCK_INTERNAL_HEIGHT + BORDER + GROOVE_DEPTH

# Effective width occupied by one DOME + groove clearance
EFFECTIVE_DOME_WIDTH = DOME_OD + (GROOVE_WIDTH - GLASS_THICKNESS)

# Pitch between dome centers
PITCH = EFFECTIVE_DOME_WIDTH + SEPARATION

X_MAX = 2 * STANDOFF + NUM_DOMES * EFFECTIVE_DOME_WIDTH + (NUM_DOMES - 1) * SEPARATION

# Inner opening margins
INNER_MARGIN_X = BORDER
INNER_MARGIN_BOTTOM = BORDER
INNER_MARGIN_TOP = GROOVE_DEPTH + BORDER


def add_notch(poly, x, y_top, width, depth):
    poly += [
        (x, y_top),
        (x, y_top - depth),
        (x + width, y_top - depth),
        (x + width, y_top),
    ]


outer = [(0, 0), (0, Y_MAX)]

for i in range(NUM_DOMES):
    x1 = STANDOFF + i * PITCH
    x2 = x1 + EFFECTIVE_DOME_WIDTH

    add_notch(outer, x1, Y_MAX, GROOVE_WIDTH, GROOVE_DEPTH)
    add_notch(outer, x2, Y_MAX, GROOVE_WIDTH, GROOVE_DEPTH)

outer += [
    (X_MAX, Y_MAX),
    (X_MAX, 0),
    (0, 0),
]

inner = [
    (INNER_MARGIN_X, Y_MAX - INNER_MARGIN_TOP),
    (X_MAX - INNER_MARGIN_X, Y_MAX - INNER_MARGIN_TOP),
    (X_MAX - INNER_MARGIN_X, INNER_MARGIN_BOTTOM),
    (INNER_MARGIN_X, INNER_MARGIN_BOTTOM),
    (INNER_MARGIN_X, Y_MAX - INNER_MARGIN_TOP),
]

doc = ezdxf.new(dxfversion="AC1021")  # DXF >=R2002 required for polyline

msp = doc.modelspace()
msp.add_lwpolyline(outer, close=True)
msp.add_lwpolyline(inner, close=True)

doc.saveas("generated/clock_brass.dxf")

print(f"Created template x={X_MAX}, y={Y_MAX}")
