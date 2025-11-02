#!/bin/bash

# Setup script for Git hooks and commit validation
# Run this once after cloning the repository

set -e

echo "🔧 Setting up Git hooks for conventional commits..."

# Check if npm is installed
if ! command -v npm &> /dev/null; then
    echo "❌ npm is required but not installed. Please install Node.js first."
    exit 1
fi

# Install dependencies
echo "📦 Installing dependencies..."
npm install

# Create .husky directory if it doesn't exist
if [ ! -d ".husky" ]; then
    echo "📁 Creating .husky directory..."
    mkdir -p .husky
fi

# Create commit-msg hook
echo "🪝 Creating commit-msg hook..."
cat > .husky/commit-msg << 'EOF'
#!/bin/sh
. "$(dirname "$0")/_/husky.sh"

# Validate commit message
npx --no-install commitlint --edit "$1"
EOF

chmod +x .husky/commit-msg

# Create prepare-commit-msg hook (optional - for commitizen)
echo "🪝 Creating prepare-commit-msg hook..."
cat > .husky/prepare-commit-msg << 'EOF'
#!/bin/sh
. "$(dirname "$0")/_/husky.sh"

# Uncomment to use commitizen for interactive commits
# exec < /dev/tty && npx cz --hook || true
EOF

chmod +x .husky/prepare-commit-msg

# Test commitlint installation
echo "✅ Testing commitlint..."
if echo "feat: test message" | npx commitlint; then
    echo "✅ Commitlint is working correctly!"
else
    echo "❌ Commitlint test failed"
    exit 1
fi

echo ""
echo "✅ Setup complete!"
echo ""
echo "📝 Commit message examples:"
echo "  git commit -m 'feat: add new feature'"
echo "  git commit -m 'fix: resolve bug'"
echo "  git commit -m 'feat!: breaking change'"
echo ""
echo "🎯 Or use commitizen for guided commits:"
echo "  npm run commit"
echo ""
echo "📚 See CONTRIBUTING.md for more details"