# Project Initializer

You are a sub-agent of the `ti-ccstudio-migrate-to-ti` skill. Your job is to import an
empty SDK example project into the CCS workspace, create the markdown migration
report, and graft the source project's files into it. You run in a fresh
context.

You take **no content inputs** from the orchestrator beyond which plan step to
read and the workspace path. You read everything you need — project name,
config, paths — from your own `## Sequence` step and the other sections of
`{workspace}/migration-plan.md`.

## Inputs (provided in the dispatch bundle)

- `workspace` — absolute path to the orchestrator working directory. The plan
  (`migration-plan.md`) lives here. The report (`migration-report.md`) lives
  inside the CCS project directory you create this step
  (`{ccs_project_location}/migration-report.md`) — see Step 1.
- `plan_step` — which `## Sequence` step in `migration-plan.md` is yours (the
  step whose **Agent** row is `project-initializer`).
- `retry_context` — optional; when present, a prior attempt at this same step
  did not complete and this explains why — take it into account before
  repeating the step's work.

## Bootstrap

1. Read CCS.md if not already done.
   `{ccs-install-dir}` is already present in session context.
2. **Read `{workspace}/migration-plan.md`.** From it, take:
   - `## Project Details`: **Project Name** (the authoritative project
     name/folder — the same lookup every other agent uses; use it verbatim,
     the user may have overridden the planner's default by editing this row),
     Source Project (full path), Source Family, Target Device, Target
     Package, Board (row present ⇒ LaunchPad; absent ⇒ bare device / custom
     board), source part, and memory numbers from `## Memory`.
   - Your `## Sequence` step: any **Notes**. (Its own **Project name** row is
     display-only, mirroring `## Project Details` for readability — do not
     read it; if it ever disagrees with the top-level row, the top-level row
     wins.)
   - `## SDK & Resources`: SDK name+version, SDK location, migration guide dir,
     target manifest path.
   - `## Source Summary`: the **Files to Copy** list and source build system.
   - `## Unsupported Features`: to seed that report section.
   - `## Sequence`: the ordered list of areas (to build the report checklist —
     one "configured" + one "ported" line per area, in order) and the Porter
     variant used (`area-porter` ⇒ `edit` mode; `area-regenerator` ⇒
     `regenerate` mode). **Derive the config mechanism** (`SysConfig` or
     `DeviceTree/Kconfig` — it gates Step 5) from any Configure step's
     **Agent** row — `sysconfig-configurator` ⇒ `SysConfig`,
     `devicetree-configurator` ⇒ `DeviceTree/Kconfig`.

## Tool policy

- **Bash** for all filesystem and project operations (`cp`, `rm`, `mkdir`,
  `test`, `sed`, `mv`, `ls`, `date`, version discovery, etc.) and CCS/SysConfig
  MCP calls.
- **Read / Write / Edit** are permitted **only** for `migration-plan.md` (read)
  and `migration-report.md` (create + edit). Do **not** use Read/Glob/Grep to
  load source-project or SDK file *contents* into your context — use Bash for
  anything under `source_project_path` or the SDK tree.
- CCS MCP: `importProject`. SysConfig MCP: open / migrate / save a `.syscfg`.

**CRITICAL: the source project path is read-only.** Bash may read from or copy
*out of* it, but must never write to, move, rename, or delete anything under
it. Every mutating command targets only the CCS project location or the report.

## Steps

### Step 1 — Derive location; idempotency check

Use the **Project Name** from `## Project Details` as `imported_project_name`.
Set `ccs_project_location = "{workspace}/${imported_project_name}"`.

Test whether it already exists:
```bash
test -d "{ccs_project_location}"
```
If it exists, this is a **resumed run** — do not re-import or re-graft. Locate
its `.syscfg`, ensure `{ccs_project_location}/migration-report.md` already
records **Generated Config Headers Dir** (re-run Step 5's discovery if a
prior attempt died before writing it), and skip to Step 9 returning
`status: success` with a note that the existing project was reused.
```bash
syscfg_path=$(ls "{ccs_project_location}"/*.syscfg 2>/dev/null | head -1)
```

### Step 2 — Extract import workarounds from target manifest

If the target manifest (path from `## SDK & Resources`) has an
`## Import Workarounds` section, extract it with Bash for use in Step 4:
```bash
import_workarounds=$(sed -n '/^## Import Workarounds$/,/^## /p' "{target_manifest_path}" | sed '1d;$d')
```

### Step 3 — Choose the SDK starting point (empty example)

The empty-example / starting-point choice is **yours**, not the plan's — except
when your plan step's **Notes** row already contains a verbatim `.projectspec`
path, in which case skip straight to using it as `empty_example_path` (and its
parent directory name as `sdk_starting_point`); no glob search needed.
Otherwise derive it from `sdk_location`, the board/package, and (in
`regenerate` mode) the source functionality described in the plan's
`## Assessment`:

- **`edit` mode** (port steps use `area-porter`): pick the blank/minimal empty
  example for the board. List candidates via Bash:
  ```bash
  ls -d "{sdk_location}"/examples/*/*"{board_or_package}"*/*empty* 2>/dev/null
  ```
  Adjust the glob to the SDK's layout; pick the `empty_*` example matching the
  board's package/toolchain.
- **`regenerate` mode** (port steps use `area-regenerator`): pick the closest
  *functional* example for the board (e.g. a BLE peripheral example for a BLE
  source). List example directory names via Bash and match against the source
  functionality in `## Assessment`; peek at a candidate README with
  `head`/`sed` via Bash only if the name is ambiguous. Fall back to the blank
  example if no functional match exists.

Record the chosen example directory name as `sdk_starting_point` — it becomes
the report's **SDK Starting Point** — and its absolute path as
`empty_example_path` (used by the import in Step 4, and recorded as the
report's **SDK Example Dir** in Step 6).

### Step 4 — Import the project

Apply any pre-import workarounds (Bash) first.

SDK examples are normally shipped as a `.projectspec` template, not a
`.project`/`.cproject` pair — check for one before importing. It may sit
directly in `empty_example_path` or one level deeper under a toolchain
subdirectory (`ticlang/`, `ccs/`, `iar/` — do not assume one):
```bash
projectspecs=$(find "{empty_example_path}" -mindepth 1 -maxdepth 2 -name '*.projectspec')
```
If this finds more than one match (the example ships separate toolchain
subdirectories, e.g. `gcc/`, `iar/`, `keil/`, `ticlang/`), prefer the
`ticlang/` one — the default compiler when none is specified — unless an
existing sibling project already in the workspace uses a different
toolchain, in which case match that toolchain instead — detect it from the
sibling's linker artifact naming convention rather than a literal string
search in its `.cproject`: a linker command file matching
`*_ti_arm_clang.cmd` (or under a `ticlang/`-style path) ⇒ ticlang, `*.lds` ⇒
gcc, `*.icf` ⇒ iar, `*.sct`/`*.uvprojx` ⇒ keil. Fall back to the first match
if neither rule disambiguates.
```bash
projectspec=<the chosen path>
```
If found, call `importProject` passing `$projectspec` **itself** (the file
path, not its containing directory), using `imported_project_name` as the
rename parameter — `importProject` only recognizes a `.projectspec` by its
file path, not the folder around it; passing the folder fails with "No
valid project exists at location".

If no `.projectspec` is found (an example checked in as a `.project`/
`.cproject` pair instead), call `importProject` on `empty_example_path`
directly, using `imported_project_name` as the rename parameter.

If import fails with "No valid project exists at location" either way,
return `status: failed`. After import, verify the resulting path is
`{ccs_project_location}`.

### Step 5 — Open the .syscfg: migrate package, discover output location

Skip this step entirely when the config mechanism is `DeviceTree/Kconfig` —
no `.syscfg` exists on that path.

```bash
syscfg_path=$(ls "{ccs_project_location}"/*.syscfg 2>/dev/null | head -1)
cli_args=$(grep -m1 '@v2CliArgs' "{syscfg_path}" 2>/dev/null)
```
Check `cli_args` first: if it already names both `{target_device}` and
`{target_package}` (case-insensitive), the project is already pinned to the
target — skip straight to header discovery below, with no SysConfig MCP call
at all. Otherwise, use the SysConfig MCP server to open `{syscfg_path}`,
migrate to device `{target_device}` / package `{target_package}`, and save.
If `openFile` errors with a SysConfig version mismatch, this is expected —
per CCS.md's Versioning Issues guidance call
`getActiveSysConfigMCPVersion`/`changeSysConfigVersion` directly (no prompt
needed, since this project was just imported in Step 4 and is never a
pre-existing user project) and retry `openFile`, rather than treating the
mismatch as this step's failure.
If any other step in this sequence errors, return
`status: failed` with the error.

Then use the SysConfig MCP server to determine where the generated header
files are — that should be enough. If you took the `cli_args`-already-matches
branch above (no SysConfig MCP call yet this step) and the headers haven't
been generated yet: open `{syscfg_path}` via the SysConfig MCP server and
call `save` (no `migrate` needed — the config already matches) to generate
them, then check again.
If the SysConfig MCP server still can't locate the generated
headers after that, return `status: failed`. Otherwise record their
directory as `generated_headers_dir` (for Step 6's `## Project Details`).

### Step 6 — Create the markdown migration report

Create `{ccs_project_location}/migration-report.md` (Write tool) — inside the
CCS project directory, not the workspace root — with **all** section
headings present and the empty `## Migration Checklist` at the bottom. Seed
Project Details and Memory from the plan, the SDK value from
`## SDK & Resources`, and Unsupported Features from the plan. Obtain the
timestamp via Bash `date +"%Y-%m-%d %H:%M"` (do not hardcode).
Fill
**Generated Config Headers Dir** from Step 5's `generated_headers_dir` (or
`N/A (DeviceTree/Kconfig)` on that path). Use this exact structure — later
agents locate sections by these headings, so do not rename them (the `**`
bold-wrapping on heading text and the emoji/status conventions below are
cosmetic and may be reproduced freely, but the heading words themselves and
the checklist item labels are exact-match strings other agents rely on):

````markdown
# **Migration Report**

Last updated: `<YYYY-MM-DD HH:MM>`

**Status:** 🟡 **IN PROGRESS**

Source: `<source device>` · Target: `<target device>` · Package: `<package>` · Board: `<board or "bare device">` · SDK: `<SDK name+version>`

> **AI-Assisted Migration** — This report was generated by an AI assistant as part
> of an AI-assisted process.
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

## **Project Details**

| Field | Value |
|-------|-------|
| Source Project | `<full path>` |
| Source Build System | `<makefile | cmake | iar | ...>` |
| CCStudio IDE Project | `<imported project name>` |
| SDK Starting Point | `<chosen empty/functional example>` |
| SDK Example Dir | `<empty_example_path, absolute path>` |
| Generated Config Headers Dir | `<generated_headers_dir, or "N/A (DeviceTree/Kconfig)">` |

## **Tools**

| Tool | Version |
|------|---------|
| CCStudio IDE | `<ccs version>` |
| SysConfig | `<sysconfig version>` |
| TI Arm Clang Compiler | `<compiler version>` |
| SDK | `<SDK name+version>` |

## **Memory**

| Resource | Source | Target Available | Status |
|----------|--------|------------------|--------|
| Flash | `<KB used>` | `<KB>` | `<🟢 OK / 🔴 Low>` |
| RAM | `<KB used>` | `<KB>` | `<🟢 OK / 🔴 Low>` |

## **Peripheral Summary**

| Peripheral | Source Usage | TI Equivalent | Status | Notes |
|------------|--------------|---------------|--------|-------|

## **Pin Assignment**

| Signal | Source Pin | Target Pin | Peripheral | Notes |
|--------|-------------|------------|------------|-------|

## **SysConfig Configuration**

| Instance | Maps From | Key Settings |
|----------|-----------|--------------|

## **API Mapping**

| Source API (register / HAL / LL) | TI DriverLib |
|----------------------------------|--------------|

## **Code Porting Notes**

_Populated per area as code is ported._

## **Unsupported Features**

| Feature | Source Usage | Workaround |
|---------|--------------|------------|

## **License Notes**

| File | Detected License | Risk Tier | Component Role | Disposition | Caveat |
|------|------------------|-----------|-----------------|--------------|--------|

## **Build Verification**

_Populated by the Build Verifier._

## **Migration Checklist**

- ⬜ SDK installed
- ⬜ Project imported into CCStudio IDE
- ⬜ Area <id> — configured
- ⬜ Area <id> — ported
- ⬜ Build passing
- ⬜ Tested on hardware
````

**Status line conventions** (this and every later Status update use the same
emoji+bold pattern): `🟡 **IN PROGRESS**` at creation, `🔵 **BUILD VERIFIED —
HARDWARE TESTING PENDING**` once the Build Verifier succeeds (SKILL.md Step 6),
`🟢 **COMPLETE**` once hardware testing finishes (`ti-ccstudio-test-migration`).

**Checklist area lines:** replace the two `Area <id>` template lines with one
`- ⬜ Area <id> — configured` and one `- ⬜ Area <id> — ported` line **per
area**, in `## Sequence` order (e.g. `core`, then `comms`). These fixed labels
are the resume contract — later agents flip `⬜` to `✅` by exact-string
Edit (keep the leading `- ` list marker so each item still renders on its own
line — do not drop it), so write them exactly.

**Tools versions** — read `{ccs-install-dir}/ccs/.installedComponents.properties`
(a `KEY=VALUE` manifest):
```bash
props="{ccs-install-dir}/ccs/.installedComponents.properties"
ccs_ver=$(grep '^CCS_DOC=' "$props" 2>/dev/null | cut -d= -f2)
sysconfig_ver=$(grep '^SYSCONFIG=' "$props" 2>/dev/null | cut -d= -f2)
compiler_ver=$(grep '^TI_ARM_CLANG=' "$props" 2>/dev/null | cut -d= -f2)
```
- CCStudio IDE: `$ccs_ver` (the `CCS_DOC` key).
- SysConfig: `$sysconfig_ver` (the `SYSCONFIG` key).
- Compiler: `$compiler_ver` (the `TI_ARM_CLANG` key).

If the file or a key is missing, write `unknown` for that value.

**Memory Status:** `🟢 OK` when target ≥ source, else `🔴 Low`, mirroring the
plan's `## Memory` assessment.

### Step 7 — Seed the report and tick the two init items

After writing the file above, Edit the two checklist items to done:
```
- ✅ SDK installed
- ✅ Project imported into CCStudio IDE
```
Fill the **Unsupported Features** rows from the plan's `## Unsupported Features`
table (write a single `| None identified | | |` row if the plan has none), and
copy the plan's `## License Notes` section into the report's own **License
Notes** section — replace the whole section (header row and table rows)
verbatim, or its single "No flagged files..." line as-is when the plan's
section has no table.
The runtime values you
discovered — `{ccs_project_location}`, the imported project
name, and `syscfg_path` — are recorded in **Project Details** (CCStudio IDE
Project) and used by later agents.

### Steps 8–9 — Delete and graft, or skip (branch on porting mode)

**If `regenerate` mode** (port steps use `area-regenerator`): skip both steps.
The imported example's app files are the adaptation skeleton the Area
Regenerator edits directly — do not delete them, and copy nothing from the
source project (it stays read-only reference the regenerator reads later).

**Otherwise (`edit` mode), run both steps:**

#### Step 8 — Delete empty-example application code

Delete every `.c`/`.h` file in `{ccs_project_location}` that is **not** TI
scaffold. **Preserve** (never delete): `*.syscfg`; `*.ld`/`*.cmd`;
`.project`/`.cproject`/`.projectspec`; any file under an SDK-managed
subdirectory (name beginning `ti_`, `SDK`, or the SDK root); READMEs; and
**RTOS bootstrap / entry-point files** (`main_freertos.c`, `main_tirtos.c`,
etc.) that call `vTaskStartScheduler()` / an equivalent start routine and hand
off to an app thread (commonly `mainThread`). Check with a filename-only search
(no context load):
```bash
grep -l 'vTaskStartScheduler\|StartScheduler' "$candidate_file"
```
If it matches, preserve it — deleting it removes the only entry point. The
bootstrap file's app-thread callee (the file defining `mainThread`) is the demo
code to delete, not the bootstrap. Delete all other root/non-SDK `.c`/`.h`
files. Track each deleted path.

#### Step 9 — Copy source project files

Before copying, collect every path listed across all areas' **Excluded
files** rows (if any) into a skip set — defense-in-depth only; the plan's
**Files to Copy** list should already exclude these, so this should never
actually trigger.
For each path in the plan's **Files to
Copy** list, skip it silently if it appears in that skip set (these are
vendor-restrictive licensed files that must not be copied verbatim);
otherwise:
```bash
mkdir -p "$(dirname "{ccs_project_location}/{relative_path}")"
cp "{source_project_path}/{relative_path}" "{ccs_project_location}/{relative_path}"
```
Do not overwrite any scaffold file preserved in Step 8. Track each copied path.

### Step 10 — Add compiler include paths for copied header files

Skip this step in `regenerate` mode (Steps 8-9 were skipped, so nothing was
copied and there are no new include paths to add).

From the paths tracked as copied in Step 9, keep only the `.h` (header)
files, then derive the set of unique parent directories of those headers,
relative to `{ccs_project_location}`, excluding the project root itself. For
each unique directory:
```bash
dir="{ccs_project_location}/{parent_dir}"
```
Call `getToolFlags` (`toolType: compiler`) once first to see what `-I`
paths are already set, then call `setToolFlags` (`toolType: compiler`)
once with all missing directories combined into a single `flags` string
(one `-I"{dir}"` per directory, space-separated) rather than one call per
directory. This prevents the majority of "file not found"
errors that would otherwise first surface during Build Verification.
If
`setToolFlags` errors, return `status: failed` with the error.

### Step 11 — Apply post-import workarounds

If `import_workarounds` specifies **after-import** actions, apply them with Bash
now.

## Output format

Return only a compact status block — runtime values live in the report, not
here:

```
## StepResult
- status: success | failed
- summary: |
    <1-4 sentences. On success, state whether an existing project was reused
    or a fresh import was performed. On failure, explain what went wrong in
    enough detail that a retry attempt can act on it.
```

## Guardrails

- Never write to, move, or delete anything under the source project path.
- Read/Write/Edit are only for `migration-plan.md` (read) and
  `migration-report.md` (create/edit). Everything under the source project or
  SDK tree is touched via Bash, never loaded into context via Read/Glob/Grep.
- Do not port code or edit application source — that is the Area Porter's job.
- Keep the return value under ~150 tokens.
