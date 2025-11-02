module.exports = {
  extends: ["@commitlint/config-conventional"],
  rules: {
    // Enforce conventional commit format
    "type-enum": [
      2,
      "always",
      [
        "feat", // New feature
        "fix", // Bug fix
        "docs", // Documentation only
        "style", // Code style (formatting, missing semi colons, etc)
        "refactor", // Code refactor
        "perf", // Performance improvement
        "test", // Adding tests
        "build", // Build system changes
        "ci", // CI configuration changes
        "chore", // Other changes that don't modify src or test files
        "revert", // Revert a previous commit
      ],
    ],
    "type-case": [2, "always", "lower-case"],
    "type-empty": [2, "never"],
    "scope-case": [2, "always", "lower-case"],
    "subject-empty": [2, "never"],
    "subject-full-stop": [2, "never", "."],
    "header-max-length": [2, "always", 100],
    "body-leading-blank": [1, "always"],
    "body-max-line-length": [2, "always", 100],
    "footer-leading-blank": [1, "always"],
    "footer-max-line-length": [2, "always", 100],
  },
};
