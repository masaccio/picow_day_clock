import cadquery as cq

# Import DXF as a wire
wp = cq.importers.importDXF("generated/clock_brass.dxf")

# Select all wires and extrude
solid = wp.wires().toPending().extrude(4)

# Export STL
cq.exporters.export(solid, "generated/clock_brass.stl")
