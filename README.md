# Heng Li — Engineering Portfolio

Single-page static portfolio for **Heng Li (Chua)**, an undergraduate Engineering Product Development (EPD) student at [SUTD](https://www.sutd.edu.sg) with an electrical engineering focus and an AI minor.

**Live site:** https://chuahengli.github.io/Portfolio-Site/

---

## Overview

A lightweight, fast, framework-free personal site that presents engineering work:

1. **Engineering projects** — embedded firmware (ESP32-S3/Arduino), electronics prototyping, custom KiCad PCB design, engineering analysis, machine learning and electromagnetics/antenna work.
2. **Python, data & equity research** — the portfolio tracker and a structured quantitative research programme.
3. **Recognition** — certificates and competition results. Career detail lives on the résumé (linked from the hero).

Every project card links to real, first-party evidence: schematics, PCB layouts, source code zips, demo videos, interactive HTML models, reports, certificates and decks.

---

## Tech

- **Static site** — a single `index.html` plus an `assets/` folder. No build step, no frameworks, no server-side code.
- **GitHub Pages** renders it directly from the repo (deploy from `main` at the root `/`).
- Plain dark theme via CSS custom properties (`:root`): near-black background, single quiet accent for links, sharp corners and thin hairlines. No framework, no animation, no external fonts — a straightforward, text-first presentation.
- Everything is **privacy-first**: no financial holdings, live portfolio data, or secrets are published. Financial previews are labelled as illustrative sample data, and no unverified group photos appear on the site.

---

## Repository structure

```
Portfolio-Site/
├── index.html              ← the entire site (self-contained)
├── README.md
├── assets/
│   ├── projects/
│   │   ├── engineering/
│   │   │   ├── alleviate/        ← kinematics + Monte Carlo + interactive HTML models + milestone decks
│   │   │   ├── amr/              ← Falali media, diagrams, firmware, PCB and PDR files
│   │   │   ├── antenna/          ← S11 + EM-1D deliverables + CST automation scripts
│   │   │   ├── ml/               ← project ML report + sample data
│   │   │   └── modelling/        ← parachute experiment report
│   │   ├── finance/              ← sample-data tracker preview (illustrative)
│   │   └── global/               ← GEO Mahidol case study + charts
│   └── recognition/              ← shareable certificates
├── private/                ← local-only; gitignored (never pushed)
└── .gitignore
```

---

## Run locally

```bash
# Serve the folder as a static site (no install needed)
python3 -m http.server 8000
# then open http://localhost:8000
```

Or just double-click `index.html`.

---

## Customising

- **Add or replace a project** — drop files into the matching folder under
  `assets/projects/...`, then add or edit the relevant `div.project` block
  in `index.html`.
- **Add a photo** — the hero currently has no avatar; add a `<img>` in the
  hero section if you want one.
- **Update links/contact** — the footer and contact section link to [github.com/chuahengli](https://github.com/chuahengli) and `linkedin.com/in/heng-li-chua` (verified).

Live edits should be **validated** before publishing:

1. Check HTML tag balance (a simple parser will flag unclosed/mismatched tags).
2. Confirm every `assets/...` reference in `index.html` resolves to an existing file.
3. Serve the folder locally and request the index plus each referenced asset.
4. Visually inspect the page for broken images, clipping or leaked private information.

---

## Deploying to GitHub Pages

Changes go live by pushing to `main`:

```bash
git add -A
git commit -m "describe your change"
git push
```

GitHub Pages rebuilds from the branch automatically, usually within ~1 minute.

Deployment is configured under **Settings → Pages → Build and deployment → Deploy from a branch → `main` / root (/)**. No build step beyond that; the push alone is enough.

> Only the site itself, `README.md`, `.gitignore` and `assets/` are for publication. Anything you do not want public goes in `private/` and is ignored by git.

---

## License

Project content and files belong to the author unless otherwise credited. Contact via [GitHub](https://github.com/chuahengli) or [LinkedIn](https://linkedin.com/in/heng-li-chua).