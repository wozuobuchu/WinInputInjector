# Validation

## Single-row footer without Clear — 2026-09-28

- Debug x64 and Release x64 application builds, injection regression tests, and
  updated native UI tests passed. The Debug UI test retains the previously noted
  C4702 warning in `inject_thread.hpp`.
- UI tests cover every short caption, zero/partial failures with and without an
  error code, two-line details, collapse on a new running state, anchored progress,
  tooltip registration/content, font sizes, and 100000-unit source text at
  96/120/144/192 DPI and minimum/default/large/restored sizes. These DPI changes
  are synthetic; physical cross-monitor movement was not tested.
- Live default, 720x480 minimum, maximized, and restored layouts were inspected.
  No Clear control remains; the right-aligned status shares the parameter row.
  The README screenshot shows the new default empty window.
- Live Tab and Shift+Tab wrap between text/gap/chunk; gap 0 skips Chunk. Enter
  inserts a newline without sending, and Ctrl+A/Delete clears the source while
  retaining the latest status. Hover help was inspected on disabled Chunk and
  on running/completed status, including the target-not-verified explanation.
- F2 with empty text displays "No text". A 300-unit Chinese/English/emoji sample
  matched exactly in the app's EDIT control after a paced send. Another F2 during
  that run did not restart it; the live percentage advanced to "Sent to Windows".
- Failure/cancellation presentation was checked through test-owned reports and
  controls; no external target failure was deliberately induced.

## Compact native UI — 2026-09-28

- Debug x64 and Release x64 application builds, existing injection recorder tests,
  and the new native UI layout checks passed. The Debug UI test build reports an
  existing C4702 warning in `inject_thread.hpp`; application builds succeeded.
- Actual default, minimum 720x480, maximized, and restored windows were inspected.
  Parameters and Clear fit on one row. The right-aligned hint repaints correctly
  when shrinking; there is no mode selector or send button.
- Live checks passed for Tab/Shift+Tab, skipping disabled chunk at gap 0, Enter
  inserting a newline without starting injection, Clear, and chunk correction
  from 99999 to 32768 on focus loss. Defaults remain 1000 us and 128.
- Empty F2 showed "No text to send." A controlled 300-unit Chinese/English/emoji
  sample sent into the app's EDIT control matched in full after completion.
  F2 pressed again during this paced run did not restart or duplicate the text.
- UI tests cover 100000-unit source text, zero-gap disabling, numeric correction,
  font sizes and control bounds at 96/120/144/192 DPI, and full two-line rendering
  of running/completed/cancelled/maximum-length failure messages. DPI changes are
  synthetic messages to a test-owned window, not physical cross-monitor testing.
- The README screenshot was captured from the new native window. No browser or
  third-party target verification was added; sender semantics remain unchanged.

## Adjustable chunks — 2026-09-28

- Debug x64 and Release x64 application builds and recorder tests passed.
- Recorder tests cover chunk 1/128/32768, invalid/empty/oversized values, full and
  short tails, surrogate boundaries, exact whitespace/newline event preservation,
  100000-unit chunked samples, failures in later batches, cancellation, and gaps
  measured after whole batches complete. Gap 0 still takes exactly one call for
  every tested chunk setting, including invalid numeric values.
- Live UI confirmed default chunk 128 and gap 1000 us. Entering 99999 and leaving
  the field displayed 32768; clearing the field and leaving it restored 128.
  Gap 0 disabled the chunk input; switching back to 1000 retained its value.
- Default and minimum 720x480 layouts were inspected: both control rows and the
  status line fit without overlap.
- In the app's Win32 EDIT control, a 1011-unit Chinese/code/emoji sample ultimately
  matched in full at chunk 1 and 128, with gap 1000 us. UI-observed completion was
  roughly 13 seconds in both runs, so this experiment did not establish a stable
  end-to-end speedup. Chunk 128 initially showed sent status before the complete
  received text was visible; a subsequent read matched. Sender wait/call counts
  decrease, but target processing and observation overhead still affect elapsed
  time. The sample was below the accessibility provider's 4096-unit read limit
  and did not rely on whitespace normalization for comparison.
- No browser or original third-party page test was performed for this change.

## Original paced sender — 2026-09-28

- Application: Debug x64 and Release x64 built successfully with MSVC v145.
- Recorder tests: Debug x64 and Release x64 passed. Coverage includes exact mixed
  Unicode/control-character event sequences, every partial cut through a surrogate
  pair, explicit and missing Windows errors, no retry, monotonic complete-character
  progress, interrupted waits, slow-send pacing, and the 100,000-unit single batch.
- Live Win32 EDIT: 0, 1000, and 10000 us runs completed. The accessible text matched
  the baseline for the mixed Chinese/English/code/emoji sample. Accessibility folds
  whitespace, so this comparison does not prove byte-for-byte newline preservation.
- Live 100,000-unit Win32 EDIT batch at 0 us completed, with the visible status
  changing from sending to sent. The readable prefix matched. The accessibility
  provider caps text at 4096 units, so full output equality was not verified. The
  unthrottled batch took several minutes to finish processing in the edit control.
- Near-limit paced live runs were not performed. Browser textarea/contenteditable
  tests were blocked: the browser tool rejected the local `file://` fixture under
  its URL security policy. The original third-party webpage was not modified or
  tested. `browser_input.html` and the manual procedure remain available for reruns.

These results establish the sender's behavior and error handling; they do not
establish that every browser editor can reliably handle a 1 ms interval.
