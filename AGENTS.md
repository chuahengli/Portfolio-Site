# AGENTS.md — Portfolio Site (Chua Heng Li)

This file is read automatically by Hermes at the start of every session that
touches this folder. It carries the design spec and house rules for the site.
EDIT IT when you want to change the design direction — that is the point of
this file. Keep it plain, factual, and current. AGENTS.md is the source of
truth: when a design/edit decision changes, this file is updated to match.

## Project in one line
Single-file, framework-free, GitHub-Pages portfolio for Chua Heng Li (SUTD EPD,
electrical engineering + AI minor). No build step. Live:
https://chuahengli.github.io/Portfolio-Site/

## Ownership of edits (user rule, non-negotiable)
- The agent builds/edits everything and hands over precise push steps.
- The USER does all git commits and pushes. Agent never commits/pushes.
- Agent's GitHub token is Contents read-only (403 on writes) — commit attempts
  will fail by design. Never ask for user credentials.
- Repo: chuahengli.github.io (browser push path documented in private/docs/DEPLOY-GITHUB.md).

## Identity facts (verified, do not change without user confirmation)
- GitHub: github.com/chuahengli
- Email: chuahengli@gmail.com
- LinkedIn: linkedin.com/in/heng-li-chua (verified Aug-2026; the resume
  contains the older in/henglichua handle — the site must use the verified one)
- Site title: "Chua Heng Li - Engineering Product Development Portfolio"

## Design spec (current, user-approved Aug-2026)
Direction: humble, simple, not show-offy; refined dark editorial. The site
exists to present projects, not to wow with frontend design. User rejected
"vibe coded" styling; later tweaks (hero media tiles) kept — visual polish
always stays restrained.

Hard rules:
- NO ALL-CAPS WORDING, EVER. No `text-transform:uppercase` in CSS, no
  full-caps copy in the HTML. Every label renders in normal case
  ("Selected projects", "Recognition", "Engineering", "Data & Finance").
- Palette "Matte Circuit" (premium matte-PCB look): bg dark charcoal #121214,
  panel dark slate #1E2025, hairline #2B2E36, text soft silver #E5E7EB (muted
  #9CA3AF, faint #6B7280). SINGLE accent: circuit gold #D4AF37 — used ONLY for
  interactive states (links, focus rings, button hover/resume border) and
  selection tint. Everything else stays neutral — dates, labels, tags are
  silver/grey. Never introduce a second hue; gold is the only colour.
- Skills panel rows: grid with fixed 180px label column (labels 14px muted,
  `white-space:nowrap` so they never wrap), values column 16px fg. Values are
  ONE flowing inline text line with LITERAL " · " separators — NOT flex-wrapped
  spans and NOT css-pseudo dots; this is the ONLY layout that guarantees no
  stray leading dots and even spacing when a long row (e.g. Hardware & Design)
  wraps. Row vertical padding 11px, `align-items:start`, rows separated by
  `.hs + .hs` hairline (no border under the title; the title carries its own
  bottom hairline + 14px padding).
- Single system font family: Arial / Segoe UI / Helvetica Neue fallbacks.
  No webfonts, no Google Fonts, no external font requests.
- Whole-pixel type scale only: 36, 44, 48, 56 (headings) and 12, 14, 16,
  18, 22, 24 (everything else). Minimum 12px. No rem, no clamp(), no
  fractional sizes, no monospace overrides. This renders identically on
  Windows / macOS / Linux — CSS px is device-independent (1/96in); layout
  adapts via media queries at 900 / 700 / 520px.
- No gradients, no glows, no glass blur, no shadows, no hover transforms /
  scale animations. Flat panels (#11171d) on near-black (#0d1116), 1px
  hairline borders, small stable radii (6px buttons, 8-12px media/cards).
  Hover = border colour change + (on links) underline only.
- No partial-bold inside paragraphs — plain, even-weight copy.
- Concise sentences: each project = what it is, plus links to real evidence.
- Facts/date inline with titles; recognition as quiet cards, not "1st prize!"
  shouting.

## Page structure (current, intentional)
- Sticky plain nav: name left, two links (Projects, Recognition).
- Hero: TWO-COLUMN grid. Left column: name (48px) + one summary paragraph
  + GitHub / LinkedIn / Resume buttons (inline-SVG brand marks, never asset
  files). Right column: skills panel (`hero-skills`) — bordered card with a
  "Skills" title (16px semibold, underlined by a hairline) at top, then
  aligned rows: label column fixed 180px (14px muted, nowrap), values as ONE
  flowing line with literal " · " separators (never flex-spans/pseudo-dots —
  that caused ragged dots and stray leading-circle on wrapped rows), top-
  aligned (`align-items:start`), 11px row padding, hairline between rows only.
  NO kicker/eyebrow line above the name. NO hero image tiles.
- Sections: "Projects" (heading 36px; group titles Engineering / Data & Finance
  24px, full-brightness fg) and "Recognition" (certificate cards). NO
  section-intro paragraphs anywhere — user removed them as unneeded filler.
- Type scale (whole px): headings 48px hero, 36px section, 24px group title,
  16px body. Minimum 12px.
- NO dedicated Education/Experience sections — user decision (Aug-2026): the
  resume PDF is one click away in the hero, and the hero summary already
  covers SUTD/EPD/AI. Do NOT re-add them unless the user asks; if asked,
  add a single quiet text-only block (label/value rows), not cards.

## Pending / optional
- Gallery/media assets: user curates; agent stages into assets/ subfolders.
- If the user changes their mind about Education/Experience on-page, update
  the "Page structure" section above at the same time.

## Content rules (non-negotiable)
- Privacy-first: never publish real financial holdings / live portfolio data.
  Sample-data previews must be labelled "illustrative sample data". A live demo
  is only offerable "on request", never embedded.
- Facts trace to source files: exact numbers, dates, names. Never invent.
  Wrong details were caught once before delivery — always re-verify.
- Equity-research deep-dives are AI-assisted (user's methodology + questions,
  tool-assisted scanning). Frame publicly as a learning/research program, not
  sole authorship. Label forecast provenance (management guidance / analyst
  consensus / own modelling).
- No people-photos / group shots on the public site. Flag them to the user
  first (they are held in private/people-photos/).
- Team projects: emphasise the user's own subsystem/contribution, not the
  whole team effort.

## Site structure
- index.html — the ENTIRE site, self-contained (single <style> block,
  inline-SVG icons). Body markup is content; keep SVG path data untouched
  (never hand-type paths — rebuild programmatically if they must change).
- assets/ — projects/, recognition/ (certificates), resume/ (PDF only).
  Relative paths only. GitHub-Pages-safe.
- private/ — local-only, gitignored, NEVER pushed: docs/, people-photos/.
- Validated source of truth for site content only.

## Resume convention
- User converts own docx -> PDF and drops it in the parent folder
  (/workspace/Portfolio Related/). Agent only copies the PDF into
  assets/resume/ and updates the href. Never run a converter.

## Validation before delivery (required after every edit)
1. Python HTMLParser: balanced tags, no mismatches.
2. Regex every assets/... ref in index.html -> file must exist
   (URL-encoded refs like %20 decode first).
3. Serve folder with `python3 -m http.server` and curl index + sample of each
   asset kind (png, jpg, pdf, mp4, docx, html, cst, bas, zip) for 200.
   Server must run from the site folder root.
4. Visual check (or user eyeball) for broken images / leaked private info.
5. Grep: zero `text-transform` occurrences (all-caps ban).

## Reference docs (human-readable)
- README.md — public-facing repo introduction.
- private/docs/SITE-README.md — structure + editing guide.
- private/docs/DEPLOY-GITHUB.md — user-side push steps (browser path + git path).
- private/docs/ASSET-HARVEST-MANIFEST.md — where media came from.