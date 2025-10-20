# Glint3D Documentation

## Quick Navigation

- **[Documentation Index](INDEX.md)** - Complete guide to all documentation by role/topic
- **[Getting Started](ONBOARDING.md)** - New developer onboarding
- **[Product Vision](PRODUCT_VISION.md)** - Market positioning and roadmap
- **[Task Modules](TASKS.md)** - Auto-generated task system documentation

---

## Generating Documentation

### Local Generation

**Prerequisites:**
- Doxygen 1.9.0+ ([download](https://www.doxygen.nl/download.html))
- Graphviz (optional, for diagrams)
- Python 3.x

**Generate:**
```bash
# Windows
docs\generate-docs.bat

# Linux/macOS
./docs/generate-docs.sh
```

Output: `docs/api/html/index.html`

### GitHub Pages (Auto-Deploy)

Documentation automatically deploys to GitHub Pages when you push to `main`.

**One-time setup:**
1. Go to **Settings** → **Pages**
2. Set **Source** to: `GitHub Actions`
3. Push changes to trigger deployment

**Live URL:** https://ahoqp1.github.io/Glint3D/

**Manual trigger:**
- Go to **Actions** → **Deploy Documentation** → **Run workflow**

---

## What's Published

- ✅ API documentation (all public headers)
- ✅ Task module system docs
- ✅ Architecture guides
- ✅ Code examples and tutorials

---

## Customization

**Doxygen config:** Edit `Doxyfile` (project root)
**Layout/navigation:** Edit `docs/DoxygenLayout.xml`
**Task docs:** Auto-generated via `python tools/generate-task-docs.py`

---

## Files in This Directory

```
docs/
├── README.md                 # This file (GitHub Pages setup)
├── INDEX.md                  # Documentation index by role/topic
├── DoxygenLayout.xml         # Doxygen navigation customization
├── TASKS.md                  # Auto-generated task documentation
├── ONBOARDING.md             # Developer onboarding guide
├── PRODUCT_VISION.md         # Market strategy
├── ARCHITECTURE_NOTES.md     # Technical decisions
├── generate-docs.bat         # Windows doc generator
├── generate-docs.sh          # Linux/macOS doc generator
└── api/                      # Generated output (gitignored)
    └── html/                 # Doxygen HTML output
```

---

**Auto-generated docs are NOT committed to the repo** - they're built on-demand and deployed via GitHub Actions.
