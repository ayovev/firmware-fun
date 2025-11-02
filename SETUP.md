# Automated Release Setup Guide

This guide will help you set up automated semantic versioning and releases for your firmware project.

## Overview

Once configured, this system will:

- ✅ Validate all commit messages follow conventional commit format
- ✅ Automatically determine the next version number based on commits
- ✅ Build and sign firmware
- ✅ Create GitHub releases with changelogs
- ✅ Update CHANGELOG.md automatically
- ✅ Tag releases in Git

## Prerequisites

- GitHub repository
- Node.js 24+ installed locally (for commit validation)
- PlatformIO installed (for firmware builds)
- OpenSSL (for firmware signing)

## Step 1: Add Files to Repository

Add these files to your repository root:

```
firmware-fun/
├── .github/
│   └── workflows/
│       ├── release.yml          # Main release workflow
│       └── commitlint.yml       # Commit validation
├── .releaserc.json              # Semantic-release config
├── commitlint.config.js         # Commitlint rules
├── package.json                 # Node dependencies
├── CONTRIBUTING.md              # Developer guide
└── setup-git-hooks.sh           # Setup script
```

All these files are provided in the artifacts above.

## Step 2: Configure GitHub Secrets

### 2.1 Generate Signing Key

```bash
# Generate RSA private key
openssl genrsa -out private_key.pem 2048

# Extract public key (embed this in your firmware)
openssl rsa -in private_key.pem -outform PEM -pubout -out public_key.pem

# View private key (you'll copy this to GitHub)
cat private_key.pem
```

### 2.2 Add Secret to GitHub

1. Go to your repository on GitHub
2. Settings → Secrets and variables → Actions
3. Click "New repository secret"
4. Name: `FIRMWARE_SIGNING_KEY`
5. Value: Paste the entire contents of `private_key.pem` (including BEGIN/END lines)
6. Click "Add secret"

⚠️ **IMPORTANT**: Store `private_key.pem` securely offline. Never commit it to Git!

### 2.3 Embed Public Key in Firmware

In your `src/main.cpp`:

```cpp
// This is safe to commit - it's the public key
const char* PUBLIC_KEY = R"(
-----BEGIN PUBLIC KEY-----
MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA...
[paste your public_key.pem contents here]
-----END PUBLIC KEY-----
)";
```

## Step 3: Update Repository Settings

### 3.1 Enable GitHub Actions

1. Go to Settings → Actions → General
2. Under "Workflow permissions", select:
   - ✅ "Read and write permissions"
   - ✅ "Allow GitHub Actions to create and approve pull requests"
3. Save

### 3.2 Protect Main Branch (Optional but Recommended)

1. Settings → Branches
2. Add rule for `main`
3. Enable:
   - ✅ Require status checks to pass before merging
   - ✅ Require branches to be up to date before merging
   - Select: `commitlint` check

This prevents merging PRs with invalid commit messages.

## Step 4: Local Development Setup

### 4.1 Install Dependencies

```bash
# Clone repository
git clone https://github.com/ayovev/firmware-fun.git
cd firmware-fun

# Install Node dependencies
npm install

# Setup git hooks (validates commits locally)
chmod +x setup-git-hooks.sh
./setup-git-hooks.sh
```

### 4.2 Configure Git (Optional)

For guided commit message creation:

```bash
# Use commitizen for interactive commits
npm run commit

# This will prompt you:
# ? Select the type of change: (feat, fix, docs, etc.)
# ? What is the scope? (wifi, ota, serial, etc.)
# ? Write a short description:
# ? Provide a longer description: (optional)
# ? Are there any breaking changes? (y/N)
```

## Step 5: Update Firmware Code

### 5.1 Update platformio.ini

```ini
[env:esp32-s3]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino

# Version will be automatically updated by CI
build_flags =
    -DFIRMWARE_VERSION=\"1.0.0\"
    -DDEVICE_MODEL=\"P61\"
```

### 5.2 Update main.cpp

```cpp
// Version is injected at build time by CI
#ifndef FIRMWARE_VERSION
#define FIRMWARE_VERSION "dev"
#endif

const char* FIRMWARE_VERSION_STR = FIRMWARE_VERSION;

void setup() {
  Serial.begin(115200);
  Serial.printf("Firmware Version: %s\n", FIRMWARE_VERSION_STR);
  // ... rest of setup
}
```

## Step 6: Test the System

### 6.1 Make a Test Commit

```bash
# Create a test branch
git checkout -b test/release-automation

# Make a small change
echo "# Test" >> README.md

# Commit with conventional format
git add README.md
git commit -m "docs: test release automation"

# Push
git push origin test/release-automation
```

### 6.2 Create Pull Request

1. Go to GitHub and create a PR
2. CI will run commitlint to validate your commit message
3. If valid: ✅ Check passes
4. If invalid: ❌ Check fails with explanation

### 6.3 Merge to Main

1. Merge the PR
2. Watch GitHub Actions:
   - Actions tab → "Automated Release" workflow
3. After ~5 minutes:
   - New release appears in Releases
   - Firmware binaries attached
   - CHANGELOG.md updated
   - Commit tagged (e.g., `v1.0.1`)

## Step 7: First Real Release

### 7.1 Initial Version

If starting fresh, create an initial release tag:

```bash
# Create first version tag
git tag v1.0.0
git push origin v1.0.0

# This establishes the baseline for semantic-release
```

### 7.2 Start Using Conventional Commits

From now on, all commits should follow the format:

```bash
# New features (bumps MINOR version)
git commit -m "feat(wifi): add mDNS support"

# Bug fixes (bumps PATCH version)
git commit -m "fix(temp): correct sensor reading"

# Breaking changes (bumps MAJOR version)
git commit -m "feat!: redesign API"
git commit -m "feat: change protocol

BREAKING CHANGE: Command format changed to JSON"
```

## Usage Examples

### Example 1: Bug Fix Release

```bash
# Make fix
git checkout -b fix/temperature-offset
# ... edit code ...
git add src/main.cpp
git commit -m "fix(temp): correct Celsius to Fahrenheit conversion"
git push origin fix/temperature-offset

# Create PR, merge to main
# → CI automatically releases v1.0.1 (if current is v1.0.0)
```

### Example 2: Feature Release

```bash
# Add feature
git checkout -b feat/bluetooth
# ... edit code ...
git add src/bluetooth.cpp
git commit -m "feat(bluetooth): add BLE device discovery"
git push origin feat/bluetooth

# Create PR, merge to main
# → CI automatically releases v1.1.0 (if current is v1.0.0)
```

### Example 3: Breaking Change

```bash
# Breaking change
git checkout -b feat/new-api
# ... edit code ...
git add src/api.cpp
git commit -m "feat(api): redesign command protocol

BREAKING CHANGE: Commands now use JSON format instead of plain text.
See migration guide in docs/migration.md"
git push origin feat/new-api

# Create PR, merge to main
# → CI automatically releases v2.0.0 (if current is v1.5.2)
```

## Troubleshooting

### "Commitlint check failed"

Your commit message doesn't follow conventional format.

**Fix:**

```bash
# View the error in GitHub Actions log
# Amend your commit:
git commit --amend -m "feat: correct commit message format"
git push --force
```

### "Release workflow failed at build step"

PlatformIO build failed.

**Fix:**

```bash
# Test build locally
pio run

# Fix compilation errors
# Commit and push fix
git commit -m "fix(build): resolve compilation error"
```

### "Signing failed"

Private key not configured correctly.

**Fix:**

1. Verify secret name is exactly `FIRMWARE_SIGNING_KEY`
2. Re-add the secret with full key contents
3. Ensure key includes BEGIN/END lines

### "No release created"

No release-worthy commits since last release.

**Explanation:**

- `docs:`, `chore:`, `style:` commits don't trigger releases
- Only `feat:`, `fix:`, `perf:`, `refactor:` trigger releases

**Fix:** Make a meaningful change with proper commit type.

### "Version conflict"

Manually created tag conflicts with semantic-release.

**Fix:**

```bash
# Delete conflicting tag
git tag -d v1.2.3
git push origin :refs/tags/v1.2.3

# Let semantic-release handle versioning
```

## Advanced Configuration

### Custom Release Rules

Edit `.releaserc.json` to customize version bumping:

```json
{
  "releaseRules": [
    { "type": "docs", "release": "patch" }, // Make docs trigger patch
    { "type": "refactor", "release": false }, // Disable refactor releases
    { "scope": "deps", "release": "patch" } // Dependencies always patch
  ]
}
```

### Pre-release Versions

Create a `beta` branch for pre-releases:

```json
{
  "branches": [
    "main",
    {
      "name": "beta",
      "prerelease": true
    }
  ]
}
```

Commits to `beta` branch create `v1.2.0-beta.1` releases.

### Skip CI

To push without triggering release:

```bash
git commit -m "chore: update docs [skip ci]"
```

## Monitoring

### View Release History

- GitHub → Releases
- See all versions with changelogs
- Download firmware for any version

### View Automated Changelog

- Check `CHANGELOG.md` in repository
- Automatically updated with each release
- Grouped by type (Features, Bug Fixes, etc.)

### Check CI Status

- GitHub → Actions
- See all workflow runs
- Debug any failures

## Best Practices

1. **Always use conventional commits** - CI enforces this
2. **Write clear commit messages** - They become release notes
3. **Scope your commits** - Makes changelog easier to read
4. **Test locally** - `npm run commit` helps catch issues
5. **Review changelogs** - Before merging, preview what will be released
6. **Use PRs** - Allows review of commit messages before release

## Resources

- [Conventional Commits](https://www.conventionalcommits.org/)
- [Semantic Versioning](https://semver.org/)
- [Semantic Release Docs](https://semantic-release.gitbook.io/)
- [Commitlint](https://commitlint.js.org/)

## Support

If you run into issues:

1. Check the Actions logs in GitHub
2. Review CONTRIBUTING.md for examples
3. Test commits locally with: `echo "feat: test" | npx commitlint`
4. Ensure git hooks are installed: `./setup-git-hooks.sh`

---

**You're all set!** 🎉

Start committing with conventional format and watch your releases happen automatically!
