import os
from antenna_calculator import PatchAntennaCalculator
from cst_controller import CSTController

if __name__ == "__main__":

    calc = PatchAntennaCalculator(
        freq_ghz=2.45,
        er=3.5,
        h_mm=0.5
    )

    dims = calc.design_rectangular()

    cst = CSTController()

    cst.build_from_dimensions(dims)

    project_path = r"C:\Users\hengl\Documents\SUTD\Term 5\Electromagnetics & Apps\1D\CST antenna design python automation\AntennaProject\patch245.cst"
    cst.save(project_path)

    print("Starting Solver...")
    cst.run_solver()
    print("Solver Completed.")

    # Define export paths
    base_name = os.path.splitext(project_path)[0]
    s11_export = base_name + "_s11.txt"
    ff_export = base_name + "_farfield.txt"

    print(f"Exporting results to: {os.path.dirname(project_path)}")
    cst.export_sparameters(s11_export)

    try:
        cst.export_farfield(ff_export, dims["frequency_GHz"])
        print("Far-field export completed successfully.")
    except Exception as exc:
        print(f"Warning: far-field export skipped because CST reported no exportable plot data: {exc}")

    print("Done.")