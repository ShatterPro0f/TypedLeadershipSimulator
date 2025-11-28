# Project Organization Policy

## 📁 Documentation Location Standard

**RULE: All documentation files (.md, .txt) must be stored in the `docs/` folder. Root folder should only contain `README.md`.**

### Rationale
- **Cleaner project structure**: Keep root focused on build files, source, and tests
- **Easier navigation**: All user-facing documentation in one consistent location
- **Better organization**: Separate concerns (code vs. documentation)
- **Git enforcement**: `.gitignore` prevents accidental root-level doc creation

### Current Structure

```
TypedLeadershipSimulator/
├── README.md                          ← ONLY .md file in root
├── src/                               ← Source code
├── tests/                             ← Test code
├── build/                             ← Build output
├── docs/                              ← ALL DOCUMENTATION
│   ├── START_HERE.md                  ← Navigation hub
│   ├── DOCUMENTATION_INDEX.md         ← Complete index
│   ├── README.md                      ← Duplicate reference file
│   ├── PHASE2_SUMMARY.md              ← Completion summary
│   ├── PHASE2_IMPLEMENTATION.md       ← Architecture (30+ sections)
│   ├── IMPLEMENTATION_NOTES.md        ← Technical details
│   ├── QUICKREF.md                    ← Code examples
│   ├── COMPLETION_REPORT.md           ← Phase 2 wrap-up
│   ├── FILE_INVENTORY.md              ← All files listed
│   ├── PHASE2_CHECKLIST.md            ← Completion checklist
│   ├── PHASE2_FINAL.md                ← Final status
│   ├── PHASE2_VISUAL_SUMMARY.txt      ← Visual overview
│   ├── API_REFERENCE.md               ← Class/function docs
│   ├── PHASE1_REFERENCE.md            ← Phase 1 features
│   ├── HOW_TO_RUN_TESTS.md            ← Test instructions
│   ├── TESTING_GUIDE.md               ← QA guide
│   ├── GamePlan.md                    ← Game planning docs
│   └── ImplementationChecklist.md     ← Dev checklist
├── .gitignore                         ← Prevents root .md files
├── CMakeLists.txt                     ← Build configuration
└── .github/
    └── copilot-instructions.md        ← Dev guidelines
```

### Cross-Reference Guidelines

**From root files to docs:**
```markdown
<!-- Good: Points to docs folder -->
See [PHASE2_SUMMARY.md](docs/PHASE2_SUMMARY.md) for details.
```

**From docs files to root:**
```markdown
<!-- Good: Points back to root with .. -->
See [README.md](../README.md) for project overview.
```

**From docs files to other docs files:**
```markdown
<!-- Good: Local reference within same folder -->
See [QUICKREF.md](QUICKREF.md) for code examples.
```

### Getting Started

1. **First time?** → Start with [`docs/START_HERE.md`](START_HERE.md)
2. **Quick reference?** → See [`docs/QUICKREF.md`](QUICKREF.md)
3. **Architecture deep dive?** → Read [`docs/PHASE2_IMPLEMENTATION.md`](PHASE2_IMPLEMENTATION.md)
4. **Project overview?** → Check [`README.md`](../README.md)

### Git Enforcement

The `.gitignore` file includes this rule:

```gitignore
# Documentation policy: All .md files must go in docs/ folder
# Root-level markdown files are not allowed (documentation goes in docs/)
/*.md
!README.md
```

This prevents accidentally committing documentation to the root folder. If you try to add a `.md` file to root:

```bash
git add SOME_FILE.md  # This will be ignored by git
git status            # File won't appear in staging area
```

### Creating New Documentation

When creating new documentation:

1. **Always create in `docs/` folder**
   ```bash
   # Correct
   touch docs/NEW_GUIDE.md
   
   # Wrong - will be ignored
   touch NEW_GUIDE.md
   ```

2. **Update `DOCUMENTATION_INDEX.md`** to include your new file

3. **Link appropriately**:
   - If in `docs/`, use relative paths: `[File](OTHER_FILE.md)`
   - If linking from root, use: `[File](docs/OTHER_FILE.md)`
   - If linking from docs to root, use: `[File](../README.md)`

### Workspace Setup

All developer workspaces should follow this standard:

- **Documentation author?** Place files in `docs/`
- **Contributing code?** Keep `.md` out of root
- **CI/CD pipelines?** Verify no root-level `.md` files in commits

---

## 📋 Current Documentation (17 Files)

| File | Purpose | Status |
|------|---------|--------|
| `START_HERE.md` | Navigation hub for all documentation | ✅ Complete |
| `DOCUMENTATION_INDEX.md` | Complete index of all docs | ✅ Complete |
| `README.md` | Project overview (also in root) | ✅ Complete |
| `PHASE2_SUMMARY.md` | Phase 2 completion summary | ✅ Complete |
| `PHASE2_IMPLEMENTATION.md` | Comprehensive 30+ section architecture guide | ✅ Complete |
| `IMPLEMENTATION_NOTES.md` | Technical implementation details | ✅ Complete |
| `QUICKREF.md` | Code examples for common tasks | ✅ Complete |
| `COMPLETION_REPORT.md` | Phase 2 wrap-up and recommendations | ✅ Complete |
| `FILE_INVENTORY.md` | Complete inventory of all source files | ✅ Complete |
| `PHASE2_CHECKLIST.md` | Phase 2 completion checklist | ✅ Complete |
| `PHASE2_FINAL.md` | Final Phase 2 status | ✅ Complete |
| `PHASE2_VISUAL_SUMMARY.txt` | Visual summary of Phase 2 | ✅ Complete |
| `API_REFERENCE.md` | Class and function documentation | ✅ Complete |
| `PHASE1_REFERENCE.md` | Phase 1 features reference | ✅ Complete |
| `HOW_TO_RUN_TESTS.md` | Test execution instructions | ✅ Complete |
| `TESTING_GUIDE.md` | QA and testing guide | ✅ Complete |
| `GamePlan.md` | Game planning documentation | ✅ Complete |
| `ImplementationChecklist.md` | Developer implementation checklist | ✅ Complete |

---

## 🔗 Quick Links

- **Start:** [`START_HERE.md`](START_HERE.md)
- **Index:** [`DOCUMENTATION_INDEX.md`](DOCUMENTATION_INDEX.md)  
- **Architecture:** [`PHASE2_IMPLEMENTATION.md`](PHASE2_IMPLEMENTATION.md)
- **Quick Ref:** [`QUICKREF.md`](QUICKREF.md)
- **Testing:** [`HOW_TO_RUN_TESTS.md`](HOW_TO_RUN_TESTS.md)

---

**Last Updated:** Phase 2 Completion
**Total Documentation:** 17 Files, 200+ Pages
**Organization Level:** ✅ Standardized
