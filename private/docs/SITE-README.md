# Portfolio Site — README

Single-page static portfolio for **Chua Heng Li**, an Engineering Product Development student at SUTD specialising in electrical engineering with an AI minor.

The site is organised around three clear areas:

1. **Engineering projects** — embedded firmware, electronics prototyping, PCB design, engineering analysis, machine learning and electromagnetics.
2. **Python, data & equity research** — the portfolio tracker and the broader structured research program.
3. **Experience & education** — SUTD, UOB Kay Hian, leadership, awards and technical toolkit.

## Folder structure

```
Portfolio Site/
├── index.html                  ← complete self-contained site
├── ASSET-HARVEST-MANIFEST.md   ← full-workspace harvest log (2026-08-20)
├── assets/
│   ├── portfolio-deck.pdf
│   ├── projects/
│   │   ├── engineering/
│   │   │   ├── alleviate/     ← kinematics + Monte Carlo + interactive models + milestone decks
│   │   │   ├── amr/           ← Falali media, diagrams, firmware, PCB and PDR files
│   │   │   ├── antenna/       ← S11 + EM-1D deliverables + CST automation scripts
│   │   │   ├── c-bracket/     ← CAD → 3D print → tensile test package
│   │   │   ├── ml/            ← CaDDE ML report + SEM sample
│   │   │   ├── modelling/     ← parachute experiment report + cover
│   │   │   └── underwater-comms/ ← technical presentation + script-generated diagrams
│   │   ├── finance/           ← sample-data tracker preview only
│   │   └── global/            ← GEO Mahidol case study + charts
│   └── recognition/           ← award certificates (4)
├── private/                   ← gitignored; people-photos + anything not for the public site
├── DEPLOY-GITHUB.md
├── SITE-README.md
└── .gitignore
```

## How it works

- **No build step.** `index.html` + `assets/` is all GitHub Pages needs.
- **No frameworks or local dependencies.** Google Fonts are optional; the site falls back to system fonts.
- The descriptions are based on source files in the workspace. Claims should be rechecked against source material when projects change.
- Private financial holdings, databases, credentials and unverified group photos are not published.

## Content decisions

- The opening is deliberately direct: name, degree, specialisation, minor and working areas. It does not use a broad slogan.
- **Falali — One AMR Fits All** is the team project title. The site separately identifies the arm-subsystem PCB, electronics and firmware contributions.
- **Alleviate** uses the two verified technical outputs available in the workspace (Monte Carlo + kinematics) plus the interactive HTML models and compressed milestone decks; the "Heng Li Present Portion" deck is linked separately so the site never over-claims ownership.
- **Origaminah** includes the additional battery, wiring, motion-sensor and structure visuals found in the DTI workspace.
- **Parametric Computational Design** was removed at the user's request.
- The previous KiCad wheel schematic was removed from the CaDDE and finance projects; it belonged to the AMR work.
- The finance section uses an illustrative dashboard image only. It does not publish live account data or holdings.
- **C-Bracket Reimagined** and **Underwater Wireless Communications** were added from the Aug-2026 full-workspace harvest; every claim traces to files now in `assets/`.
- **CodeChamps 2024** appears in the Recognition grid explicitly labelled "Certificate of participation" — it is participation, not a prize, and is not presented as an award.
- **GEO Mahidol** appears as a leadership/programmes timeline entry with the case-study PDF linked; acceptance-level claims about the Waterloo GEXP offer are deliberately not published until the user confirms.
- **People-photos** (KL trip, EM-prof, FabLab, group shots) are staged in gitignored `private/` and are not on the public site — they await the user's explicit approval.
- Full provenance of every staged or excluded file: see `ASSET-HARVEST-MANIFEST.md`.

## Editing tips

- Add or replace a project inside its matching `assets/projects/...` folder, then update the relevant `<article class="project">` in `index.html`.
- Keep media in the project-specific directory so technical files cannot be accidentally reused in the wrong project.
- The verified LinkedIn URL is `linkedin.com/in/heng-li-chua`.

## Re-deploying after edits

```bash
git add -A
git commit -m "Describe the update"
git push
```

GitHub Pages normally updates within about a minute.

## Validation

Before publishing a substantial edit:

1. Check HTML tag balance.
2. Check every `assets/...` reference exists.
3. Serve the folder locally with `python3 -m http.server` and request the index plus every referenced asset.
4. Inspect the public page for image crops, video playback and accidental private information.

See `DEPLOY-GITHUB.md` for the first-time GitHub Pages setup.

## Source coverage still worth reviewing later

The August 2026 full-workspace harvest is logged in `ASSET-HARVEST-MANIFEST.md`. Public-safe
engineering and recognition assets are now on the site. Still deliberately held back pending
your call: people-photos (`private/`), the Waterloo GEXP offer letter (acceptance
unconfirmed), scanned JC/O/A-level certs, creative/fashion material (Sartorial Samsara,
Mimpikita), Term 1–3 coursework decks, and the DES teaching-role evidence (role title
pending confirmation). Add any of these only after checking accuracy, privacy and whether
they strengthen the intended engineering profile.
