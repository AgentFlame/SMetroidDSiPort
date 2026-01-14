# Super Metroid DSi Port - Project Guidelines

## Project Overview

This is a port of Super Metroid (SNES) to Nintendo DSi using devkitARM and libnds. The project involves physics engine translation, graphics rendering, enemy AI, and asset conversion.

## Git Commit Policy

**IMPORTANT:** All code changes must be committed to git with a brief description.

### Commit Guidelines

1. **Commit frequently** - After implementing any meaningful feature or fix
2. **Brief but descriptive** - Commit messages should clearly indicate what changed
3. **Follow conventional format**:
   ```
   <type>: <brief description>

   Co-Authored-By: Claude Sonnet 4.5 <noreply@anthropic.com>
   ```

### Commit Types

- `feat:` - New feature implementation
- `fix:` - Bug fixes
- `refactor:` - Code restructuring without behavior change
- `perf:` - Performance improvements
- `docs:` - Documentation updates
- `test:` - Adding or updating tests
- `build:` - Build system or dependency changes
- `asset:` - Asset conversion or data changes

### Examples

```bash
git commit -m "feat: implement Samus physics engine with gravity and momentum

- Add gravity calculation for normal/water/lava environments
- Implement acceleration and deceleration curves
- Add collision detection for floor and ceiling

Co-Authored-By: Claude Sonnet 4.5 <noreply@anthropic.com>"
```

```bash
git commit -m "fix: correct sprite priority sorting in renderer

- Fix z-ordering bug causing sprites to render behind backgrounds
- Add priority comparison in sprite sort function

Co-Authored-By: Claude Sonnet 4.5 <noreply@anthropic.com>"
```

```bash
git commit -m "asset: convert Crateria tileset to DSi format

- Process tiles from ROM offset 0x8C8000
- Generate palette and tile data using grit
- Add to asset build pipeline

Co-Authored-By: Claude Sonnet 4.5 <noreply@anthropic.com>"
```

## Development Workflow

1. **Before making changes**: Ensure you're on the correct branch
2. **After implementing a feature/fix**:
   - Review the changes with `git diff`
   - Stage relevant files with `git add`
   - Commit with a descriptive message
3. **Push regularly**: Push commits to remote to back up work

## Branch Strategy

- `main` - Stable, tested code
- `dev` - Active development branch
- Feature branches: `feature/physics-engine`, `feature/boss-ai`, etc.

## Code Standards

- **C code**: Follow libnds conventions, keep functions focused
- **Python tools**: PEP 8 style, clear variable names
- **Comments**: Only where logic isn't self-evident
- **Performance**: Target 60 FPS, profile before optimizing

## Testing Requirements

- Physics: Frame-by-frame comparison with original SNES
- Graphics: Visual verification, no sprite flickering
- Gameplay: All rooms accessible, all items collectable
- Performance: Stable 60 FPS in typical gameplay

## Documentation

- Update README.md for major features
- Document build instructions in docs/building.md
- Keep architecture notes in docs/ folder
- Track known issues in bugs/ folder

---

**Remember:** Commit early, commit often. Each commit should represent a logical unit of work that could be understood by someone reviewing the history later.
