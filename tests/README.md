# x64 injection checks

From a Visual Studio developer terminal, build and run:

```powershell
msbuild tests/injection_tests.vcxproj /p:Configuration=Debug /p:Platform=x64
./x64/Tests/Debug/injection_tests.exe
msbuild tests/injection_tests.vcxproj /p:Configuration=Release /p:Platform=x64
./x64/Tests/Release/injection_tests.exe
```

The tests replace `SendInput` with a recorder. They check exact Unicode event order,
surrogate boundaries, 100,000-unit input, one-call zero interval, short sends with
and without an error code, no retry, progress, cancellation during waits, and
minimum gaps measured from send completion (including slow sends).
They never inject input into another application.

For live checks, run the x64 app, load a known mixed Chinese/English code sample,
and compare the resulting text at intervals 0, 1000, and 10000 us in a Win32 EDIT
control and both editors in `browser_input.html`. Compare CRLF/LF separately;
these are sent unchanged and editors may discard or normalize them. Include emoji
and repeated text approaching the 100,000-unit limit. Paced runs at that limit can
take minutes, particularly at 10 ms. Check both app status and actual received text:
100% only means Windows accepted all events. Do not use a live submission form
for synthetic test text.
