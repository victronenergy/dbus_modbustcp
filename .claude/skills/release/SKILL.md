---
name: release
description: Cut a new dbus-modbustcp release - bump VERSION in dbus_modbustcp.pro, commit "vX.Y.Z" on master, create the annotated tag vX.Y.Z, and push master and the tag to origin. Only use when the user explicitly asks for a release.
disable-model-invocation: true
---

# Create a release

A release is a single commit on `master` that changes only the `VERSION = x.y.z`
line in `dbus_modbustcp.pro`, plus an annotated tag on that commit. Both the
commit message and the tag annotation are exactly the version prefixed with a
`v`, e.g. `v1.0.103`. Look at `git show v1.0.102` for a reference release.

The user may pass a version as an argument (e.g. `/release 1.0.110`). Without
one, bump the third component.

Work through the steps in order. Any failed check is a **hard stop**: report
what is wrong and end. Don't try to fix it (no switching branches, no pulling,
no rebasing, no stashing) unless the user explicitly asks afterwards.

## 1. Must be on an up-to-date master

Releases have accidentally been made off other branches before, so check this
first, before touching anything:

```bash
git rev-parse --abbrev-ref HEAD
```

If this isn't exactly `master`, stop and tell the user which branch they're on.

Then make sure master is not behind origin:

```bash
git fetch origin
git rev-list --left-right --count origin/master...HEAD
```

The output is `<behind> <ahead>`. If `behind` is not 0, stop: master is behind
or has diverged from `origin/master`.

If `ahead` is not 0, show the commits that will be pushed along with the
release so the user knows what is going out:

```bash
git log --oneline origin/master..HEAD
```

## 2. Working tree must be sane

```bash
git diff --cached --quiet   # nothing staged
git diff --quiet -- dbus_modbustcp.pro   # .pro file unmodified
```

If anything is staged, or `dbus_modbustcp.pro` has local changes, stop. Other
modified tracked files (`git diff --stat`) are only a warning: mention them and
note they won't be part of the release. Ignore untracked files (swap files,
`Makefile`, `.obj/` etc).

## 3. Choose the version

Read the current version from the `VERSION = ` line in `dbus_modbustcp.pro`.
Propose the next version: the argument if one was given, otherwise the current
version with the third component incremented (`1.0.102` -> `1.0.103`).

Confirm it with the user using AskUserQuestion (proposed version, with "Other"
available for a different one). A version must be three numeric components and
greater than the current one.

Make sure the tag doesn't already exist, locally or on origin. Both of these
must print nothing:

```bash
git tag -l vNEW
git ls-remote --tags origin refs/tags/vNEW
```

## 4. Bump, commit and tag

Edit only the `VERSION = ` line in `dbus_modbustcp.pro`.

Commit just that file. The message is exactly `vNEW` with nothing else: no
body and no Co-Authored-By trailer, to match the existing release history.

```bash
git commit -m "vNEW" -- dbus_modbustcp.pro
git tag -a vNEW -m "vNEW"
git show --stat vNEW
```

Check the `git show` output: the tag message is `vNEW`, the commit message is
`vNEW`, and the only change is one line in `dbus_modbustcp.pro`.

## 5. Confirm, then push

Check once more that the branch is still `master`
(`git rev-parse --abbrev-ref HEAD`).

Ask the user with AskUserQuestion whether to push `master` and `vNEW` to
origin. Never push without an explicit yes.

On yes, push both in one atomic push so it's all or nothing:

```bash
git push --atomic origin master vNEW
```

Report the result.

If the user declines, leave the commit and tag in place locally and tell them
how to either push later (the command above) or undo the release:

```bash
git tag -d vNEW && git reset --keep HEAD~1
```
