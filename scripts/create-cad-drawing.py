import ezdxf

NUM_DOMES = 7

SEPARATION = 5.0  # Gap between the domes
STANDOFF = 20.0  # Standoff left and right of the domes to the clock edge
BORDER = 10.0  # Inner border wall thickness
CLOCK_INTERNAL_HEIGHT = 50.0  # Internal height of clock cavity
DOME_OD = 28.0  # Dome outer diameter
DOME_ID = 23.5  # Dome inner diameter
TAB_WIDTH = 18.5  # Width of tab to mount LCD
TAB_HEIGHT = 36.75
TOLERANCE = 0.25  # Laser cutter tolerance
GROOVE_DEPTH = 23.0  # Glass groove depth

DIM_TEXT_OFFSET = 3.0  # How far away to place dimension line text
LCD_HEIGHT = 38.8
LCD_WIDTH = 22.0
MOUNT_HOLE_ID = 2.00
MOUNT_HOLE_OFFSET = 3.75  # Distance of centre of mounting holes from side

# Inner opening margins
INNER_MARGIN_X = BORDER
INNER_MARGIN_BOTTOM = BORDER
INNER_MARGIN_TOP = GROOVE_DEPTH + BORDER

PITCH = DOME_OD + TOLERANCE
GROOVE_WIDTH = ((DOME_OD - DOME_ID) / 2) + TOLERANCE  # Width of groove to slot glass in

# Y max is the top of the clock base rather than the extend of the model
Y_MAX = BORDER + CLOCK_INTERNAL_HEIGHT + BORDER + GROOVE_DEPTH
X_MAX = STANDOFF + PITCH * NUM_DOMES + SEPARATION * (NUM_DOMES - 1) + STANDOFF


def groove_left_x_start(n: int) -> float:
    return STANDOFF + (DOME_OD + TOLERANCE + SEPARATION) * n


def groove_left_x_end(n: int) -> float:
    return groove_left_x_start(n) + GROOVE_WIDTH


def groove_right_x_start(n: int) -> float:
    return groove_left_x_start(n) + PITCH - GROOVE_WIDTH


def groove_right_x_end(n: int) -> float:
    return groove_left_x_start(n) + PITCH


def left_groove_poly(n: int) -> list[tuple[float, float]]:
    return [
        (groove_left_x_start(n), Y_MAX),
        (groove_left_x_start(n), Y_MAX - GROOVE_DEPTH),
        (groove_left_x_end(n), Y_MAX - GROOVE_DEPTH),
        (groove_left_x_end(n), Y_MAX),
    ]


def right_groove_poly(n: int) -> list[tuple[float, float]]:
    return [
        (groove_right_x_start(n), Y_MAX),
        (groove_right_x_start(n), Y_MAX - GROOVE_DEPTH),
        (groove_right_x_end(n), Y_MAX - GROOVE_DEPTH),
        (groove_right_x_end(n), Y_MAX),
    ]


def tab_poly(n) -> list[tuple[float, float]]:
    tab_offset = (groove_right_x_start(n) - groove_left_x_end(n) - TAB_WIDTH) / 2
    return [
        (groove_left_x_end(n) + tab_offset, Y_MAX),
        (groove_left_x_end(n) + tab_offset, Y_MAX + TAB_HEIGHT),
        (groove_left_x_end(n) + tab_offset + TAB_WIDTH, Y_MAX + TAB_HEIGHT),
        (groove_left_x_end(n) + tab_offset + TAB_WIDTH, Y_MAX),
    ]


def flatten(matrix):
    return [item for row in matrix for item in row]


def add_dim_style(doc: object, dimtad: int, name: str, ext1: bool, ext2: bool) -> None:
    dimstyle = doc.dimstyles.add(name)
    dimstyle.dxf.dimdsep = ord(".")  # Decimal separator
    dimstyle.dxf.dimdec = 2  # 2 decimal places
    dimstyle.dxf.dimrnd = TOLERANCE  # Round to tolerance
    dimstyle.dxf.dimasz = 2.0  # 2mm arrows
    dimstyle.dxf.dimtxt = 3.0  # 4mm text
    dimstyle.dxf.dimtad = dimtad
    dimstyle.dxf.dimtih = 0  # text inside line
    dimstyle.dxf.dimclrd = 1  # red dim lines/arrows
    dimstyle.dxf.dimclre = 1  # red ext lines
    dimstyle.dxf.dimclrt = 5  # blue text
    dimstyle.dxf.dimlwd = 40  # 0.40mm dim lines
    dimstyle.dxf.dimlwe = 25  # 0.25mm ext lines
    dimstyle.dxf.dimgap = 1  # 2mm gap to line
    dimstyle.dxf.dimexo = 0  # Extension line gap to origin
    dimstyle.dxf.dimexe = 0  # Extension line gap to origin
    dimstyle.dxf.dimfxl = 3  # Extension line length
    dimstyle.dxf.dimfxlon = 1  # Fixed length extension lines
    dimstyle.dxf.dimse1 = 0 if ext1 else 1  # Suppress extension lines
    dimstyle.dxf.dimse2 = 0 if ext2 else 1  # Suppress extension lines


def add_dim_line(p1: tuple, p2: tuple, dimstyle: str, text_shift: tuple[float, float] = [0.0, 0.0]) -> None:
    global msp, DIM_TEXT_OFFSET
    distance = DIM_TEXT_OFFSET if dimstyle == "DimAbove" else -DIM_TEXT_OFFSET
    dim = msp.add_aligned_dim(p1=p1, p2=p2, distance=distance, dimstyle=dimstyle)
    dim.shift_text(*text_shift)
    dim.render()


outer_poly = [(0, 0), (0, Y_MAX)]
outer_poly += flatten([left_groove_poly(i) + tab_poly(i) + right_groove_poly(i) for i in range(NUM_DOMES)])
outer_poly += [(X_MAX, Y_MAX), (X_MAX, 0), (0, 0)]

inner_poly = [
    (INNER_MARGIN_X, Y_MAX - INNER_MARGIN_TOP),
    (X_MAX - INNER_MARGIN_X, Y_MAX - INNER_MARGIN_TOP),
    (X_MAX - INNER_MARGIN_X, INNER_MARGIN_BOTTOM),
    (INNER_MARGIN_X, INNER_MARGIN_BOTTOM),
    (INNER_MARGIN_X, Y_MAX - INNER_MARGIN_TOP),
]

# Use setup=True to get default dimstyles with arrows etc.
doc = ezdxf.new(dxfversion="AC1021", setup=True)

msp = doc.modelspace()
add_dim_style(doc, 1, "DimAbove", True, True)
add_dim_style(doc, 4, "DimBelow", True, True)
add_dim_style(doc, 4, "DimBelowNoSel", False, False)
add_dim_style(doc, 4, "DimBelowSel1", True, False)

# Geometry
msp.add_lwpolyline(outer_poly, close=True)
msp.add_lwpolyline(inner_poly, close=True)

# Mounting holes
for i in range(NUM_DOMES):
    tab_left = groove_left_x_end(i) + (groove_right_x_start(i) - groove_left_x_end(i) - TAB_WIDTH) / 2
    radius = (MOUNT_HOLE_ID + TOLERANCE) / 2
    msp.add_circle((tab_left + MOUNT_HOLE_OFFSET, Y_MAX + MOUNT_HOLE_OFFSET), radius)
    msp.add_circle((tab_left + TAB_WIDTH - MOUNT_HOLE_OFFSET, Y_MAX + MOUNT_HOLE_OFFSET), radius)

# Groove separation dimensions
add_dim_line((groove_left_x_end(0), Y_MAX), (groove_right_x_start(0), Y_MAX), "DimBelowNoSel")

# Groove width
add_dim_line(
    (groove_left_x_start(1), Y_MAX - GROOVE_DEPTH),
    (groove_left_x_end(1), Y_MAX - GROOVE_DEPTH),
    "DimBelow",
    (0.0, -5.0),  # Auto-alignment places text above line
)


# Groove depth
add_dim_line(
    (groove_left_x_start(1) + DIM_TEXT_OFFSET, Y_MAX - GROOVE_DEPTH),
    (groove_left_x_start(1) + DIM_TEXT_OFFSET, Y_MAX),
    "DimBelowSel1",
)

# Left standoff dimension
add_dim_line((0, Y_MAX), (STANDOFF, Y_MAX), "DimAbove")

# Right standoff dimension
add_dim_line((X_MAX - STANDOFF, Y_MAX), (X_MAX, Y_MAX), "DimAbove")

# Separation dimensions between domes
add_dim_line((groove_right_x_end(0), Y_MAX), (groove_left_x_start(1), Y_MAX), "DimAbove")

# Tab width
tab_offset = (groove_right_x_start(0) - groove_left_x_end(0) - TAB_WIDTH) / 2
add_dim_line(
    (groove_left_x_end(0) + tab_offset, Y_MAX + TAB_HEIGHT / 2),
    (groove_right_x_start(0) - tab_offset, Y_MAX + TAB_HEIGHT / 2),
    "DimBelowNoSel",
)
# Tab height
add_dim_line(
    (groove_left_x_end(1) + tab_offset, Y_MAX),
    (groove_left_x_end(1) + tab_offset, Y_MAX + TAB_HEIGHT),
    "DimBelowSel1",
)

# Inner dimensions
add_dim_line((0, BORDER * 3), (BORDER, BORDER * 3), "DimBelowNoSel")
add_dim_line((BORDER * 3, 0), (BORDER * 3, BORDER), "DimBelowNoSel")
add_dim_line((X_MAX - BORDER, BORDER * 3), (X_MAX, BORDER * 3), "DimBelowNoSel")

# Width dimension
add_dim_line((0, 0), (X_MAX, 0), "DimBelow")

# Height dimension
add_dim_line((0, 0), (0, Y_MAX), "DimAbove")

doc.saveas("generated/clock_brass.dxf")

print(f"Created template x={X_MAX}, y={Y_MAX}")
