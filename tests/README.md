# x64 injection checks

From a Visual Studio developer terminal, build and run:

```powershell
msbuild tests/injection_tests.vcxproj /p:Configuration=Debug /p:Platform=x64
./x64/Tests/Debug/injection_tests.exe
msbuild tests/injection_tests.vcxproj /p:Configuration=Release /p:Platform=x64
./x64/Tests/Release/injection_tests.exe
msbuild tests/ui_layout_tests.vcxproj /p:Configuration=Debug /p:Platform=x64
./x64/UiTests/Debug/ui_layout_tests.exe
msbuild tests/ui_layout_tests.vcxproj /p:Configuration=Release /p:Platform=x64
./x64/UiTests/Release/ui_layout_tests.exe
```

The tests replace `SendInput` with a recorder. They check exact Unicode event order,
surrogate boundaries, 100,000-unit input, one-call zero interval, short sends with
and without an error code, no retry, progress, cancellation during waits, and
minimum gaps measured from send completion (including slow sends).
They never inject input into another application.

The UI checks require the same Boost include setup as the application. They create
an unshown native window and deliver synthetic DPI changes at 96, 120, 144, and
192 DPI. They check control bounds at minimum/default/large/restored sizes, font
scaling, short status captions, two-line failure details, expansion/collapse,
tooltip contents, 100,000-unit source text, numeric correction, and disabled chunk
state. This does not replace a physical cross-monitor DPI test.

Chunk coverage includes parsing/defaults/clamping (1-32768, default 128), every
zero-interval setting taking one call, full and short final batches, intact
surrogate pairs counted as one character, lone surrogate preservation, the maximum
131072-event batch of 32768 supplementary characters, partial failures in later
batches, cancellation before the next batch, and 100000-unit chunked text at sizes
128 and 32768. The chunk-1 case uses a smaller sample to avoid long timed tests.

For live checks, run the x64 app, load a known mixed Chinese/English code sample,
and compare the resulting text at intervals 0, 1000, and 10000 us in a Win32 EDIT
control and both editors in `browser_input.html`. Compare CRLF/LF separately;
these are sent unchanged and editors may discard or normalize them. Include emoji
and repeated text approaching the 100,000-unit limit. Paced runs at that limit can
take minutes, particularly at 10 ms. Check both app status and actual received text:
100% only means Windows accepted all events. Do not use a live submission form
for synthetic test text.

For chunk comparisons, keep the batch gap at 1000 us and test chunk sizes 1 and
128 on the same sample. Wait for both the completion status and the received text
to settle. Also check the default 128, empty input reverting to 128, values above
32768 clamping on focus loss, and the chunk field being disabled (with its value
retained) at gap 0. At the minimum 720x480 window size, parameters and the short
status should fit on one row without overlap or clipping. Failure details expand
below this row and collapse on the next run. Check maximize/restore, Tab and
Shift+Tab cycling between text/gap/chunk (skipping disabled chunk), Enter inserting
a newline, Ctrl+A/Delete clearing the text, and F2 as the only send entry point.
Repeated F2 during a task must not restart it. Hover over each parameter and the
status; parameter help must also work while Chunk is disabled.
