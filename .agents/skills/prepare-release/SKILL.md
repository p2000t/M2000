---
name: prepare-release
description: Prepare a new software release by discovering every active version reference, updating it consistently across source and packaging metadata, applying platform-specific packaging rules such as WiX GUID/version changes, and validating the resulting diff. Use when an agent is asked to bump, update, or prepare a release version in a repository.
---

# Prepare a Release

Update a repository from an existing release version to a requested new version. Treat versioning as a repository-wide consistency task: discover the sources first, make only intentional edits, handle installer rules explicitly, and verify both the old-version removal and the new-version presence.

## Workflow

### 1. Establish the release change

- Read the requested old and new versions from the user. If the old version is not stated, determine it from the repository's primary version source and confirm that it is consistent with the other metadata.
- Preserve the user's current branch and unrelated work. Inspect the worktree before editing; do not discard or overwrite existing changes.
- Identify repository-local instructions (`AGENTS.md`, `CLAUDE.md`, `CONTRIBUTING.md`, CI/release documentation) and follow them.
- Do not commit, tag, push, publish, or create a release unless the user explicitly requests it.

### 2. Discover version sources

Search tracked text files, including hidden files, for the old version and nearby version patterns. Prefer `rg` when available; exclude `.git`, build output, dependency caches, and generated/binary artifacts. Adapt the command to the host shell.

```text
rg -n --hidden --glob '!.git' --glob '!build' --glob '!dist' --glob '!bin' OLD_VERSION .
```

Classify each match before editing:

- Update active product metadata, source constants, package manifests, installer definitions, platform manifests, library/core metadata, documentation that states the current version, and CI/release configuration.
- Normally preserve historical changelogs, archived release notes, migration tests, fixtures, and examples that intentionally describe the old release. If the user explicitly requires every textual occurrence to change, obey that request after warning that historical accuracy will be lost.
- Do not edit compiled binaries or generated files when their source/configuration is available; regenerate them only when the project workflow requires it.

Also search for the current version in forms that may differ by format, such as `Version`, `version`, `display_version`, `CFBundleShortVersionString`, package-control fields, C macros, and installer XML attributes.

### 3. Apply the version bump

- Update the canonical version definition first, then all active consumers found during discovery.
- Use the repository's existing version format and line-ending/style conventions.
- Make focused edits. Do not perform an unrestricted repository-wide replacement without first classifying matches.
- Keep package versions valid for their ecosystem (for example, Debian package versions and plist/XML values must remain syntactically valid).

### 4. Apply WiX rules when present

If a WiX source file contains release instructions, read and follow them. For this repository's `package/WiX/product.wxs`:

- Replace the three old-version occurrences used by the WiX product and upgrade logic with the new version.
- Generate a new GUID for the `Component` with `Id="ApplicationFiles"` on every release.
- Keep the `UpgradeCode` unchanged.
- Change the `ApplicationShortcuts` component GUID only when one of its child elements changes; otherwise preserve it.
- Do not change unrelated WiX identifiers or package behavior.

Check the installer semantics after editing: the product version must be the new version, newer-version detection must reject versions above it, and the older-version range must allow upgrades from prior versions according to the existing inclusivity settings.

### 5. Validate

Run the repository's relevant lightweight checks and inspect the complete diff. At minimum:

```text
rg -n --hidden --glob '!.git' OLD_VERSION .
git diff --check
git status --short
git diff --stat
git diff
```

The old version should have no remaining active references. Confirm that the new version appears in every expected metadata/source location, the generated GUID is a valid new GUID, and no unrelated files changed. Validate XML/plist/manifest/package syntax where a suitable local checker exists. Run the project's release/build or test command when it is available and reasonably scoped.

Report the files changed, any intentionally preserved historical matches, validation results, and any checks that could not run. Leave the worktree uncommitted unless the user asks for a commit.

## Common pitfalls

- A pull request's diff should be compared against its target branch; do not “remove” legitimate base-branch history merely to hide it.
- Replacing only the visible README version leaves installers and platform packages inconsistent.
- Reusing a WiX component GUID across changed component contents can break Windows Installer servicing.
- Changing WiX `UpgradeCode` prevents the installer from recognizing the existing product family.
- A successful textual replacement is not proof that package metadata or upgrade ranges are semantically correct.
