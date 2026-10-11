# Instrument Rules

## Capability verbs

The test plan expresses every instrument action as one of these verbs,
naming the instrument channel and the board pin it is wired to:

| Verb | Meaning |
|---|---|
| `capture-logic(channel)` | Record digital levels / edges on a logic-capture channel. |
| `capture-analog(channel)` | Record an analog voltage waveform on an oscilloscope input. |
| `measure-frequency(channel)` | Measure the frequency / period of a signal on a channel. |
| `generate-signal(freq, amp, offset)` | Output an analog waveform (frequency, amplitude, DC offset) on a waveform-generator output. |
| `drive-digital(pin, level \| pattern)` | Drive a static digital level or a bit pattern on a digital output channel. |

`generate-signal` and `drive-digital` are **output verbs** — they put a
voltage onto a board pin, so their feasibility also depends on the target's
IO voltage (see "IO-voltage rule").

## Research rules

- **Every claim you write must come from a source you fetched during this
  run.** Use whatever web search / fetch tool is available in your
  environment. Prefer vendor documentation (SDK reference manuals,
  programming guides, the official package repository / registry page)
  over forums and blog posts. Record every source URL you rely on.
- **If no web tool is available, or the one you try fails** (errors, is
  blocked, returns nothing usable) before you can establish what you were
  asked to research → stop and return `status: failed` with a
  `failure_reason` stating exactly which tool you tried and what went wrong
  (or that no web tool is available). **Never fall back to answering from
  your own knowledge.**
- If the interface is established but a specific verb, limit, or install
  step cannot be confirmed by any fetched source, record it as not feasible /
  unknown with the note `not confirmed by any fetched source` — do not fill
  the gap by guessing.
- Record the license of every component (e.g. MIT, proprietary
  free-of-charge, commercial). A library's license and the license of the
  vendor runtime it wraps can differ — record each separately.
- **A verb is realizable only through something the generated code can call
  directly:** a native library binding, a foreign-function interface (FFI)
  to the vendor's shared library, a small subprocess bridge in another
  language that has a binding, a vendor command-line tool, or an instrument
  protocol (e.g. SCPI over VISA / USBTMC / TCP). Never realize a verb through
  an MCP server — the generated project has no MCP client at runtime.

## IO-voltage rule

An output verb (`generate-signal`, `drive-digital`) is **feasible only if
the instrument can be configured to keep its output within 0 V …
`Target IO voltage`** (the value in the plan's `## Fixed Wiring Setup`).
Record, per output, the instrument's output range and whether its digital
logic level is configurable:

- An analog output with a configurable amplitude/offset (e.g. a ±5 V
  waveform generator into a 3.3 V target) is feasible — the generated code
  clamps each request to the limit.
- A digital output whose high level is fixed above `Target IO voltage`
  (e.g. fixed 3.3 V logic into a 1.8 V target) is **not feasible**; say so
  in the verb's notes.

An instrument profile is written before the plan has a target IO voltage:
it records the output limits and writes an output verb's `Feasible` value as
a condition on the target IO voltage where one applies (e.g. `yes, only when
Target IO voltage >= 3.3 V`). The instrument integrator applies the rule
against the plan's value.

## Profile format

An **instrument profile** describes what one instrument can do,
independent of the language the generated project uses. Every profile has
these sections:

````markdown
# <Vendor> <Model>

Aliases: <comma-separated names / abbreviations / model variants a user might type>
Instrument class: <oscilloscope | logic analyzer | signal generator | mixed-signal>

## Software prerequisites

| Component | Purpose | License | Detect | Install source | Install method | Auto-installable? |
|---|---|---|---|---|---|---|

## Language bindings

| Language | Approach (native / FFI / bridge) | Package | Install | License | Notes |
|---|---|---|---|---|---|

Other languages: <short note on other bindings, or "none found">

## Channels

| Function | Channel names | Notes |
|---|---|---|

## Wire colours

<How a user finds each pin on the instrument's leads, then the table. When
no fetched source documents a lead colour code, write only the line
`No wire colour code confirmed by any fetched source.`>

| Pin | Wire colour | Notes |
|---|---|---|

## Output limits

| Output | Range | Logic level configurable? | Notes |
|---|---|---|---|

## Verbs

| Verb | Feasible | <calls column(s)> | Notes |
|---|---|---|---|

## Pitfalls

- <device-open exclusivity, required settle times, calibration, sample-rate / buffer limits, etc.>

## Sources

- <URL>
````

- `Detect` is a read-only check for whether the component is installed
  (e.g. `python -c "import <package>"`, the vendor shared library's
  documented install path). A profile describes the check; it does not
  record a result.
- `## Verbs` has one row for **every** capability verb. `Feasible` is
  language-independent — whether the instrument and an interface the
  generated code can call directly can realize the verb: `yes` or `no`,
  optionally qualified (e.g. `yes (derived)`, `yes (clamped)`,
  `yes, only when Target IO voltage >= 3.3 V`). The calls columns name
  concrete functions / commands: one column per language covered in
  `## Language bindings` (e.g. `Python`, `JavaScript`), or a single
  `Interface calls` column naming the vendor API's own functions /
  commands.
- `## Wire colours` covers instruments whose probe leads or flywires are
  colour-coded. `Pin` uses the names from `## Channels`, plus `GND` for the
  ground leads. When one colour is used on more than one pin, say so above
  the table and name the label printed on the instrument that tells those
  pins apart. Mark any pin whose colour no fetched source confirms as
  `not confirmed by any fetched source`.
- `## Language bindings` may list more than one row per language when more
  than one approach is viable, marking the recommended one in `Notes`. It
  may be empty when no fetched source names a binding.
