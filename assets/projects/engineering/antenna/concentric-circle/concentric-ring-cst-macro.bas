' =====================================================================
'  Concentric-Ring Microstrip Antenna  -  CST Studio Suite VBA macro
'  Cleaned & parametrised rebuild of the supplied history file.
'  Goal: |S11| dip (< -10 dB) at 2.45 GHz, with broadside gain.
'
'  HOW TO RUN:
'    Home > Macros > Edit/Run Macro  ->  paste this  ->  Run.
'  (Run it on a fresh, empty project.)
'
'  All geometry is built in GLOBAL coordinates and the port is placed
'  by coordinates (no face/edge picks), so it survives parameter sweeps.
' =====================================================================
Option Explicit

Sub Main ()

    '----------------------------------------------------------------
    ' 0)  PARAMETERS  (mm / GHz)  --  these are what you sweep later
    '----------------------------------------------------------------
    StoreParameter("sub_x",   25)      ' substrate HALF length (X)  -> 50 mm
    StoreParameter("sub_y",   15)      ' substrate HALF width  (Y)  -> 30 mm
    StoreParameter("sub_h",   0.5)     ' substrate thickness   (Z)
    StoreParameter("eps_r",   3.66)    ' substrate permittivity
    StoreParameter("tand",    0.0037)  ' substrate loss tangent
    StoreParameter("gnd_t",   0.035)   ' ground copper thickness
    StoreParameter("met_t",   0.01)    ' patch copper thickness

    StoreParameter("cx",     -2)       ' ring centre X
    StoreParameter("cy",      0)       ' ring centre Y
    StoreParameter("r_out_o", 12.5)    ' outer ring  OUTER radius   *
    StoreParameter("r_out_i", 10)      ' outer ring  INNER radius   *
    StoreParameter("r_in_o",  6)       ' inner ring  OUTER radius   *
    StoreParameter("r_in_i",  2)       ' inner ring  INNER radius   *
    '   ( * = primary knobs for moving the 2.45 GHz resonance )

    StoreParameter("feed_x1", 10)      ' feed line start X (joins outer ring)
    StoreParameter("feed_w",  1)       ' feed line HALF width (Y)
    StoreParameter("con_x1", -13)      ' connect bar X1
    StoreParameter("con_x2", -7)       ' connect bar X2
    StoreParameter("con_w",   1)       ' connect bar HALF width (Y)

    StoreParameter("fmin",    1)       ' solver f-min
    StoreParameter("fmax",    4)       ' solver f-max
    StoreParameter("f0",      2.45)    ' design / monitor frequency

    '----------------------------------------------------------------
    ' 1)  UNITS
    '----------------------------------------------------------------
    With Units
        .Geometry "mm"
        .Frequency "GHz"
        .Time "ns"
    End With

    '----------------------------------------------------------------
    ' 2)  MATERIALS
    '----------------------------------------------------------------
    With Material
        .Reset
        .Name "Copper (annealed)"
        .Folder ""
        .FrqType "all"
        .Type "Lossy metal"
        .SetMaterialUnit "GHz", "mm"
        .Mu "1.0"
        .Kappa "5.8e7"
        .Rho "8930"
        .Colour "1", "1", "0"
        .Create
    End With

    With Material
        .Reset
        .Name "Rogers RO4350B (lossy)"
        .Folder ""
        .FrqType "all"
        .Type "Normal"
        .SetMaterialUnit "GHz", "mm"
        .Epsilon "eps_r"
        .Mu "1.0"
        .Kappa "0.0"
        .TanD "tand"
        .TanDFreq "10.0"
        .TanDGiven "True"
        .TanDModel "ConstTanD"
        .Colour "0.94", "0.82", "0.76"
        .Create
    End With

    '----------------------------------------------------------------
    ' 3)  COMPONENT + SUBSTRATE + GROUND
    '----------------------------------------------------------------
    Component.New "component1"

    With Brick
        .Reset
        .Name "Substrate"
        .Component "component1"
        .Material "Rogers RO4350B (lossy)"
        .Xrange "-sub_x", "sub_x"
        .Yrange "-sub_y", "sub_y"
        .Zrange "0", "sub_h"
        .Create
    End With

    With Brick
        .Reset
        .Name "Ground"
        .Component "component1"
        .Material "Copper (annealed)"
        .Xrange "-sub_x", "sub_x"
        .Yrange "-sub_y", "sub_y"
        .Zrange "-gnd_t", "0"
        .Create
    End With

    '----------------------------------------------------------------
    ' 4)  RADIATING LAYER  (sits on top of substrate)
    '     Z = [ sub_h , sub_h + met_t ]
    '----------------------------------------------------------------
    ' 4a) feed line  -- this solid becomes the final merged "Patch"
    With Brick
        .Reset
        .Name "Patch"
        .Component "component1"
        .Material "Copper (annealed)"
        .Xrange "feed_x1", "sub_x"
        .Yrange "-feed_w", "feed_w"
        .Zrange "sub_h", "sub_h+met_t"
        .Create
    End With

    ' 4b) outer ring (annulus)
    With Cylinder
        .Reset
        .Name "OuterRing"
        .Component "component1"
        .Material "Copper (annealed)"
        .OuterRadius "r_out_o"
        .InnerRadius "r_out_i"
        .Axis "z"
        .Xcenter "cx"
        .Ycenter "cy"
        .Zrange "sub_h", "sub_h+met_t"
        .Segments "0"
        .Create
    End With

    ' 4c) inner ring (annulus)
    With Cylinder
        .Reset
        .Name "InnerRing"
        .Component "component1"
        .Material "Copper (annealed)"
        .OuterRadius "r_in_o"
        .InnerRadius "r_in_i"
        .Axis "z"
        .Xcenter "cx"
        .Ycenter "cy"
        .Zrange "sub_h", "sub_h+met_t"
        .Segments "0"
        .Create
    End With

    ' 4d) bar connecting inner ring <-> outer ring
    With Brick
        .Reset
        .Name "ConnectBar"
        .Component "component1"
        .Material "Copper (annealed)"
        .Xrange "con_x1", "con_x2"
        .Yrange "-con_w", "con_w"
        .Zrange "sub_h", "sub_h+met_t"
        .Create
    End With

    ' 4e) merge everything into one solid: component1:Patch
    Solid.Add "component1:Patch", "component1:OuterRing"
    Solid.Add "component1:Patch", "component1:InnerRing"
    Solid.Add "component1:Patch", "component1:ConnectBar"

    '----------------------------------------------------------------
    ' 5)  BACKGROUND + OPEN BOUNDARIES  (required for farfield / gain)
    '----------------------------------------------------------------
    With Background
        .ResetBackground
        .Type "Normal"
        .Epsilon "1.0"
        .Mu "1.0"
        .ApplyInAllDirections "True"
    End With

    With Boundary
        .Xmin "expanded open"
        .Xmax "expanded open"
        .Ymin "expanded open"
        .Ymax "expanded open"
        .Zmin "expanded open"
        .Zmax "expanded open"
        .ApplyInAllDirections "False"
    End With

    '----------------------------------------------------------------
    ' 6)  DISCRETE PORT  (edge feed at board edge X = sub_x)
    '     feed top (z = sub_h+met_t)  ->  ground bottom (z = -gnd_t)
    '----------------------------------------------------------------
    With DiscretePort
        .Reset
        .PortNumber "1"
        .Type "SParameter"
        .Impedance "50.0"
        .SetP1 "False", "sub_x", "0", "sub_h+met_t"
        .SetP2 "False", "sub_x", "0", "-gnd_t"
        .UseProjection "False"
        .ReverseProjection "False"
        .Create
    End With

    '----------------------------------------------------------------
    ' 7)  FREQUENCY RANGE + MONITORS
    '----------------------------------------------------------------
    Solver.FrequencyRange "fmin", "fmax"

    With Monitor
        .Reset
        .Name "farfield (f=2.45)"
        .Domain "Frequency"
        .FieldType "Farfield"
        .MonitorValue "f0"
        .ExportFarfieldSource "False"
        .Create
    End With

    With Monitor
        .Reset
        .Name "e-field (f=2.45)"
        .Dimension "Volume"
        .Domain "Frequency"
        .FieldType "Efield"
        .MonitorValue "f0"
        .Create
    End With

    '----------------------------------------------------------------
    ' 8)  TIME-DOMAIN SOLVER
    '----------------------------------------------------------------
    ChangeSolverType "HF Time Domain"

    Mesh.SetCreator "High Frequency"
    With Solver
        .Method "Hexahedral"
        .CalculationType "TD-S"
        .StimulationPort "All"
        .StimulationMode "All"
        .SteadyStateLimit "-40"
        .MeshAdaption "False"
        .AutoNormImpedance "True"
        .NormingImpedance "50"
    End With

    ' --- Uncomment the next line to launch the solver automatically ---
    ' Solver.Start

End Sub
