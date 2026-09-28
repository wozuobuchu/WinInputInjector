# Validation — 2026-09-28

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
