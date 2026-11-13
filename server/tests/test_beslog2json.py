#!/usr/bin/env python3
# -*- mode: python; c-basic-offset:4 -*-
#
# Unit tests for server/beslog2json.py
#
# Run with:  python3 -m unittest discover -s tests -v   (from the server directory)
#
import contextlib
import importlib.util
import io
import json
import os
import sys
import tempfile
import unittest

SCRIPT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "beslog2json.py")

D = "|&|"


def load_module():
    """Load a fresh copy of beslog2json.py so that module globals (flags, prefixed keys) never leak between tests."""
    spec = importlib.util.spec_from_file_location("beslog2json_under_test", SCRIPT)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


def request_line(time="1601646465", pid="2122", with_ce=True, olfs_time="1601646465304"):
    fields = [time, "inst", pid, "rid-outer", "request", "OLFS", "127.0.0.1", "agent", "sess", "user",
              olfs_time, "REQ-1", "HTTP-GET", "/opendap/data/nc/fnoc1.nc.dds", "-", "BES",
              "get.dds", "dap2", "/data/nc/fnoc1.nc"]
    if with_ce:
        fields.append("u[0:1]")
    return D.join(fields)


def simple_line(rtype, msg, time="1601642679", pid="2122"):
    return D.join([time, "inst", pid, "rid", rtype, msg])


def timing_line(name="Command timing: foo"):
    return D.join(["1601642669", "inst", "2122", "rid", "timing", "elapsed-us", "4", "start-us", "100",
                   "stop-us", "104", "REQ-1", name])


def square_timing_line(timer="Command timing: BESXMLInterface::transmit_data() - "):
    return ("[UTC Wed Nov 27 18:48:20 2024][pid:117][thread:139661163814208][timing][ELAPSED][4 us]"
            "[STARTED][1732733300645844 us][STOPPED][1732733300645848 us][-][" + timer + "]")


class Base(unittest.TestCase):
    def setUp(self):
        self.m = load_module()

    def run_stdin(self, *lines):
        """Feed lines through read_from_stdin(); return (list of parsed json records, raw stdout)."""
        stdin = io.StringIO("".join(l + "\n" for l in lines))
        out = io.StringIO()
        with contextlib.redirect_stdout(out):
            old = sys.stdin
            sys.stdin = stdin
            try:
                self.m.read_from_stdin()
            finally:
                sys.stdin = old
        raw = out.getvalue()
        return [json.loads(l) for l in raw.splitlines() if l], raw

    def run_main(self, argv, stdin_text=""):
        out, err = io.StringIO(), io.StringIO()
        old = sys.stdin
        sys.stdin = io.StringIO(stdin_text)
        try:
            with contextlib.redirect_stdout(out), contextlib.redirect_stderr(err):
                self.m.main(argv)
        finally:
            sys.stdin = old
        return [json.loads(l) for l in out.getvalue().splitlines() if l], err.getvalue()


class TestRequestRecords(Base):
    def test_all_fields(self):
        (rec,), _ = self.run_stdin(request_line())
        self.assertEqual(rec, {
            "time": 1601646465, "instance-id": "inst", "pid": 2122, "type": "request",
            "client-ip": "127.0.0.1", "user-agent": "agent", "session-id": "sess", "user-id": "user",
            "olfs-start-time": 1601646465304, "request-id": "REQ-1", "http-verb": "HTTP-GET",
            "url-path": "/opendap/data/nc/fnoc1.nc.dds", "query-string": "-", "bes-action": "get.dds",
            "return-as": "dap2", "local-path": "/data/nc/fnoc1.nc", "ce": "u[0:1]",
        })

    def test_missing_ce_defaults_to_dash(self):
        (rec,), _ = self.run_stdin(request_line(with_ce=False))
        self.assertEqual(rec["ce"], "-")

    def test_non_numeric_times_stay_strings(self):
        (rec,), _ = self.run_stdin(request_line(time="2024-11-27T18:48:20", olfs_time="not-a-number"))
        self.assertEqual(rec["time"], "2024-11-27T18:48:20")
        self.assertEqual(rec["olfs-start-time"], "not-a-number")

    def test_disabled(self):
        self.m.TRANSMIT_REQUEST_LOG = False
        recs, raw = self.run_stdin(request_line())
        self.assertEqual(recs, [])
        self.assertEqual(raw, "")


class TestMessageRecords(Base):
    def test_info_error_verbose(self):
        recs, _ = self.run_stdin(simple_line("info", "hello"), simple_line("error", "boom"),
                                 simple_line("verbose", "chatty"))
        self.assertEqual([(r["type"], r["message"]) for r in recs],
                         [("info", "hello"), ("error", "boom"), ("verbose", "chatty")])
        self.assertEqual(recs[0]["time"], 1601642679)
        self.assertEqual(recs[0]["pid"], 2122)
        self.assertEqual(recs[0]["request-id"], "rid")
        self.assertEqual(recs[0]["instance-id"], "inst")

    def test_each_type_can_be_disabled(self):
        for rtype, flag in (("info", "TRANSMIT_INFO_LOG"), ("error", "TRANSMIT_ERROR_LOG"),
                            ("verbose", "TRANSMIT_VERBOSE_LOG")):
            with self.subTest(rtype=rtype):
                m = load_module()
                setattr(m, flag, False)
                self.m = m
                recs, _ = self.run_stdin(simple_line(rtype, "x"))
                self.assertEqual(recs, [])


class TestTimingRecords(Base):
    def test_suppressed_by_default(self):
        recs, _ = self.run_stdin(timing_line())
        self.assertEqual(recs, [])

    def test_enabled(self):
        self.m.TRANSMIT_TIMING_LOG = True
        (rec,), _ = self.run_stdin(timing_line())
        self.assertEqual(rec["elapsed-us"], 4)
        self.assertEqual(rec["start-us"], 100)
        self.assertEqual(rec["stop-us"], 104)
        self.assertEqual(rec["request-id"], "REQ-1")
        self.assertEqual(rec["timer-name"], "Command timing: foo")
        self.assertEqual(rec["type"], "timing")

    def test_profiling_only(self):
        # TRANSMIT_PROFILING_LOG defaults to True, TRANSMIT_TIMING_LOG to False.
        recs, _ = self.run_stdin(timing_line("Command timing: foo"), timing_line("Profile timing: bar"))
        self.assertEqual([r["timer-name"] for r in recs], ["Profile timing: bar"])

    def test_profiling_disabled(self):
        self.m.TRANSMIT_PROFILING_LOG = False
        recs, _ = self.run_stdin(timing_line("Profile timing: bar"))
        self.assertEqual(recs, [])


class TestSquareBracketRecords(Base):
    def test_timing_suppressed_by_default(self):
        recs, _ = self.run_stdin(square_timing_line())
        self.assertEqual(recs, [])

    def test_timing_enabled(self):
        self.m.TRANSMIT_TIMING_LOG = True
        (rec,), _ = self.run_stdin(square_timing_line())
        self.assertEqual(rec["time"], "UTC Wed Nov 27 18:48:20 2024")
        self.assertEqual(rec["pid"], 117)
        self.assertEqual(rec["type"], "timing")
        self.assertEqual(rec["elapsed-us"], 4)
        self.assertEqual(rec["start-us"], 1732733300645844)
        self.assertEqual(rec["stop-us"], 1732733300645848)
        self.assertEqual(rec["request-id"], "-")
        self.assertEqual(rec["timer-name"], "Command timing: BESXMLInterface::transmit_data() - ")

    def test_profiling_only(self):
        recs, _ = self.run_stdin(square_timing_line("Command timing: x"),
                                 square_timing_line("Profile timing: y"))
        self.assertEqual([r["timer-name"] for r in recs], ["Profile timing: y"])

    def test_missing_elapsed_is_error_record(self):
        self.m.TRANSMIT_TIMING_LOG = True
        line = square_timing_line().replace("[ELAPSED]", "[BOGUS]")
        (rec,), _ = self.run_stdin(line)
        self.assertEqual(rec["type"], "error")
        self.assertIn("Failed to identify timing data", rec["message"])

    def test_non_timing_type_is_error_record(self):
        line = "[UTC Wed Nov 27 18:48:20 2024][pid:117][thread:1][info][some message]"
        (rec,), _ = self.run_stdin(line)
        self.assertEqual(rec["type"], "error")
        self.assertIn("Only timing", rec["message"])


class TestErrorHandling(Base):
    def assert_error(self, rec, text):
        self.assertEqual(rec["type"], "error")
        self.assertIn(text, rec["message"])
        self.assertIsInstance(rec["time"], int)
        self.assertEqual(rec["pid"], os.getpid())

    def test_unknown_type(self):
        (rec,), _ = self.run_stdin(simple_line("mystery", "x"))
        self.assert_error(rec, "UNKNOWN LOG RECORD TYPE mystery")

    def test_too_few_fields(self):
        (rec,), _ = self.run_stdin("garbage|&|only")
        self.assert_error(rec, "Incompatible input log line 1")

    def test_no_delimiter(self):
        (rec,), _ = self.run_stdin("just some text")
        self.assert_error(rec, "Incompatible input log line 1")

    def test_exception_becomes_error_record(self):
        (rec,), _ = self.run_stdin(simple_line("info", "x", pid="not-an-int"))
        self.assert_error(rec, "failed with the message")

    def test_truncated_request_becomes_error_record(self):
        (rec,), _ = self.run_stdin(D.join(["1", "inst", "2", "rid", "request", "OLFS"]))
        self.assert_error(rec, "failed with the message")

    def test_blank_lines_skipped_and_processing_continues(self):
        # Regression for HYRAX-2326: a blank line must not terminate processing.
        recs, _ = self.run_stdin(simple_line("info", "a"), "", simple_line("info", "b"))
        self.assertEqual([r["message"] for r in recs], ["a", "b"])

    def test_empty_input(self):
        recs, raw = self.run_stdin()
        self.assertEqual((recs, raw), ([], ""))

    def test_unicode_decode_error_is_reported(self):
        class BadStdin(io.StringIO):
            calls = 0

            def readline(self, *a):
                BadStdin.calls += 1
                if BadStdin.calls == 1:
                    raise UnicodeDecodeError("utf-8", b"\xff", 0, 1, "invalid start byte")
                return ""

        out = io.StringIO()
        old = sys.stdin
        sys.stdin = BadStdin()
        try:
            with contextlib.redirect_stdout(out):
                self.m.read_from_stdin()
        finally:
            sys.stdin = old
        (rec,) = [json.loads(l) for l in out.getvalue().splitlines()]
        self.assert_error(rec, "invalid start byte")


class TestPrefixAndSorting(Base):
    def test_prefix_applied_to_all_keys(self):
        self.m.TRANSMIT_TIMING_LOG = True
        recs, _ = self.run_main_with_prefix("besd", request_line(), simple_line("info", "m"), timing_line())
        req, info, timing = recs
        self.assertTrue(all(k.startswith("besd-") for k in req), req.keys())
        self.assertEqual(req["besd-client-ip"], "127.0.0.1")
        self.assertEqual(req["besd-ce"], "u[0:1]")
        self.assertEqual(info["besd-message"], "m")
        self.assertEqual(info["besd-pid"], 2122)
        self.assertTrue(all(k.startswith("besd-") for k in timing), timing.keys())
        self.assertEqual(timing["besd-elapsed-us"], 4)
        self.assertEqual(timing["besd-timer-name"], "Command timing: foo")

    def run_main_with_prefix(self, prefix, *lines):
        recs, _ = self.run_main(["-p", prefix, "-t", "true"], "".join(l + "\n" for l in lines))
        return recs, None

    def test_prefix_trailing_dash_not_doubled(self):
        recs, _ = self.run_main(["-p", "besd-"], simple_line("info", "m") + "\n")
        self.assertIn("besd-message", recs[0])
        self.assertNotIn("besd--message", recs[0])

    def test_no_prefix(self):
        self.assertEqual(self.m.the_prefix, "")
        self.assertEqual(self.m.TIME_KEY, "time")

    def test_sorted_keys(self):
        _, raw = self.run_stdin(simple_line("info", "m"))
        self.assertEqual(list(json.loads(raw).keys()),
                         ["instance-id", "message", "pid", "request-id", "time", "type"])

    def test_unsorted_keys(self):
        self.m.SORT_KEYS = False
        _, raw = self.run_stdin(simple_line("info", "m"))
        self.assertEqual(list(json.loads(raw).keys()),
                         ["time", "instance-id", "pid", "request-id", "type", "message"])


class TestMain(Base):
    def test_defaults_read_stdin(self):
        recs, _ = self.run_main([], simple_line("info", "hi") + "\n")
        self.assertEqual(recs[0]["message"], "hi")
        self.assertFalse(self.m.TRANSMIT_TIMING_LOG)

    def test_all_flag(self):
        self.run_main(["-a"], "")
        for flag in ("REQUEST", "INFO", "ERROR", "VERBOSE", "TIMING", "PROFILING"):
            self.assertTrue(getattr(self.m, f"TRANSMIT_{flag}_LOG"), flag)

    def test_timing_flag_values(self):
        self.run_main(["-t", "true"], "")
        self.assertTrue(self.m.TRANSMIT_TIMING_LOG)
        m = self.m = load_module()
        m.TRANSMIT_TIMING_LOG = True
        self.run_main(["-t", "False"], "")
        self.assertFalse(self.m.TRANSMIT_TIMING_LOG)

    def test_disable_info_and_request_via_options(self):
        recs, _ = self.run_main(["-i", "false", "-r", "false"],
                                request_line() + "\n" + simple_line("info", "x") + "\n" + simple_line("error", "e") + "\n")
        self.assertEqual([r["type"] for r in recs], ["error"])

    def test_profiling_option(self):
        self.run_main(["-P", "false"], "")
        self.assertFalse(self.m.TRANSMIT_PROFILING_LOG)

    def test_sort_long_option(self):
        self.run_main(["--sort", "false"], "")
        self.assertFalse(self.m.SORT_KEYS)

    def test_read_from_file(self):
        with tempfile.NamedTemporaryFile("w", suffix=".log", delete=False) as f:
            f.write(simple_line("info", "from file") + "\n")
            f.write(simple_line("error", "bad") + "\n")
            path = f.name
        try:
            recs, _ = self.run_main(["-f", path])
        finally:
            os.unlink(path)
        self.assertEqual([(r["type"], r["message"]) for r in recs],
                         [("info", "from file"), ("error", "bad")])

    def test_read_from_file_skips_blank_lines_and_strips_crlf(self):
        with tempfile.NamedTemporaryFile("wb", suffix=".log", delete=False) as f:
            f.write(simple_line("info", "a").encode() + b"\r\n\n" + simple_line("info", "b").encode() + b"\n")
            path = f.name
        try:
            recs, _ = self.run_main(["-f", path])
        finally:
            os.unlink(path)
        self.assertEqual([r["message"] for r in recs], ["a", "b"])

    def test_sort_short_option_takes_value(self):
        recs, _ = self.run_main(["-s", "false"], simple_line("info", "x") + "\n")
        self.assertFalse(self.m.SORT_KEYS)
        self.assertEqual(len(recs), 1)

    def test_verbose_option_takes_value(self):
        recs, _ = self.run_main(["-v", "false"], simple_line("verbose", "x") + "\n")
        self.assertFalse(self.m.TRANSMIT_VERBOSE_LOG)
        self.assertEqual(recs, [])
        self.m = load_module()
        recs, _ = self.run_main(["-v", "true"], simple_line("verbose", "x") + "\n")
        self.assertEqual(len(recs), 1)

    def test_debug_writes_to_stderr_only(self):
        recs, err = self.run_main(["-d"], simple_line("info", "x") + "\n")
        self.assertEqual(len(recs), 1)  # stdout still pure JSON
        self.assertIn("# ", err)
        self.assertIn("beslog2json.py configuration", err)

    def test_help_exits_zero(self):
        with self.assertRaises(SystemExit) as cm:
            self.run_main(["-h"])
        self.assertIn(cm.exception.code, (None, 0))

    def test_bad_option_exits_2(self):
        with self.assertRaises(SystemExit) as cm:
            self.run_main(["--no-such-option"])
        self.assertEqual(cm.exception.code, 2)

    def test_unexpected_argument_exits_2(self):
        with self.assertRaises(SystemExit) as cm:
            self.run_main(["stray"])
        self.assertEqual(cm.exception.code, 2)


class TestHelpers(Base):
    def test_eord(self):
        self.assertEqual(self.m.eord(True), "enabled")
        self.assertEqual(self.m.eord(False), "disabled")

    def test_debug_off_is_silent(self):
        err = io.StringIO()
        with contextlib.redirect_stderr(err):
            self.m.debug("hello")
        self.assertEqual(err.getvalue(), "")

    def test_debug_on_escapes_newlines(self):
        self.m.DEBUG_FLAG = True
        err = io.StringIO()
        with contextlib.redirect_stderr(err):
            self.m.debug("a\nb")
        self.assertEqual(err.getvalue(), "# a\\nb\n")

    def test_processing_error_record(self):
        rec = {}
        self.assertTrue(self.m.processing_error("oops", rec))
        self.assertEqual(rec["type"], "error")
        self.assertEqual(rec["message"], "oops")
        self.assertEqual(rec["pid"], os.getpid())


if __name__ == "__main__":
    unittest.main()
