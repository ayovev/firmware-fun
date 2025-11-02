# Contributing to Roast Logger Firmware

## Conventional Commits & Semantic Versioning

This project uses **Conventional Commits** and **Semantic Versioning** to automatically manage releases.

### Commit Message Format

All commits must follow this format:

```
<type>(<scope>): <subject>

<body>

<footer>
```

#### Examples:

```bash
# Feature (triggers MINOR version bump: 1.0.0 → 1.1.0)
feat: add support for MAX31856 thermocouple
feat(wifi): add mDNS discovery support
feat(ota): implement firmware signature verification

# Bug Fix (triggers PATCH version bump: 1.0.0 → 1.0.1)
fix: correct temperature reading offset
fix(serial): handle disconnection gracefully
fix(wifi): retry connection on timeout

# Performance (triggers PATCH version bump)
perf: optimize temperature reading loop
perf(serial): reduce JSON serialization overhead

# Breaking Change (triggers MAJOR version bump: 1.0.0 → 2.0.0)
feat!: change JSON message format

# Or with footer:
feat: redesign command protocol

BREAKING CHANGE: Command format changed from single string to JSON object
```

#### Available Types:

| Type       | Description             | Version Bump | Example                            |
| ---------- | ----------------------- | ------------ | ---------------------------------- |
| `feat`     | New feature             | MINOR        | `feat: add WiFi status LED`        |
| `fix`      | Bug fix                 | PATCH        | `fix: correct temperature scaling` |
| `perf`     | Performance improvement | PATCH        | `perf: reduce memory usage`        |
| `refactor` | Code refactor           | PATCH        | `refactor: simplify WiFi logic`    |
| `docs`     | Documentation only      | None         | `docs: update API reference`       |
| `style`    | Code formatting         | None         | `style: fix indentation`           |
| `test`     | Add tests               | None         | `test: add thermocouple tests`     |
| `build`    | Build system changes    | None         | `build: update platformio.ini`     |
| `ci`       | CI/CD changes           | None         | `ci: add firmware signing`         |
| `chore`    | Maintenance tasks       | None         | `chore: update dependencies`       |
| `revert`   | Revert previous commit  | PATCH        | `revert: undo WiFi changes`        |

#### Scopes (optional but recommended):

- `wifi` - WiFi related changes
- `ota` - OTA update functionality
- `serial` - Serial communication
- `temp` - Temperature reading
- `led` - LED status indicators
- `storage` - Preferences/EEPROM
- `security` - Security features

### How Semantic Versioning Works

Version format: `MAJOR.MINOR.PATCH` (e.g., `1.2.3`)

- **MAJOR** (1.0.0 → 2.0.0): Breaking changes

  - Incompatible API changes
  - Changes that require user action
  - Triggered by: `BREAKING CHANGE:` footer or `!` after type

- **MINOR** (1.0.0 → 1.1.0): New features (backwards compatible)

  - New functionality
  - New optional features
  - Triggered by: `feat:` commits

- **PATCH** (1.0.0 → 1.0.1): Bug fixes (backwards compatible)
  - Bug fixes
  - Performance improvements
  - Triggered by: `fix:`, `perf:`, `refactor:` commits

### Workflow

1. **Make changes** in a feature branch
2. **Commit** using conventional commit format
3. **Push** to GitHub
4. **Create Pull Request** to `main`
5. **CI validates** commit messages
6. **Merge** to `main`
7. **Automatic release**:
   - Semantic-release analyzes commits
   - Calculates new version number
   - Generates changelog
   - Builds and signs firmware
   - Creates GitHub release
   - Tags commit
   - Updates CHANGELOG.md

### Example Development Flow

```bash
# Create feature branch
git checkout -b feat/add-bluetooth-support

# Make changes and commit with conventional format
git add .
git commit -m "feat(bluetooth): add BLE device discovery"

# Push and create PR
git push origin feat/add-bluetooth-support

# After PR is merged to main:
# - CI automatically detects "feat:" commit
# - Bumps version from 1.2.3 → 1.3.0
# - Builds and releases firmware
# - Creates release notes
```

### Breaking Changes

When making breaking changes:

```bash
# Option 1: Add ! after type
git commit -m "feat!: change temperature data format"

# Option 2: Add BREAKING CHANGE footer
git commit -m "feat: redesign command protocol

BREAKING CHANGE: Commands now require JSON format instead of plain text.
Migration guide: See docs/migration-v2.md"
```

### Multi-Commit PR

If your PR has multiple commits:

```bash
# All commits in PR should follow convention
git commit -m "feat(wifi): add connection retry logic"
git commit -m "test(wifi): add retry tests"
git commit -m "docs(wifi): document retry behavior"

# Version bump will be based on highest-impact commit:
# - If any commit is feat: → MINOR bump
# - If all commits are fix: → PATCH bump
# - If any commit has BREAKING CHANGE: → MAJOR bump
```

### Tips

✅ **Good commit messages:**

```
feat(ota): add firmware signature verification
fix(temp): correct Celsius to Fahrenheit conversion
perf(serial): reduce JSON buffer allocations
docs: add OTA update guide
```

❌ **Bad commit messages:**

```
fixed bug          # No type
Add feature        # No type, wrong case
feat: Fixed wifi   # Mixed tenses
fix added stuff    # Missing colon
FEAT: new thing    # Wrong case
```

### Local Validation

Install commitlint locally to validate before pushing:

```bash
# Install
npm install -g @commitlint/cli @commitlint/config-conventional

# Validate last commit
echo "$(git log -1 --pretty=%B)" | commitlint

# Add to git hooks (optional)
npx husky add .husky/commit-msg 'npx commitlint --edit $1'
```

### Viewing Changelog

The changelog is automatically maintained in `CHANGELOG.md`:

```markdown
# Changelog

## [1.3.0] - 2024-01-15

### ✨ Features

- **wifi**: add mDNS discovery support (#42)
- **ota**: implement firmware signature verification (#45)

### 🐛 Bug Fixes

- **serial**: handle disconnection gracefully (#43)
- **temp**: correct temperature reading offset (#44)

### ⚡ Performance Improvements

- **serial**: reduce JSON serialization overhead (#46)
```

### Release Process (Automatic)

When you push to `main`:

1. **CI analyzes commits** since last release
2. **Determines version bump**:
   - Breaking change? → Major (2.0.0)
   - New feature? → Minor (1.3.0)
   - Bug fix only? → Patch (1.2.1)
   - No release-worthy commits? → No release
3. **Updates version** in code
4. **Builds firmware** with PlatformIO
5. **Signs firmware** with private key
6. **Generates changelog**
7. **Creates GitHub release** with:
   - Tag (e.g., `v1.3.0`)
   - Release notes
   - Firmware binaries
   - Signatures
   - Manifest
8. **Commits CHANGELOG.md** back to repo

### Manual Release (Emergency)

To force a release:

```bash
# Create and push a tag manually
git tag v1.2.4
git push origin v1.2.4

# Or trigger via GitHub UI:
# Releases → Draft a new release → Create tag
```

### Versioning for Firmware

Devices check for updates using semantic versioning:

```cpp
// Device on v1.2.0 sees v1.3.0 available
// → Offers optional update (new features)

// Device on v1.2.0 sees v2.0.0 available
// → Warns about breaking changes, requires confirmation

// Device on v1.2.0 sees v1.2.1 available
// → Auto-installs (bug fixes)
```

### Questions?

- **"Do I need to know the next version?"** No, it's calculated automatically
- **"What if I forget the format?"** CI will reject and tell you
- **"Can I skip a version?"** No, versioning is sequential
- **"Can I revert a release?"** Yes, use `revert:` commits
- **"What about pre-releases?"** Create a `beta` branch for v1.3.0-beta.1

### Resources

- [Conventional Commits Specification](https://www.conventionalcommits.org/)
- [Semantic Versioning](https://semver.org/)
- [Semantic Release](https://semantic-release.gitbook.io/)

## Quick Reference

```bash
# Feature: 1.0.0 → 1.1.0
git commit -m "feat: add new sensor support"

# Bug fix: 1.0.0 → 1.0.1
git commit -m "fix: correct temperature offset"

# Breaking change: 1.0.0 → 2.0.0
git commit -m "feat!: redesign API"

# No release
git commit -m "docs: update readme"
git commit -m "chore: update dependencies"
```
