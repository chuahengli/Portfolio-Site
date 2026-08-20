import os
import streamlit as st
import numpy as np
import matplotlib.pyplot as plt
from antenna_calculator import PatchAntennaCalculator
from cst_controller import CSTController

st.set_page_config(page_title="Flexible Patch Antenna Designer")
st.title("📡 Flexible Patch Antenna Designer for Sports KT")

with st.sidebar:
    freq = st.slider("Frequency (GHz)", 1.0, 6.0, 2.45)
    er = st.slider("Permittivity", 2.0, 10.0, 3.5)
    h = st.slider("Substrate Thickness (mm)", 0.2, 5.0, 1.8)
    bend_R = st.slider("Bending Radius (mm)", 30, 200, 90)
    patch_type = st.selectbox("Patch Topology",
                              ["Rectangular", "Slotted", "E-shaped", "CPW-fed", "Flexible bent"])

    if patch_type == "Slotted":
        slot_len = st.slider("Slot length (mm)", 5, 20, 10)
        slot_width = st.slider("Slot width (mm)", 0.5, 3.0, 1.0)
        slot_off = st.slider("Slot offset from centre (mm)", 2, 15, 5)
    elif patch_type == "E-shaped":
        slot_len = st.slider("Slot length (mm)", 5, 20, 10)
        slot_width = st.slider("Slot width (mm)", 0.5, 3.0, 1.0)
        slot_spacing = st.slider("Slot spacing (mm)", 2, 15, 6)
    elif patch_type == "CPW-fed":
        cpw_gap = st.slider("CPW gap (mm)", 0.1, 1.0, 0.2)
        cpw_strip = st.slider("CPW strip width (mm)", 1.0, 4.0, 2.0)

calc = PatchAntennaCalculator(freq_ghz=freq, er=er, h_mm=h)

if patch_type == "Rectangular":
    dims = calc.design_rectangular()
elif patch_type == "Slotted":
    dims = calc.design_slotted(slot_len, slot_width, slot_off)
elif patch_type == "E-shaped":
    dims = calc.design_eshaped(slot_len, slot_width, slot_spacing)
elif patch_type == "CPW-fed":
    dims = calc.design_cpwfed(cpw_gap, cpw_strip)
else:
    dims = calc.design_rectangular()

st.subheader("Dimensions")
col1, col2, col3, col4 = st.columns(4)
col1.metric("Patch Width", f"{dims['W_mm']:.2f} mm")
col2.metric("Patch Length", f"{dims['L_mm']:.2f} mm")
col3.metric("Feed Width" if patch_type != "CPW-fed" else "CPW strip width",
            f"{dims.get('Wf_mm', dims.get('cpw_strip_mm', 0)):.2f} mm")
col4.metric("Feed Inset" if patch_type != "CPW-fed" else "CPW gap",
            f"{dims.get('y0_mm', dims.get('cpw_gap_mm', 0)):.2f} mm")

if patch_type in ["Slotted", "E-shaped"]:
    st.write(f"Slot length: {dims['slot_length_mm']:.1f} mm, width: {dims['slot_width_mm']:.1f} mm")

shift_pct, f_bent = calc.bending_shift(bend_R, dims["L_mm"])
if patch_type == "Flexible bent":
    st.write(f"**Bending effect:** Frequency shift = {shift_pct:.2f}% → new resonance ≈ {f_bent:.2f} GHz")
else:
    st.write(f"Predicted frequency shift if bent (R={bend_R} mm): {shift_pct:.2f}%")

freqs = np.linspace(freq-0.8, freq+0.8, 500)
s11, bw = calc.estimate_s11_curve(freq, freqs, dims["er_eff"], dims["W_mm"])
fig, ax = plt.subplots()
ax.plot(freqs, s11)
ax.axhline(-10, linestyle="--", color='r', label="-10 dB threshold")
ax.set_xlabel("Frequency (GHz)")
ax.set_ylabel("S11 (dB)")
ax.set_title(f"Predicted S11 — est. BW: {bw*1000:.0f} MHz")
ax.legend()
ax.grid()
st.pyplot(fig)

if st.button("Generate CST Model"):
    cst = CSTController()
    cst_type = patch_type if patch_type != "Flexible bent" else "Rectangular"

    try:
        if cst_type == "Rectangular":
            cst.build_patch_antenna(
                patch_type=cst_type,
                patch_W=dims["W_mm"], patch_L=dims["L_mm"], substrate_h=h,
                feed_W=dims["Wf_mm"], inset_dist=dims["y0_mm"], center_freq=freq
            )
        elif cst_type == "Slotted":
            cst.build_patch_antenna(
                patch_type=cst_type,
                patch_W=dims["W_mm"], patch_L=dims["L_mm"], substrate_h=h,
                feed_W=dims["Wf_mm"], inset_dist=dims["y0_mm"], center_freq=freq,
                slot_len_mm=dims["slot_length_mm"], slot_width_mm=dims["slot_width_mm"],
                slot_offset_mm=dims["slot_offset_mm"]
            )
        elif cst_type == "E-shaped":
            cst.build_patch_antenna(
                patch_type=cst_type,
                patch_W=dims["W_mm"], patch_L=dims["L_mm"], substrate_h=h,
                feed_W=dims["Wf_mm"], inset_dist=dims["y0_mm"], center_freq=freq,
                slot_len_mm=dims["slot_length_mm"], slot_width_mm=dims["slot_width_mm"],
                slot_spacing_mm=dims["slot_spacing_mm"]
            )
        elif cst_type == "CPW-fed":
            cst.build_patch_antenna(
                patch_type=cst_type,
                patch_W=dims["W_mm"], patch_L=dims["L_mm"], substrate_h=h,
                cpw_gap_mm=dims["cpw_gap_mm"], cpw_strip_mm=dims["cpw_strip_mm"],
                center_freq=freq
            )

        # --- Discrete port: P1 (ground, z=0) → P2 (feed copper top, z=h+0.035) ---
        # Both points share the same XY centre of the feed line bottom tip.
        feed_W  = dims.get('Wf_mm', dims.get('cpw_strip_mm', 1.0))
        patch_L = dims['L_mm']
        feed_L  = 15.0
        x_ctr   = 0.0                           # feed is centred on X=0
        y_ctr   = -patch_L / 2 - feed_L + 0.5  # 0.5 mm inside feed bottom tip
        zmin    = -0.035                         # P1: bottom of ground plane
        zmax    =  h + 0.035                     # P2: top of copper on substrate

        # Add port BEFORE save
        cst.add_discrete_port(1,
            xmin=x_ctr, xmax=x_ctr,
            ymin=y_ctr, ymax=y_ctr,
            zmin=zmin,  zmax=zmax,
            impedance=50)

        # FIX: Save AFTER port is added
        save_path = r"C:\Users\hengl\Documents\SUTD\Term 5\Electromagnetics & Apps\1D\CST antenna design python automation\AntennaProject\patch245.cst"
        cst.save(save_path)

        # Solver setup and run
        cst.create_solver_setup(
            fmin=freq * 0.8,
            fmax=freq * 1.2,
            max_passes=5
        )
        with st.spinner("Running CST solver — this may take a few minutes…"):
            cst.run_solver()

        # Export results
        base = os.path.splitext(save_path)[0]
        s2p_path = base + '.s2p'
        ff_path  = base + '_farfield.txt'
        cst.export_sparameters(s2p_path)
        cst.export_farfield(ff_path)

        # Validate and display
        try:
            report = validate_s2p(s2p_path, freq, er, h)
            rel_err = report['rel_error_pct']
            color = "green" if abs(rel_err) < 2 else ("orange" if abs(rel_err) < 5 else "red")
            st.success(f"✅ Simulation complete — Resonance: {report['sim_f0_ghz']:.3f} GHz  |  "
                       f"Δ% vs design: {rel_err:.2f}%  |  "
                       f"−10 dB BW: {report['bw_mhz']:.1f} MHz")

            # Show simulated S11 alongside predicted
            fig2, ax2 = plt.subplots()
            sim_freqs_ghz = report['freqs_hz'] / 1e9
            ax2.plot(sim_freqs_ghz, report['s11_db_trace'], label="CST simulated", color="blue")
            ax2.plot(freqs, s11, label="Analytical estimate", color="orange", linestyle="--")
            ax2.axhline(-10, linestyle=":", color='r', label="-10 dB")
            ax2.axvline(report['sim_f0_ghz'], linestyle=":", color='blue', alpha=0.5)
            ax2.axvline(freq, linestyle=":", color='orange', alpha=0.5)
            ax2.set_xlabel("Frequency (GHz)")
            ax2.set_ylabel("S11 (dB)")
            ax2.set_title("Simulated vs Predicted S11")
            ax2.legend()
            ax2.grid()
            st.pyplot(fig2)

            with st.expander("Full validation report"):
                st.json({k: v for k, v in report.items()
                         if k not in ('s11_db_trace', 'freqs_hz')})
        except Exception as e_val:
            st.warning(f"Project saved & solver ran, but validation failed: {e_val}")

    except Exception as e:
        st.error(f"CST generation failed: {e}")