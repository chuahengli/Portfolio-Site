# Portfolio Site — README

Single-page static portfolio for **Chua Heng Li** (Engineering Product Development @ SUTD, Electrical Track, AI Minor), designed to be hosted on **GitHub Pages** for free.

The site is intentionally framed around **electrical engineering, embedded systems and Python/data engineering**. RF/antenna is one project, not the headline identity. Projects are presented with relatively equal visual weight rather than a single featured project.

## Folder structure

```
Portfolio Site/
├── index.html                  ← the entire site (self-contained HTML + CSS + tiny JS)
├── assets/
│   ├── tracker-sample-preview.png   ← sample-data preview of the Stock Tracker dashboard
│   ├── alleviate-montecarlo.png     ← Monte-Carlo torque model output ('Alleviate' project)
│   ├── antenna-s11.png              ← S11 return-loss plot (2.45 GHz flexible antenna)
│   ├── robot-arm-subsystem.svg      ← arm-subsystem function diagram (EDI robot)
│   ├── robot-system-flow.svg        ← system-level function diagram (spare)
│   ├── bird-main-structure.png      ← bird-deterrent mechanism structure render
│   ├── dti-circuit.png              ← bird-deterrent circuit-side design
│   ├── semi-1st-prize-cert.png      ← SEMI Tech Zoomers 2026 1st Prize certificate
│   ├── apex-one-raft-cert.pdf       ← APEX "One Raft One Nation" certificate (3 Aug 2025)
│   ├── dti-sustainability-cert.pdf  ← DTI Sustainability Practice Prize certificate
│   ├── parachute-cover.jpg          ← modelling/data-analysis project preview
│   ├── parachute-study.pdf          ← modelling/data-analysis project report
│   ├── parametric/                  ← Grasshopper/Python design visuals + deck
│   ├── alleviate/                   ← kinematics output
│   ├── robot/                       ← actual 30.007 prototype, PCB, schematic and demo
│   └── Heng-Li-Portfolio-Deck.pdf   ← downloadable one-page portfolio deck
├── DEPLOY-GITHUB.md             ← step-by-step instructions to publish on GitHub Pages
└── SITE-README.md               ← this file
```

## How it works

- **No build step.** `index.html` + `assets/` is all GitHub Pages needs — upload and go.
- **No frameworks, no dependencies** except Google Fonts (gracefully falls back to system fonts).
- Every project writeup is grounded in the workspace facts (portfolio deck, resume, project files).

## Design decisions (and why)

- **Sample-data dashboard preview instead of a live embed** — your real Moomoo holdings stay private; the preview is clearly labelled as illustrative. A live sanitised demo can be added later if you want.
- **Actual 30.007 project visuals** — the robot card now uses the final prototype, laser-cutting work, arm schematic and wheel schematic from the project workspace, plus linked PCB/PDR PDFs and a clamp demo video.
- **Investing research is a project, not a side-interest footer** — the equity research is AI-assisted; the site frames it honestly as a learning project you're building, not as your sole authorship.
- **Peer-level project grid** — nine projects are shown with similar visual weight: robot, Alleviate, portfolio tracker, bird deterrent, parametric design, antenna simulation, parachute modelling, CaDDE ML and quantitative research toolkit.

## Editing tips

- **Change your photo:** put `photo.jpg` in `assets/`, then in `index.html` change the avatar `<img id="photo" src="" ...>` to `src="assets/photo.jpg"` (and it'll show automatically; the initials fallback disappears).
- **Colors:** all theme colours live in the `:root{ }` block at the top of the `<style>` section.
- **Projects:** each project is an `<article class="project">` — copy one to add another. The current grid gives eight projects peer-level treatment: robot, Alleviate, portfolio tracker, bird deterrent, parametric design, flexible antenna, parachute modelling, and investing/quantitative toolkit.
- **LinkedIn:** the site links to `linkedin.com/in/heng-li-chua` — confirmed correct (Aug 2026). Your resume lists the old `linkedin.com/in/henglichua`; if you ever update the resume, align it to `heng-li-chua`.

## Re-deploying after edits

Simplest: edit the file(s), then re-upload via the GitHub web UI (Add file → Upload files), or `git add -A && git commit -m "update" && git push`. Pages picks up changes in ~1 minute.
