# NcML ICU Date Format Alternatives Log

## 2026-05-19 09:59:51 MDT

Prompt:

> The NcML handler is located in modules/ncml_module. Currently this code builds except for one error:
>
> ScanElement.cc:55:10: fatal error: 'unicode/smpdtfmt.h' file not found
>
> The code uses smpdfmt.h from the ICU library to support the same or virtually the same date syntax as Java supports. NcML XML files use this to implement the dataset scan feature, which is an important feature of the NcML handler. See docs/ncml-module-deep-dive.md for more info about this. Is there a way I can get equivalent functionality without using the ICU library?

Assumptions and environment:

- Repository: `/Users/jhrg/src/opendap/hyrax/bes`
- Date/time: `2026-05-19 09:59:51 MDT`
- This is an analysis pass, not an implementation pass.
- No configure flags or dependency locations were exercised.
- `retired/` was not searched intentionally.

Findings:

- The ICU dependency is localized to `modules/ncml_module/ScanElement.cc`.
- `ScanElement` uses ICU only for `scan@dateFormatMark`: parse a filename substring with a Java-like date pattern, format it as `yyyy-MM-dd'T'HH:mm:ss'Z'`, and sort scanned datasets by that generated value.
- In-tree NcML examples use only `yyyyDDD_HHmmss`, with literal text before the `#` marker.
- The malformed-pattern regression case uses `yyyxDDD_HHmmss`, so a replacement should reject unsupported or invalid pattern letters instead of silently producing wrong times.
- The repository requires C++11 and probes for C++14, so a small internal parser can use ordinary C++11 facilities but should avoid relying on C++20 date APIs.

Reasoning:

- Full Java `SimpleDateFormat` compatibility is too broad to recreate safely without either ICU or another date library.
- The deployed and tested behavior appears much narrower than full Java compatibility.
- A small purpose-built parser for the supported subset would preserve important `scan@dateFormatMark` behavior while avoiding the ICU build dependency.
- The replacement should be explicit about supported pattern letters and should fail fast for unsupported syntax.

Recommendation:

- Replace ICU with a small NcML-specific date pattern parser only if the project is willing to document a supported subset, not full Java `SimpleDateFormat`.
- Start with support for the existing behavior: `yyyy`, `DDD`, `MM`, `dd`, `HH`, `mm`, `ss`, quoted literals, and unquoted literal separators.
- Convert parsed UTC fields to epoch seconds using a local UTC calendar conversion helper, then format with `gmtime_r`/`gmtime` plus `strftime`.
- Keep `scan@dateFormatMark` parse failures as NcML parse errors.
- Add focused tests for valid `yyyyDDD_HHmmss`, sorted output, malformed/unsupported pattern rejection, and leap-day/day-of-year boundaries.
