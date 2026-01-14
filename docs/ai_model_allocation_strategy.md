# AI Model Tier Allocation Strategy for Super Metroid DSi Port

## Overview

This document outlines the optimal allocation of AI model tiers (Claude, Gemini, OpenAI) across different components of the Super Metroid DSi port project. The goal is to match model capabilities to task complexity, maximizing quality while maintaining cost efficiency.

## Core Allocation Strategy (3-Agent Parallel Model)

### Agent A - Physics & Core Systems (300K-600K tokens)
**Recommended Models:** Claude Opus 4.5 or Gemini 2.0 Flash

**Responsibilities:**
- 65C816→ARM assembly translation (critical path)
- Physics engine implementation with frame-perfect accuracy
- Complex movement mechanics: shinespark chains, momentum preservation, slope physics
- Boss AI: Mother Brain (3 phases), Ridley, Draygon, Phantoon, etc.
- Player state machine and weapon systems
- Physics verification and speedrun technique testing

**Rationale:** Physics translation is the forcing function requiring frontier-level reasoning. Cross-architecture assembly translation while maintaining frame-perfect accuracy justifies premium model usage. This is the critical path bottleneck for quality.

### Agent B - Graphics & Integration (150K-300K tokens)
**Recommended Models:** Claude Sonnet 4, Gemini 1.5 Pro, or GPT-4o

**Responsibilities:**
- System architecture and memory planning
- Graphics rendering pipeline: dual-screen framebuffer, 4-layer backgrounds, sprite management
- Room/world systems and camera logic
- DSi hardware integration
- Final integration coordinator
- Performance profiling and optimization

**Rationale:** Graphics rendering and game systems are moderately complex but follow established patterns. Mid-tier models excel here. Gemini 1.5 Pro's spatial reasoning is particularly helpful for the screen-splitting challenge (256×224 → dual 256×192 screens).

### Agent C - Assets & Support (100K-200K tokens)
**Recommended Models:** Claude Haiku 4.5 or Gemini 1.5 Flash

**Responsibilities:**
- devkitARM environment setup
- Asset conversion tools (Python): graphics (SNES 4bpp→DSi), audio (SPC700→DSi), level data
- ROM extraction and parsing
- Testing infrastructure: debug overlay, input recording
- Room-by-room testing and bug documentation
- Final documentation and README

**Rationale:** ~50% of implementation work is procedural and deterministic (asset conversion, testing, docs). Lightweight models are more than capable for these tasks, providing significant cost savings without quality loss.

---

## Component-by-Component Breakdown

| Component | Complexity | Best Model | Reasoning |
|-----------|-----------|-----------|-----------|
| Physics engine & assembly translation | Critical/Very High | **Opus 4.5** or **Gemini 2.0 Flash** | Needs frontier-level reasoning for cross-architecture translation |
| Boss AI (Mother Brain, Ridley, etc.) | High | **Opus 4.5** or **Sonnet 4** | Complex state machines, pattern recognition; Sonnet handles well if patterns are clear |
| Standard enemy AI (20-30 types) | Moderate | **Sonnet 4** or **Gemini 1.5 Pro** | Repetitive patterns, less novel reasoning needed |
| Graphics rendering pipeline | Moderate-High | **Sonnet 4**, **Gemini 1.5 Pro**, or **GPT-4o** | Spatial reasoning important; all three are solid |
| Camera/room system | Moderate | **Sonnet 4** or **Gemini 1.5 Pro** | Standard game dev patterns; Gemini's spatial reasoning helpful |
| Save/load & state serialization | Low-Moderate | **Haiku 4.5** or **Gemini 1.5 Flash** | Straightforward data structures |
| Input buffering system | Low-Moderate | **Haiku 4.5** or **Gemini 1.5 Flash** | Well-defined problem; lightweight models sufficient |
| Asset conversion tools (Python) | Low | **Haiku 4.5** or **Gemini 1.5 Flash** | Deterministic transformations, minimal reasoning |
| ROM parsing & data extraction | Low | **Haiku 4.5** or **Gemini 1.5 Flash** | Procedural format parsing |
| Debug overlay & testing tools | Low | **Haiku 4.5** or **Gemini 1.5 Flash** | Support tooling, straightforward |
| Documentation & README | Low | **Haiku 4.5** or **Gemini 1.5 Flash** | Writing task, no complex reasoning |

---

## Model Tier Details by Vendor

### Frontier Tier (For Physics Translation)

**Claude Opus 4.5**
- Strongest reasoning for architectural translation
- Excellent at understanding assembly-level semantics
- Best for critical path work requiring novel problem-solving

**Gemini 2.0 Flash**
- Also frontier-level for reasoning
- May be faster for specific translation tasks
- Strong alternative to Opus

**OpenAI o1/o3**
- Capable but less ideal for large-scale code generation
- Better for proof/verification tasks than implementation

### Mid-Tier (For Graphics & Game Systems)

**Claude Sonnet 4**
- Excellent balance of reasoning and code generation
- Reliable for structured game systems
- Strong at integration work

**Gemini 1.5 Pro**
- Very capable all-around
- Strong spatial/visual reasoning (helpful for graphics work)
- Good for coordinate systems and screen layout

**GPT-4o**
- Reliable for structured code patterns
- Good at game development conventions
- Solid alternative for game systems

### Lightweight Tier (For Assets & Support)

**Claude Haiku 4.5**
- More than capable for deterministic tasks
- Excellent cost efficiency
- Great for procedural work

**Gemini 1.5 Flash**
- Fast and efficient
- Strong for format transformations
- Good alternative to Haiku

**GPT-4o mini**
- Also capable for straightforward tasks
- Cost-effective for high-volume operations

---

## Key Insights

1. **Physics is the Forcing Function:** The 65C816→ARM translation genuinely requires frontier models due to cross-architecture reasoning complexity and frame-perfect accuracy requirements.

2. **Graphics Benefits from Multi-Model Approach:** Spatial reasoning matters for the complex screen-splitting challenge. Gemini 1.5 Pro excels here, but Sonnet and GPT-4o are also solid choices.

3. **Cost Efficiency Matters:** Approximately 50% of implementation work (asset conversion, testing infrastructure, documentation) doesn't require frontier models. Using Haiku/Flash for these tasks saves costs significantly while maintaining quality.

4. **Match Complexity to Capability:** Standard enemy AI with repetitive patterns doesn't need Opus-level reasoning. Mid-tier models handle these well.

5. **Procedural Work is Lightweight Territory:** ROM parsing, asset conversion scripts, and test utilities are deterministic. Lightweight models are perfect here.

---

## Estimated Token Distribution

| Agent | Model Tier | Token Range | Cost Impact |
|-------|-----------|-------------|-------------|
| Agent A (Physics) | Frontier | 300K-600K | High (justified by critical path) |
| Agent B (Graphics) | Mid-Tier | 150K-300K | Medium (balanced quality/cost) |
| Agent C (Assets) | Lightweight | 100K-200K | Low (maximum efficiency) |
| **Total** | **Mixed** | **550K-1.1M** | **Optimized blend** |

By using this tiered approach rather than using Opus for everything:
- Save ~40-50% on token costs for Agent C work
- Save ~20-30% on token costs for Agent B work
- Maintain premium quality where it matters most (Agent A)

---

## Recommended Workflow

1. **Start with Agent A (Opus/Gemini 2.0):** Begin physics translation immediately as it's the critical path
2. **Parallel Agent C (Haiku/Flash):** Run asset conversion and setup concurrently
3. **Agent B (Sonnet/Gemini 1.5 Pro):** Begin architecture and graphics work once setup is complete
4. **Integration Phase:** Agent B coordinates; use Agent A for physics verification, Agent C for testing
5. **Polish:** All agents at appropriate tiers for their specialization

---

## Future Applications

This allocation pattern is useful for:
- Game porting projects (retro→modern platform)
- Emulator development
- Projects with clear high-complexity vs procedural boundaries
- Multi-domain technical projects (physics + graphics + tools)
- Any work where 40-60% is deterministic/procedural

The key is identifying the "forcing function" component that genuinely requires frontier reasoning, then optimizing everything else to appropriate model tiers.
