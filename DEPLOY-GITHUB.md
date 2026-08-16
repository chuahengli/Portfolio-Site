# 🚀 Put Your Portfolio Online (GitHub Pages) — Step-by-Step

Everything in the `Portfolio Site/` folder is ready to go. You just need to get it onto GitHub.
Pick **Option A** (web browser, no coding) or **Option B** (command line, if you're comfortable with git).

Your free public URL will be:

> **https://chuahengli.github.io**

---

## Option A — Web browser only (easiest, ~5 minutes)

### Step 1 — Create the repository
1. Go to https://github.com and sign in (create an account if you don't have one — use the same email as your LinkedIn, `chuahengli@gmail.com`).
2. Click the **green "+" button** (top-right) → **"New repository"**.
3. **Repository name:** type exactly `chuahengli.github.io` ← this exact name gives you the cleanest URL.
   > (If you'd rather not use that name, any name works — your URL just becomes `https://chuahengli.github.io/<name>/`. If that's the case, tell me and I'll tweak the links in `index.html`.)
4. Set it to **Public** (required — GitHub Pages doesn't publish private repos on the free plan).
5. Leave everything else as-is. Click **"Create repository"**.

### Step 2 — Upload the files
1. On your new (empty) repository page, click **"uploading an existing file"** (under "Quick setup").
2. Open the folder `Portfolio Related/Portfolio Site/` on your computer and **drag the whole contents into the browser window** — that means:
   - `index.html` (the site itself)
   - the `assets/` folder **as a folder** (drag it in whole so GitHub keeps the categorized project structure)
   - `DEPLOY-GITHUB.md`, `SITE-README.md` and `.gitignore` (optional documentation/configuration)
3. Scroll down → **"Commit changes"** → **"Commit directly to the main branch"** → **Commit**.

### Step 3 — Turn on Pages
1. Go to the repository **Settings** tab (top of the repo page).
2. In the left sidebar, click **"Pages"**.
3. Under **"Build and deployment"** → **Source**, choose **"Deploy from a branch"**.
4. Branch: **main** → folder: **/ (root)** → **Save**.
5. Wait ~1 minute. Refresh the page — GitHub will show you the URL (usually `https://chuahengli.github.io`).

### Step 4 — Verify
- Open **https://chuahengli.github.io** in a fresh/incognito window.
- You should see the full page with your name, projects, and images.
- Hard-refresh (Ctrl+Shift+R) if it looks stale after updates.

---

## Option B — Command line (git)

If you prefer git, run these in a terminal **inside the `Portfolio Site/` folder** after creating the repo as in Step 1:

```bash
cd "Portfolio Related/Portfolio Site"

git init
git add -A
git commit -m "Initial portfolio site"
git branch -M main
git remote add origin https://github.com/chuahengli/chuahengli.github.io.git
git push -u origin main
```

Then follow **Step 3** above to enable Pages.

---

## ✅ Checklist after it's live

- [ ] `https://chuahengli.github.io` loads with no 404s
- [ ] Click each project card image — they load (if any shows broken, the file wasn't uploaded; re-upload `assets/`)
- [ ] "Portfolio deck (PDF)" link downloads your deck PDF
- [ ] GitHub icon links to `github.com/chuahengli` — correct
- [ ] LinkedIn link works — **⚠️ ACTION NEEDED**: your resume says `linkedin.com/in/henglichua` but the portfolio deck says `linkedin.com/in/heng-li-chua`. I used the deck's version. Open both once in your browser and confirm which is your real profile, then fix the other one — or tell me and I'll update the site.

## 🐛 Troubleshooting

| Problem | Fix |
|---|---|
| Page 404s | File must be named exactly `index.html` (case-sensitive). Check you uploaded it at the repo root, not inside a subfolder. |
| Images broken | The `assets/` folder must be uploaded **as a folder** at the repo root, next to `index.html`. |
| Stale page | GitHub Pages caches ~1 min; hard-refresh with Ctrl+Shift+R. |
| `chuahengli.github.io` is someone else's | That exact name must be unique on GitHub; if taken, use a project repo name and update links. |
| Page loads but looks unstyled | The Google Fonts link needs internet (fine in a browser); everything else is fully self-contained. |

## ✨ Optional upgrades (whenever you want)

1. **Add your photo** — drop a `photo.jpg` into `assets/` and uncomment the `<img id="photo" src="assets/photo.jpg">` line in `index.html` (the avatar currently shows your initials).
2. **Custom domain** — buy e.g. `hengli.dev` and set it under Pages → Custom domain (needs a CNAME at your registrar). Happy to guide you.
3. **Swap or add project images** — the current version already includes the actual 30.007 prototype photo, laser-cutting photo, PCB/schematic visuals and clamp demo. You can add further project photos or videos after checking that they do not expose teammates' faces, private information or unfinished material.
4. **Live dashboard demo** — when you want to show the dashboard *live*, we can deploy it on Streamlit Community Cloud with a fake/sanitised dataset, then embed it. Privacy-first version only, per your call.
5. **GitHub link in footer** — the footer links to `github.com/chuahengli`. If you name the repo something other than `chuahengli.github.io`, send me the name and I'll point the "Stock Tracker repo →" link correctly (it already points at `Stock-Portfolio-project`).

---

*Tip: keep the folder structure exactly as-is (`index.html` + `assets/` side by side) so future edits and re-uploads are trivial.*
