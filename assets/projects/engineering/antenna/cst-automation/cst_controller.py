import sys
import os
import shutil
import fnmatch

sys.path.append(r"C:\Program Files (x86)\CST Studio Suite 2024\AMD64\python_cst_libraries")

from cst.interface import DesignEnvironment

class CSTController:

    def __init__(self):
        self.de = DesignEnvironment()
        self.project = self.de.new_mws()
        self.set_units()

    def store_parameter(self, name, value):
        self.project.model3d.add_to_history(f"Parameter_{name}", f'StoreParameter("{name}", "{value}")')

    def set_units(self):
        self.project.model3d.add_to_history(
            "Units",
            r'''
            With Units
                .Geometry "mm"
                .Frequency "GHz"
                .Time "ns"
            End With
            ''')

    def set_frequency_range(self, fmin, fmax):
        self.project.model3d.add_to_history("Frequency Range", f'Solver.FrequencyRange "{fmin}", "{fmax}"')

    def save(self, filename):
        directory = os.path.dirname(filename)
        if directory and not os.path.exists(directory):
            os.makedirs(directory)
        if os.path.exists(filename):
            os.remove(filename)
        project_folder = os.path.splitext(filename)[0]
        if os.path.exists(project_folder):
            shutil.rmtree(project_folder)
        self.project.save(filename)
        self.project_path = filename

    def create_materials(self):

        self.project.model3d.add_to_history(
            "Materials",
            r'''
            With Material
                .Reset
                .Name "Polyimide"
                .Epsilon "3.5"
                .TanD "0.0027"
                .Create
            End With

            With Material
                .Reset
                .Name "CopperCustom"
                .Type "Lossy metal"
                .Sigma "5.8e7"
                .Create
            End With
            '''
        )

    def create_substrate(self, W, L, h):

        self.project.model3d.add_to_history(
            "Substrate",
            f'''
            With Brick
                .Reset
                .Name "Substrate"
                .Component "component1"
                .Material "Polyimide"

                .Xrange "{-W/2}", "{W/2}"
                .Yrange "{-L/2}", "{L/2}"
                .Zrange "0", "{h}"

                .Create
            End With
            '''
        )

    def create_ground(self, W, L):

        self.project.model3d.add_to_history(
            "Ground",
            f'''
            With Brick
                .Reset
                .Name "Ground"
                .Component "component1"
                .Material "CopperCustom"

                .Xrange "{-W/2}", "{W/2}"
                .Yrange "{-L/2}", "{L/2}"
                .Zrange "-0.035", "0"

                .Create
            End With
            '''
        )
    
    # Patch + Feed
    def create_patch_with_feed(
        self,
        patch_W,
        patch_L,
        feed_W,
        feed_inset,
        substrate_h,
        feed_length=15.0
    ):

        copper_t = 0.035
        
        # Patch     
        self.project.model3d.add_to_history(
            "Patch",
            f'''
            With Brick
                .Reset
                .Name "Patch"
                .Component "component1"
                .Material "CopperCustom"

                .Xrange "{-patch_W/2}", "{patch_W/2}"
                .Yrange "{-patch_L/2}", "{patch_L/2}"
                .Zrange "{substrate_h}", "{substrate_h + copper_t}"

                .Create
            End With
            '''
        )

        # Feed
        y1 = -patch_L/2 - feed_length
        y2 = -patch_L/2 + feed_inset
        gap = 0.2

        self.project.model3d.add_to_history(
            "Feed",
            f'''
            With Brick
                .Reset
                .Name "Feed"
                .Component "component1"
                .Material "CopperCustom"

                .Xrange "{-feed_W/2}", "{feed_W/2}"
                .Yrange "{y1}", "{y2-gap}"
                .Zrange "{substrate_h}","{substrate_h + copper_t}"

                .Create
            End With
            '''
        )

        # Merge patch and feed

        self.project.model3d.add_to_history(
            "Merge Feed",
            '''
            Solid.Add "component1:Patch","component1:Feed"
            '''
        )

    # PORT
    def add_discrete_port(
        self,
        patch_L,
        inset_dist,
        substrate_h,
        feed_length=15.0
    ):
        # Place the port at the start of the feed line (edge of the substrate)
        # This connects the Ground (z=0) to the Feed line (z=substrate_h)
        x = 0.0
        y = -patch_L/2 - feed_length
        z1 = 0.0
        z2 = substrate_h
        
        self.project.model3d.add_to_history(
            "Discrete Port",
            f'''
            With DiscretePort
                .Reset
                .PortNumber "1"
                .Type "SParameter"
                .Impedance "50"
                .SetP1 "False", "{x}", "{y}", "{z1}"
                .SetP2 "False", "{x}", "{y}", "{z2}"
                .InvertDirection "False"
                .LocalCoordinates "False"
                .Monitor "True"
                .Create
            End With
            '''
        )
        
    # Boundaries
    def set_boundaries(self, airbox):
        self.project.model3d.add_to_history("Boundaries", f'''
            With Boundary
                .Xmin "expanded open"
                .Xmax "expanded open"
                .Ymin "expanded open"
                .Ymax "expanded open"
                .Zmin "expanded open"
                .Zmax "expanded open"
            End With
        ''')
        # Define the surrounding air box space margins on background space
        self.project.model3d.add_to_history("Background Space", f'''
            With Background
                .XminSpace "{airbox}"
                .XmaxSpace "{airbox}"
                .YminSpace "{airbox}"
                .YmaxSpace "{airbox}"
                .ZminSpace "{airbox}"
                .ZmaxSpace "{airbox}"
                .ApplyInAllDirections "False"
            End With
        ''')
    
    # Solver
    def configure_solver(
                            self,
                            fmin=1.5,
                            fmax=3.5
                        ):

        self.project.model3d.add_to_history(
            "Frequency Range",
            f'''
            Solver.FrequencyRange "{fmin}", "{fmax}"
            '''
        )

        self.project.model3d.add_to_history(
            "Solver",
            '''
            ChangeSolverType "HF Time Domain"
            '''
        )

    def add_farfield_monitor(self, freq):
        """Adds a Farfield monitor at a specific frequency (GHz)."""
        self.project.model3d.add_to_history(
            f"Farfield Monitor {freq}",
            f'''
            With Monitor
                .Reset
                .Name "farfield (f={freq})"
                .Domain "Frequency"
                .FieldType "Farfield"
                .Frequency "{freq}"
                .UseSubvolume "False"
                .Create
            End With
            ''')
    
    def build_from_dimensions(self, dims):

        self.create_materials()

        self.create_substrate(
            dims["ground_W_mm"],
            dims["ground_L_mm"],
            dims["substrate_h_mm"]
        )

        self.create_ground(
            dims["ground_W_mm"],
            dims["ground_L_mm"]
        )

        self.create_patch_with_feed(
            dims["patch_W_mm"],
            dims["patch_L_mm"],
            dims["feed_W_mm"],
            dims["feed_inset_mm"],
            dims["substrate_h_mm"]
        )

        self.add_discrete_port(
            patch_L=dims["patch_L_mm"],
            inset_dist=dims["feed_inset_mm"],
            substrate_h=dims["substrate_h_mm"]
        )

        self.set_boundaries(
            dims["airbox_margin_mm"]
        )

        # Set solver range and a monitor at the design frequency
        center_f = dims["frequency_GHz"]
        self.configure_solver(fmin=center_f*0.8, fmax=center_f*1.2)
        self.add_farfield_monitor(center_f)
    
      # Run Solver
    def run_solver(self):
        self.project.model3d.run_solver()
    def export_sparameters(self, filename):
        """Exports S1,1 data to an ASCII file."""
        # CST VBA prefers forward slashes in paths
        clean_path = filename.replace('\\', '/')
        self.project.model3d.add_to_history(
            "Export S-Parameters",
            f'''
            SelectTreeItem "1D Results\\S-Parameters\\S1,1"
            With ASCIIExport
                .Reset
                .FileName "{clean_path}"
                .Execute
            End With
            '''
        )

    def export_farfield(self, filename, freq):
        """Exports farfield results to a text file for a specific frequency."""
        # CST sometimes exposes far-field monitors under slightly different tree paths.
        # Try the common variants in order and fall back if the selected item has no plot data yet.
        f_str = str(freq)
        clean_path = filename.replace('\\', '/')
        candidate_items = [
            f"Farfields\\farfield (f={f_str}) [1]",
            f"1D Results\\Farfields\\farfield (f={f_str}) [1]",
            f"Farfields\\farfield (f={f_str})",
            f"1D Results\\Farfields\\farfield (f={f_str})",
        ]

        last_error = None

        for item in candidate_items:
            try:
                self.project.model3d.add_to_history(
                    "Export Farfield",
                    f'''
                    SelectTreeItem "{item}"
                    FarfieldPlot.Plot
                    With ASCIIExport
                        .Reset
                        .FileName "{clean_path}"
                        .Execute
                    End With
                    '''
                )
                return
            except RuntimeError as exc:
                last_error = exc
                message = str(exc)
                if "No plot data available for export" not in message and "no such property or method" not in message:
                    raise

        fallback_file = self._find_farfield_result_file(freq)
        if fallback_file:
            directory = os.path.dirname(filename)
            if directory and not os.path.exists(directory):
                os.makedirs(directory, exist_ok=True)
            shutil.copy2(fallback_file, filename)
            return

        raise RuntimeError(
            "Far-field export failed: no far-field plot data was available for any detected CST result path. "
            "Please confirm the far-field monitor finished successfully in the project before exporting."
        ) from last_error

    def _find_farfield_result_file(self, freq):
        """Locate the generated far-field result file in the CST project output folder."""
        project_path = getattr(self, 'project_path', None)
        if not project_path:
            return None

        project_folder = os.path.splitext(project_path)[0]
        result_dir = os.path.join(project_folder, 'Result')
        if not os.path.isdir(result_dir):
            return None

        f_str = str(freq)
        patterns = [
            f"farfield (f={f_str})_1.ffm",
            f"farfield (f={f_str}).ffm",
            f"farfield (f={f_str})_*.ffm",
            f"farfield (f={f_str})*.ffm",
        ]

        for pattern in patterns:
            matches = [os.path.join(result_dir, name) for name in os.listdir(result_dir) if fnmatch.fnmatch(name, pattern)]
            if matches:
                return matches[0]

        return None