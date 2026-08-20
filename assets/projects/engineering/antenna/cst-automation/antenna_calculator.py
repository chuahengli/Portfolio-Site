import numpy as np

class PatchAntennaCalculator:

    """
    Rectangular microstrip patch antenna calculator

    Intended for:
    - 2.45 GHz wearable patch antenna
    - Polyimide substrate
    - CST parameter generation
    """


    def __init__(self, freq_ghz=2.45, er=3.5, h_mm=0.5, tan_delta=0.0027, copper_thickness_um=35):
        self.freq_ghz = freq_ghz
        self.er = er
        self.h_mm = h_mm
        self.tan_delta = tan_delta
        self.t_um = copper_thickness_um

    def design_rectangular(self):

        c = 299792458.0

        f = self.freq_ghz * 1e9
        h = self.h_mm * 1e-3

        # Patch Width
        W = (c / (2 * f)) * np.sqrt(2 / (self.er + 1))

        # Effective dielectric constant
        er_eff = (
            (self.er + 1) / 2
            + ((self.er - 1) / 2)
            * (1 + 12 * h / W) ** (-0.5)
        )

        # Fringing extension
        delta_L = (
            0.412
            * h
            * (
                ((er_eff + 0.3) * (W / h + 0.264))
                /
                ((er_eff - 0.258) * (W / h + 0.8))
            )
        )

        # Effective length
        L_eff = c / (2 * f * np.sqrt(er_eff))
        
        # Actual patch length
        L = L_eff - 2 * delta_L

        # Wavelengths        
        lam0 = c / f
        lamg = lam0 / np.sqrt(er_eff)

        # 50-ohm microstrip feed width
        # Hammerstad approximation     
        A = (
            (50 / 60)
            * np.sqrt((self.er + 1) / 2)
            + ((self.er - 1) / (self.er + 1))
            * (0.23 + 0.11 / self.er)
        )

        Wf_h = (8 * np.exp(A)) / (np.exp(2 * A) - 2)

        if Wf_h < 2:
            Wf = Wf_h * h
        else:
            B = (377 * np.pi) / (
                2 * 50 * np.sqrt(self.er)
            )

            Wf = (
                (2 / np.pi)
                * h
                * (
                    B
                    - 1
                    - np.log(2 * B - 1)
                    + ((self.er - 1) / (2 * self.er))
                    * (
                        np.log(B - 1)
                        + 0.39
                        - 0.61 / self.er
                    )
                )
            )
        
        # Inset feed estimate
        R_edge = 300

        try:
            y0 = (
                L / np.pi
            ) * np.arccos(np.sqrt(50 / R_edge))
        except:
            y0 = 0.25 * L
        
        # Recommended ground plane        
        ground_W = W + 6 * h
        ground_L = L + 6 * h
        
        # CST open space recommendation        
        air_margin = lam0 / 4

        return {

            "frequency_GHz": self.freq_ghz,

            "patch_W_mm": W * 1000,
            "patch_L_mm": L * 1000,

            "feed_W_mm": Wf * 1000,
            "feed_inset_mm": y0 * 1000,

            "ground_W_mm": ground_W * 1000,
            "ground_L_mm": ground_L * 1000,

            "substrate_h_mm": self.h_mm,

            "epsilon_eff": er_eff,

            "delta_L_mm": delta_L * 1000,

            "lambda0_mm": lam0 * 1000,
            "lambda_g_mm": lamg * 1000,

            "airbox_margin_mm": air_margin * 1000
        }
    def estimate_bending_shift(self, radius_mm):

        dims = self.design_rectangular()

        L = dims["patch_L_mm"] * 1e-3
        R = radius_mm * 1e-3

        delta_L = (L ** 2) / (8 * R)

        shift_percent = 100 * delta_L / L

        f_bent = self.freq_ghz * (
            1 - shift_percent / 200
        )

        return {
            "radius_mm": radius_mm,
            "frequency_shift_percent": shift_percent,
            "new_frequency_GHz": f_bent
        }

if __name__ == "__main__":

    ant = PatchAntennaCalculator(
        freq_ghz=2.45,
        er=3.5,
        h_mm=0.5
    )

    dims = ant.design_rectangular()

    for k, v in dims.items():
        print(f"{k:20s}: {v:.3f}" if isinstance(v, float) else f"{k}: {v}")
    