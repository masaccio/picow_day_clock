import cadquery as cq

wp = cq.importers.importDXF("generated/clock_brass.dxf")

# Select all wires and extrude at 4mm depth
solid = wp.wires().toPending().extrude(4)

cq.exporters.export(solid, "generated/clock_brass.stl")
