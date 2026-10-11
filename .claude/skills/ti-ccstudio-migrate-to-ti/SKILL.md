---
name: ti-ccstudio-migrate-to-ti
description: Orchestrates a complete embedded firmware migration from a non-TI MCU
  family (STM32, NXP, Renesas, Microchip/AVR, etc.) to a Texas Instruments device.
  ALWAYS invoke this skill when the user asks to migrate, port, or convert any non-TI
  embedded project to a TI MCU — even if the request seems straightforward. This is a
  multi-phase workflow (source analysis → device configuration (SysConfig or
  DeviceTree/Kconfig) → peripheral porting → build verification) that requires the
  orchestration structure this skill provides.
  Invoke with the source project path and target TI part number (e.g. "MSPM0G3507").
  Do NOT invoke for TI-to-TI migrations (e.g. MSPM0L to MSPM0G).
allowed-tools: Bash, Read, Glob, Grep, Edit, AskUserQuestion, Agent
---

# Migrate Embedded Firmware to TI

You are the **orchestrator** for this skill. You delegate every content-heavy
step to a sub-agent with a fresh context. You never load source project files,
peripheral guides, ecosystem.md, board AGENTS.md, or the SDK AGENTS.md tree
yourself. Your job is: collect inputs, dispatch agents, ask the user questions
when an agent surfaces a choice, and surface failures.

Two files carry state between steps and across sessions — you do not hold
this state in context:

- `migration-plan.md` — the static plan (intent), at the **workspace root**.
  The **Migration Planner** writes it; the user may edit it; you dispatch
  from its `## Sequence`. Its top-level `## Project Details` **Project Name**
  row is the stable pointer to the CCS project directory
  (`project_dir = {workspace}/<Project Name>`) — see Step 5.
- `migration-report.md` — the runtime record (facts), inside `{project_dir}`
  (the CCS project directory, not the workspace root). Execution agents
  create and append to it. Its `## Migration Checklist` is the **resume
  contract**.


## Arguments

The user must provide:
- **Project path** — path to the source project directory (required; ask if not provided).
- **Target device** — a specific TI part number (required; ask if not provided).

## Sub-agent invocation

Each sub-agent has a self-contained prompt under `agents/<name>.md` (relative to
this SKILL.md). Dispatch is a **trampoline** — do **not** read the prompt file
yourself; the sub-agent reads it in its own fresh context.


To dispatch, call the `Agent` tool with `subagent_type: "general-purpose"` and a
prompt of the form:

```
Read the file `agents/<name>.md` relative to this skill's SKILL.md
(workspace path: `.claude/skills/ti-ccstudio-migrate-to-ti/agents/<name>.md`).
That file is your authoritative instructions — read it in full and follow it
exactly. The structured input bundle for this invocation is below.

--- INPUT BUNDLE ---
<bundle markdown>
--- END INPUT BUNDLE ---
```

Then parse the sub-agent's structured return value by header
(`## TargetResolution`, `## SourceSummary`, `## PlanResult`, `## StepResult` —
the last is shared by all seven Step 5 execution agents). Per-area agents
(the Configurators, Area Porter, Area Regenerator) suffix it with the area id
— `## StepResult: <area.id>` — while `project-initializer[-zephyr]` and
`build-verifier` (which aren't per-area) emit the bare `## StepResult`. Match
on the `## StepResult` prefix so either form is recognized.


**All sub-agents run sequentially. Never spawn two at once.** Execution agents
share MCP state and may touch overlapping source files; concurrent
writers corrupt artifacts.


**Dispatch failures and retries.** An errored or cut-off `Agent` call counts as
`status: failed`. Every dispatch gets one retry, same as Step 5's "Handle each
return" below; a second failure is fatal — stop and report it to the user,
leaving `migration-plan.md` / `migration-report.md` on disk for a later resume
via Step 0. Never do the sub-agent's job yourself as a substitute.

**Dispatch bundles are tiny.** Planning and execution agents read their content
from `migration-plan.md` / `migration-report.md` themselves. You pass only small
resolved values (Steps 2–4) or, for every `## Sequence` step, just `workspace`
and which `plan_step` is theirs. You do **not** slice areas, resolve guide paths,
or pass config/porting-mode flags — the planner baked all of that into the plan.
The one exception is `user_provided_descriptions`, added to a re-dispatch bundle
only by Step 5's `needs_description` handling below — a small user-authored
text blob that cannot come from the plan or report because it doesn't exist
until the user supplies it mid-run.


## Guardrail — Confirm the CCS MCP servers are connected

**Run this check first — before Step 0, before collecting arguments, before
anything else.** This skill depends on the `ccs-project` and `ccs-sysconfig`
MCP servers that CCS injects into a workspace. If either is unavailable, 
later steps fail with confusing, unrelated tool errors — catch it here instead:

Check whether your current tool list includes at least one tool named
`mcp__ccs-project__*` and at least one named `mcp__ccs-sysconfig__*`. If
either prefix is completely absent, skip straight to "Report the
guardrail failure" below.

**Report the guardrail failure.** Output this message to the user exactly as
written, then end the skill immediately — do not proceed to Step 0 or any
later step:

> **CCS AI integration not detected** — This skill needs the `ccs-project`
> and `ccs-sysconfig` MCP servers that Code Composer Studio provides, and
> they aren't reachable right now. This is usually one of:
>
> 1. **Your agent (Claude Code or similar) was started outside the CCS workspace.** 
>    CCS injects its MCP configuration (`.mcp.json`) into the workspace folder it
>    opens; that file is absent when Claude Code is started anywhere else. Open this
>    workspace from within CCS, or start Claude Code from inside that
>    workspace folder.
> 2. **CCS is not running.** These MCP servers are built into CCS and connect
>    to it — with CCS closed, they can't connect even when `.mcp.json` is
>    present. Start CCS, open this workspace there, and try again.
> 3. **AI hooks were never enabled in CCS.** From the CCS main menu, choose
>    **Help → Get Started with AI** and complete that setup, then reopen this
>    workspace.
>
> If none of these apply, this may be a bug in the CCS AI integration —
> please report it.

## Step 0 — Detect a resumable migration

**Retrospective hook (always runs first, regardless of path below).** If the
user's original request mentions "retrospective" (any case), invoke
`/ti-ccstudio-internal-retrospective` now, before branching on Resume/Start
over/Abort below.
It only sets up collection — continue this skill's steps immediately after.

Run `pwd` → `{workspace}`. Check whether `{workspace}/migration-plan.md` exists
(Bash `test -f`).

- **It does not exist** → fresh migration. Continue to Step 1.
- **It exists** → a plan is already on disk. Ask with `AskUserQuestion`:

  ```
  question: "A migration plan already exists in this workspace. Resume it?"
  options:
    - label: "Resume"
      description: "Continue from the first unfinished step in the checklist"
    - label: "Start over"
      description: "Erase the existing plan and report, then plan a fresh migration"
    - label: "Abort"
      description: "Stop here; leave the existing plan and report untouched"
  ```

  On **Resume**, skip Steps 1–4 entirely (the plan is authoritative — do not
  re-run the resolver, analyzer, or planner) and go to Step 5, computing the
  resume point from the checklist (see Step 5's resume rule). On **Start
  over**, read the existing plan's top-level `## Project Details` **Project
  Name** row and derive `{project_dir} = {workspace}/<Project Name>` (skip
  this derivation if the row or project directory is absent — the project
  was never created, so there is nothing to delete).
  Delete `{workspace}/migration-plan.md` and
  `{project_dir}/migration-report.md` (Bash `rm -f`, if present), then continue
  to Step 1 so the planner writes a fresh plan.
  On **Abort**, end the session now — do not proceed to Step 1 or
  any later step; the existing plan and report are left on disk untouched for
  a later Resume.

## Step 1 — Disclaimer, collect inputs

Output this message to the user exactly as written:

> **Migration Assistant** — The assistant will help you to migrate this project to a
> TI device. Please note that this tool is used as part of an AI-assisted process.
>
> TI does not access, collect, or store any input or output of this process. Your
> prompts, data, and results stay in your own environment and with whatever AI model
> you choose to use. You retain full ownership of your code. Any libraries or
> components remain subject to their original licenses. This tool is provided as is,
> and results won't always be correct or complete. It's up to you to review and
> confirm that any output is accurate, complete, secure, and right for your needs
> before you use or rely on it, as described further below:
>
> - Pin assignments should be confirmed against the target board schematic
> - Clock and baud rate calculations should be verified
> - Ported code should be reviewed and tested on hardware
> - The SDK documentation is the authoritative reference for API usage

If `project path` or `target device` is missing, use `AskUserQuestion` to
collect it.

## Step 2 — Resolve the target

Read CCS.md if not already done. It establishes `{ccs-ai-resources-dir}`. Derive
`{manifests_dir}` = `{ccs-ai-resources-dir}/migration-guides/manifests`.

Dispatch `agents/target-resolver.md` with `target_device`, `source_hint` (the
user's original request text), `{ccs-ai-resources-dir}`, and `{manifests_dir}`.
Wait for `## TargetResolution` (`sdk_location`, `sdk_name_version`,
`sdk_type_id`, `target_family`, `board`, `variant_candidates[]`,
`board_candidates[]`, `available_packages[]`). Apply "Dispatch failures and
retries" above on failure.

Resolve the choices the resolver could not (it cannot call `AskUserQuestion`):

1. **Variant** — if `variant_candidates[]` is non-empty, use `AskUserQuestion`
   (one option per candidate `label`) and set `target_family` to the chosen
   candidate's `family_id`.
2. **Board** — if `board_candidates[]` is non-empty, use `AskUserQuestion` (one
   option per candidate `label`) and set `board` to the chosen `board` (may be
   `null` for a bare device / custom board).
3. **Package** — if `board` is resolved (non-null) and its `AGENTS.md`
   unambiguously declares a single package, read it (via Bash/Read) and use
   that package as `target_package` without prompting. Otherwise use
   `AskUserQuestion` to present `available_packages[]` and have the user pick
   one; store as `target_package`. Note in the question that the upcoming
   assessment will identify pin-compatible packages, so the choice can still
   be changed by editing the plan.

**No-confirmation mode.** If the user's original request says to proceed
without confirmation (e.g. "proceed without asking", "no prompts",
"don't ask me"), skip `AskUserQuestion` above (and in Steps 3–4) and pick
whichever variant/board/package best fits the request and any resolved
board's `AGENTS.md`. Only fall back to `AskUserQuestion` when nothing in the
request or board data favors one option over another.

## Step 3 — Analyze the source

Dispatch `agents/source-analyzer.md` with `source_project_path`, `workspace`,
and (optional) `source_manifest_files`. Wait for `## SourceSummary`, applying
"Dispatch failures and retries" above on failure. If it instead returns
successfully with `family: unknown`, surface its `notes` and ask the user to
confirm or override the source family; on override, proceed to Step 4 with the
user's value — do not re-spawn the analyzer.

## Step 4 — Plan the migration

Dispatch `agents/migration-planner.md` with the small resolved values it needs:
`source_summary` (the `SourceSummary` block), `source_project_path`,
`target_device`, `target_family`, `target_package`, `board`, `sdk_location`,
`sdk_name_version`, `sdk_type_id`, and `workspace`.
The planner reads the manifests itself and writes
`{workspace}/migration-plan.md`. Wait for `## PlanResult`, applying "Dispatch
failures and retries" above on failure (surface `failure_reason` when
stopping).

**Present the assessment.** Read the `## Assessment` section from
`{workspace}/migration-plan.md` and print it to the user. Then use
`AskUserQuestion`:

```
question: "Proceed with the migration?"
options:
  - label: "Yes, proceed"
    description: "Initialize the TI project and start porting"
  - label: "Tweak the plan"
    description: "Drop an area, change target package or device, reorder, etc."
  - label: "Abort"
    description: "Stop here; keep the plan file for later"
```

On **Tweak**, collect the user's specific feedback (free text), then re-spawn
the Migration Planner with the same inputs plus a `user_feedback` block
describing the requested changes. (The user may also edit `migration-plan.md`
directly — either way the revised plan is on disk.) Re-read and re-print
`## Assessment`, then ask again. Do **not** re-spawn the resolver or analyzer.
Loop until the user proceeds or aborts.

## Step 5 — Execute the plan's `## Sequence`

Read the `## Sequence` from `{workspace}/migration-plan.md`. It is an ordered
list of steps; each has an **Agent** row naming the sub-agent to dispatch, and
Configure/Port steps also carry an explicit **Area** row giving that step's
`{area.id}` (e.g. `core`) — read that row rather than trying to parse the id
out of the step heading prose. Walk the steps **in order**. For each, read its
**Agent** row, its **Area** row when present, and the step heading text
(needed for the `plan_step` value in the dispatch bundle below) — the agent
reads the rest of its own step from the plan file itself — and dispatch that
agent with a tiny bundle for every dispatch below (including retries, see
"Handle each return"):


```
- workspace: {workspace}
- plan_step: "Step <n>" — <the step heading, e.g. "Step 2 — Configure core">
- retry_context: <optional> "Your previous attempt at this step did not
  complete: <prior summary>. Address this and try again."
```

`retry_context` is omitted on a step's first dispatch; present only on the one
allowed retry (see "Handle each return" below).

**Resume rule.** On a resumed run (Step 0 → Resume), first read the
**Project Name** row from `{workspace}/migration-plan.md`'s top-level
`## Project Details` and derive `{project_dir} = {workspace}/<Project Name>`.
Then read the `## Migration Checklist` in `{project_dir}/migration-report.md`
(if the report does not exist yet — the project has not been created —
nothing is done; start at the first step). Skip every
`## Sequence` step whose checklist outcome is already `- ✅`:
- `project-initializer` / `-zephyr` — done when both `SDK installed` and
  `Project imported into CCStudio IDE` are ticked.
- `sysconfig-configurator` / `devicetree-configurator` for area `X` — done when
  `Area X — configured` is ticked.
- `area-porter` / `area-regenerator` for area `X` — done when `Area X — ported`
  is ticked.
- `build-verifier` — done when `Build passing` is ticked.

Start at the first not-yet-complete step. The execution agents are idempotent
(they detect an existing project, an already-configured `.syscfg`, an
already-ported file), so re-dispatching a partially-complete step is safe.

**Handle each return.** After dispatching a step and receiving `## StepResult`:

1. Print `summary` to the user.
2. **Excluded-file description gap (Area Porter / Area Regenerator only).**
   If the returned `StepResult` has a non-empty `needs_description` list, ask
   the user directly — one `AskUserQuestion` free-text prompt per entry —
   along these lines: "The migration couldn't determine what `<file>`
   (`<peripheral>`) does from documentation alone. Can you describe its
   behavior in your own words? I will not read the original file — only your
   description will be used." Collect each answer, then re-dispatch the same
   agent for this same step with `user_provided_descriptions: [{file,
   description}, ...]` added to the bundle (`workspace`/`plan_step` as
   usual; omit `retry_context` — this is not a failure retry and does not
   count against the one-retry budget in steps 4-5 below). Go back to step 1
   with the new `StepResult`. Repeat until a returned `StepResult` for this
   step carries no more `needs_description` entries, then continue to step 3.
3. Check the mapped checklist item(s) for this step's agent type (same
   mapping as the "Resume rule" list above) with the `Grep` tool against
   `{project_dir}/migration-report.md` (derive `{project_dir}` from the
   plan's **Project Name** row, same as the "Resume rule" above, if not
   already derived this run) — e.g. pattern `✅ Area {area.id} —
   configured` — rather than reading the whole file, which only grows as
   the migration proceeds.
   The step is **complete** only if
   `status: success` **and** every mapped line greps as ticked (`- ✅`, not
   `- ⬜`).
4. If complete → continue to the next `## Sequence` step.
5. If not complete (either `status: failed`, or `status: success` but the
   checklist disagrees) and this is the step's **first** attempt this run →
   re-dispatch the same agent with `retry_context` set to the returned
   `summary` (or, if the checklist simply didn't match a `success` status, a
   synthesized note: "you returned success but did not tick <item>"). Go to 1.
6. If not complete on the **second** attempt → stop the walk; the failure
   `summary` already printed in step 1 is the surfaced failure. The plan and
   report stay on disk for a later resume via Step 0.

## Step 6 — Report, then hand off to hardware testing

When the `build-verifier` step returns `status: success` (its `summary` —
already printed in Step 5 — states the built binary's path), then:

1. Set `{project_dir}/migration-report.md`'s top **Status:** line to
   `🔵 **BUILD VERIFIED — HARDWARE TESTING PENDING**` (Edit, exact-string).
   Leave `- ⬜ Tested on hardware` **unticked** — a separate skill owns
   that tick.
2. Output to the user:

   > "Migration report saved to `migration-report.md` inside the CCS project
   > folder. Open it in the CCStudio IDE Explorer panel to review the
   > peripheral mapping, API changes, and remaining checklist items.
   >
   > Before distributing this port, review the report's license-related
   > sections — detection is header-pattern matching, not a legal review:
   >
   > - **`## License Notes`** — files flagged under non-permissive or
   >   unlicensed terms.
   > - **`## Code Porting Notes` → `⚠️ Clean-room regeneration`** —
   >   peripherals reconstructed from SDK docs (sometimes aided by a
   >   description you gave) because the original file was vendor-restricted
   >   and excluded; unverified against the original, so give these extra
   >   scrutiny and hardware testing.
   > - **`## Code Porting Notes` → `🟡 Ported/Regenerated (rewrite required
   >   before distribution)`** — example/glue code kept as-is, but its
   >   license requires a rewrite before distribution or production use;
   >   budget time to replace it before shipping.
   >
   > To verify this migration on hardware, run `/clear` to start a fresh
   > session, then invoke the `ti-ccstudio-test-migration` skill in this
   > workspace — it will help you test and verify the migration on real
   > hardware. Clearing first keeps this session's migration context from
   > crowding out the hardware-testing work."

If the migration ends mid-session before a successful build, the plan and
partial report remain on disk; a later invocation resumes via Step 0.
