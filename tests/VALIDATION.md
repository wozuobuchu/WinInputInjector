# Validation

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
