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
├── assets/
│   ├── portfolio-deck.pdf
│   ├── projects/
│   │   ├── engineering/
│   │   │   ├── alleviate/     ← kinematics + Monte Carlo outputs
│   │   │   ├── amr/           ← Falali media, diagrams, PCB and PDR files
│   │   │   ├── antenna/       ← S11 result
│   │   │   ├── ml/            ← CaDDE ML report + SEM sample
│   │   │   └── modelling/     ← parachute experiment report + cover
│   │   └── finance/           ← sample-data tracker preview only
│   └── recognition/           ← award certificates
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
- **Alleviate** uses the two verified technical outputs available in the workspace. No unsupported prototype photography was added.
- **Origaminah** includes the additional battery, wiring, motion-sensor and structure visuals found in the DTI workspace.
- **Parametric Computational Design** was removed at the user's request.
- The previous KiCad wheel schematic was removed from the CaDDE and finance projects; it belonged to the AMR work.
- The finance section uses an illustrative dashboard image only. It does not publish live account data or holdings.

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

The workspace contains additional material that is not yet represented as public site content, including detailed AMR test videos, the full antenna macro set, global-programme documents and further finance-research artifacts. Add these only after checking accuracy, privacy and whether they strengthen the intended engineering profile.
