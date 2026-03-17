#!/usr/bin/env python3
"""heap_size_report.py

Static-ish heap allocation size report for trx (project + Menu lib).

What it does
- Scans source for explicit heap allocations:
  - new T(...)
  - new T[...]
  - std::make_unique<T>(...)
  - std::make_shared<T>(...)
  - Menu::menuValue<T>, Menu::select<T>, etc. (adds corresponding Shadow<T> types)
- Uses *arm-none-eabi-gdb* against an existing firmware.elf to query sizeof(T)
  from DWARF debug info (no recompilation needed).

What it does NOT do
- It cannot enumerate/size implicit allocations from std::vector/std::string/std::function
  capacity growth, malloc bookkeeping, fragmentation, etc. Those require runtime tracing.

Typical usage
  python3 heap_size_report.py \
    --env genericSTM32F427VGT \
    --out heap_sizes.md

"""

from __future__ import annotations

import argparse
import re
import subprocess
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, Iterable, List, Optional, Sequence, Set, Tuple


@dataclass(frozen=True)
class AllocSite:
    path: str
    line: int
    kind: str
    snippet: str


def project_root() -> Path:
    return Path(__file__).resolve().parent


def default_elf(env: str) -> Path:
    return project_root() / ".pio" / "build" / env / "firmware.elf"


def iter_source_files(roots: Sequence[Path]) -> Iterable[Path]:
    exts = {".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".hxx"}
    for r in roots:
        if not r.exists():
            continue
        for p in r.rglob("*"):
            if p.is_file() and p.suffix in exts:
                yield p


def _line_for_offset(s: str, off: int) -> int:
    # 1-based
    return s.count("\n", 0, off) + 1


def _normalize_type(t: str) -> str:
    t = t.strip()
    # Strip common qualifiers that can appear in captures
    t = re.sub(r"\bconst\b", "", t)
    t = re.sub(r"\bvolatile\b", "", t)
    t = re.sub(r"\bstruct\b", "", t)
    t = re.sub(r"\bclass\b", "", t)
    t = t.replace("&", "").replace("*", "")
    t = re.sub(r"\s+", " ", t).strip()
    return t


def scan_allocations(paths: Sequence[Path]) -> Tuple[Set[str], Dict[str, List[AllocSite]]]:
    """Return (types, sites_by_type)."""

    # NOTE: This is intentionally heuristic, not a full C++ parser.
    rx_make_unique = re.compile(r"std::make_unique\s*<\s*([^>\n]+?)\s*>\s*\(")
    rx_make_shared = re.compile(r"std::make_shared\s*<\s*([^>\n]+?)\s*>\s*\(")

    # new T(...), new T{...}
    rx_new_obj = re.compile(r"(?<![\w:])new\s+([A-Za-z_][\w:<>]*)\s*(?:\(|\{|$)")

    # new T[...]
    rx_new_arr = re.compile(r"(?<![\w:])new\s+([A-Za-z_][\w:<>]*)\s*\[")

    # Menu library template users (to add Shadow<T> types)
    rx_menu_value = re.compile(r"\bMenu::menuValue\s*<\s*([^>\n]+?)\s*>")
    rx_menu_select = re.compile(r"\bMenu::(select|toggle|choose|menuVariant)\s*<\s*([^>\n]+?)\s*>")
    rx_menu_field = re.compile(r"\bMenu::menuField\s*<\s*([^>\n]+?)\s*>")

    # Also match unqualified Menu types (common in files that do `using namespace Menu;`)
    rx_menu_value_uq = re.compile(r"(?<![\w:])menuValue\s*<\s*([^>\n]+?)\s*>")
    rx_menu_select_uq = re.compile(r"(?<![\w:])(select|toggle|choose|menuVariant)\s*<\s*([^>\n]+?)\s*>")
    rx_menu_field_uq = re.compile(r"(?<![\w:])menuField\s*<\s*([^>\n]+?)\s*>")

    types: Set[str] = set()
    sites: Dict[str, List[AllocSite]] = {}

    def add(t: str, site: Optional[AllocSite]) -> None:
        t = _normalize_type(t)
        if not t:
            return

        # Ignore template definitions / placeholders (not real concrete allocations).
        # Example: Menu internal templates in headers use `...<T>` which isn't instantiable.
        if re.search(r"<\s*T\s*(?:[>,])", t):
            return

        types.add(t)
        if site is not None:
            sites.setdefault(t, []).append(site)

    for p in iter_source_files(paths):
        try:
            data = p.read_text(encoding="utf-8", errors="ignore")
        except Exception:
            continue

        def snippet_at(off: int) -> str:
            # A short, single-line snippet for the report
            line_start = data.rfind("\n", 0, off) + 1
            line_end = data.find("\n", off)
            if line_end == -1:
                line_end = len(data)
            return data[line_start:line_end].strip()[:200]

        for rx, kind in (
            (rx_make_unique, "make_unique"),
            (rx_make_shared, "make_shared"),
            (rx_new_obj, "new"),
            (rx_new_arr, "new[]"),
        ):
            for m in rx.finditer(data):
                t = m.group(1)
                site = AllocSite(str(p), _line_for_offset(data, m.start()), kind, snippet_at(m.start()))
                add(t, site)

        # Add Menu shadow types derived from usage
        for m in rx_menu_value.finditer(data):
            t = _normalize_type(m.group(1))
            # skip non-types coming from macros like `typeof(target)`
            if t and "typeof" not in t:
                add(f"Menu::menuValueShadow<{t}>", None)

        for m in rx_menu_select.finditer(data):
            t = _normalize_type(m.group(2))
            if t and "typeof" not in t:
                add(f"Menu::menuVariantShadow<{t}>", None)

        for m in rx_menu_field.finditer(data):
            t = _normalize_type(m.group(1))
            if t and "typeof" not in t:
                add(f"Menu::menuFieldShadow<{t}>", None)

        for m in rx_menu_value_uq.finditer(data):
            t = _normalize_type(m.group(1))
            if t and "typeof" not in t:
                add(f"Menu::menuValueShadow<{t}>", None)

        for m in rx_menu_select_uq.finditer(data):
            t = _normalize_type(m.group(2))
            if t and "typeof" not in t:
                add(f"Menu::menuVariantShadow<{t}>", None)

        for m in rx_menu_field_uq.finditer(data):
            t = _normalize_type(m.group(1))
            if t and "typeof" not in t:
                add(f"Menu::menuFieldShadow<{t}>", None)

    # Always include these Menu types: they are directly heap-allocated by constructors.
    for t in (
        "Menu::promptShadow",
        "Menu::textFieldShadow",
        "Menu::menuNodeShadow",
    ):
        add(t, None)

    return types, sites


def _template_typedef_variants(expr: str) -> List[str]:
    """Generate a few common template-arg spelling variants for gdb parsing.

    In DWARF, fixed-width typedefs may appear as underlying C types.
    """

    variants = [expr]

    repls = {
        "uint8_t": "unsigned char",
        "int8_t": "signed char",
        "uint16_t": "unsigned short",
        "int16_t": "short",
        "uint32_t": "unsigned int",
        "int32_t": "int",
        "uint64_t": "unsigned long long",
        "int64_t": "long long",
    }

    for src, dst in repls.items():
        if src in expr:
            variants.append(expr.replace(src, dst))

    # also try the plain built-in spelling for `unsigned char` if `uint8_t` wasn't present
    return list(dict.fromkeys(variants))


def _candidate_type_exprs(t: str, sites: List[AllocSite]) -> List[str]:
    cands: List[str] = []

    # Known "defaulted template" spellings that gdb typically won't accept as `<>`.
    if t == "io::FileWrapper<>":
        cands.append("io::FileWrapper<16, 128>")

    # Nested impl in fatfs_file
    if t == "Impl":
        cands.append("io::directory_iterator::Impl")

    # MatchedFilter internal array typedefs (these are unsized arrays, so report element size).
    if t in {"samples_t", "taps_t"}:
        site_paths = " ".join([s.path for s in sites])
        if "matched_filter" in site_paths:
            if t == "samples_t":
                cands.append("dsp::matched_filter::MatchedFilter::sample_t")
            else:
                cands.append("dsp::matched_filter::MatchedFilter::tap_t")
            # fallbacks (they are currently the same underlying type)
            cands.append("dsp::matched_filter::MatchedFilter::sample_t")
            cands.append("dsp::matched_filter::MatchedFilter::tap_t")

    # If it's a Menu shadow type unqualified, try qualifying.
    if t in {"promptShadow", "textFieldShadow", "menuNodeShadow", "fieldBaseShadow"}:
        cands.append(f"Menu::{t}")

    # Common qualification guesses for unqualified names.
    if "::" not in t:
        # Prefer based on where it appears.
        site_paths = " ".join([s.path for s in sites])
        if "/lib/Menu/src/" in site_paths:
            cands.append(f"Menu::{t}")
        if "/src/os/" in site_paths:
            cands.append(f"os::{t}")
        if "/src/io/" in site_paths:
            cands.append(f"io::{t}")
        if "/src/dsp/" in site_paths:
            cands.append(f"dsp::{t}")

        for ns in ("dsp", "os", "io", "ui", "Menu", "dsp_ui"):
            cands.append(f"{ns}::{t}")

    cands.append(t)

    # Expand typedef variants for template arguments (e.g. uint8_t -> unsigned char)
    expanded: List[str] = []
    for c in cands:
        expanded.extend(_template_typedef_variants(c))

    # De-dup while preserving order
    return list(dict.fromkeys([c for c in expanded if c]))


def gdb_sizeof(
    elf: Path,
    types: Sequence[str],
    sites_by_type: Dict[str, List[AllocSite]],
    gdb: str,
) -> Tuple[Dict[str, int], Dict[str, str], Dict[str, str]]:
    """Return (sizes, errors, resolved_as).

    sizes[type] = sizeof(type)
    errors[type] = error text (if sizeof failed)
    resolved_as[type] = gdb expression that succeeded (may differ from `type`)

    Implementation note:
    - We run gdb per-type and try multiple candidate spellings.
    """

    if not elf.exists():
        raise FileNotFoundError(f"ELF not found: {elf}")

    sizes: Dict[str, int] = {}
    errors: Dict[str, str] = {}
    resolved_as: Dict[str, str] = {}

    for t in types:
        sites = sites_by_type.get(t, [])
        candidates = _candidate_type_exprs(t, sites)

        last_err = ""
        for expr in candidates:
            safe_t = t.replace("\\", "\\\\").replace('"', r'\\"')
            cmd: List[str] = [
                gdb,
                "-q",
                "-batch",
                "-ex",
                "set pagination off",
                "-ex",
                f"file {elf}",
                "-ex",
                f'printf "{safe_t}\\t%u\\n", (unsigned)sizeof({expr})',
            ]

            p = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)

            if p.returncode == 0:
                out = p.stdout.strip()
                if "\t" in out:
                    _, val = out.split("\t", 1)
                    try:
                        sizes[t] = int(val.strip(), 10)
                        resolved_as[t] = expr
                        break
                    except ValueError:
                        pass

            last_err = (p.stderr.strip() or p.stdout.strip() or "unknown gdb error").strip()

        if t not in sizes:
            last_err = last_err.replace("\n", " | ")
            errors[t] = last_err

    return sizes, errors, resolved_as


def format_report(
    elf: Path,
    sizes: Dict[str, int],
    errors: Dict[str, str],
    resolved_as: Dict[str, str],
    sites: Dict[str, List[AllocSite]],
    max_sites: int,
) -> str:
    all_types = sorted(set(list(sizes.keys()) + list(errors.keys())))

    lines: List[str] = []
    lines.append("# Heap allocation size report (explicit allocations)")
    lines.append("")
    lines.append(f"ELF: `{elf}`")
    lines.append("")
    lines.append(
        "This report covers explicit heap allocation sites found in `/src/**` and `/lib/Menu/src/**`, "
        "and uses `arm-none-eabi-gdb` on the ELF DWARF to query `sizeof(T)` for each discovered type."
    )
    lines.append("")

    ok = len(sizes)
    bad = len(errors)
    lines.append(f"Resolved sizes: {ok}")
    lines.append(f"Unresolved sizes: {bad}")
    lines.append("")

    lines.append("## Sizes")
    lines.append("")
    lines.append("| Type (as found) | sizeof (bytes) | Resolved as (gdb) | Alloc sites (first few) |")
    lines.append("|---|---:|---|---|")

    for t in all_types:
        if t in sizes:
            sz = str(sizes[t])
        else:
            sz = "UNRESOLVED"

        locs = sites.get(t, [])
        if locs:
            shown = locs[:max_sites]
            loc_txt = "; ".join([f"{Path(s.path).as_posix()}:{s.line} ({s.kind})" for s in shown])
            if len(locs) > max_sites:
                loc_txt += f"; +{len(locs) - max_sites} more"
        else:
            loc_txt = "(no direct site recorded; may be internal/template)"

        resolved = resolved_as.get(t, "")
        resolved_txt = f"`{resolved}`" if resolved and resolved != t else ""

        lines.append(f"| `{t}` | {sz} | {resolved_txt} | {loc_txt} |")

    if errors:
        lines.append("")
        lines.append("## Unresolved")
        lines.append("")
        for t in sorted(errors.keys()):
            lines.append(f"- `{t}`: {errors[t]}")

    lines.append("")
    lines.append("## Notes")
    lines.append("")
    lines.append("- `std::vector`, `std::string`, and `std::function` internal allocations are not enumerated here; they depend on runtime sizes/capacities.")
    lines.append("- `new T[n]` sites are reported as `sizeof(T)` only; runtime `n` is not known statically.")

    return "\n".join(lines) + "\n"


def main(argv: Optional[Sequence[str]] = None) -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--env", default="genericSTM32F427VGT", help="PlatformIO env name (default: genericSTM32F427VGT)")
    ap.add_argument("--elf", default=None, help="Path to firmware.elf (default: .pio/build/<env>/firmware.elf)")
    ap.add_argument("--out", default="heap_sizes.md", help="Output markdown path")
    ap.add_argument("--gdb", default="arm-none-eabi-gdb", help="GDB executable (default: arm-none-eabi-gdb)")
    ap.add_argument("--max-sites", type=int, default=3, help="Max alloc sites listed per type")

    args = ap.parse_args(argv)

    elf = Path(args.elf) if args.elf else default_elf(args.env)

    scan_roots = [
        project_root() / "src",
        project_root() / "lib" / "Menu" / "src",
    ]

    types, sites = scan_allocations(scan_roots)
    sorted_types = sorted(types)

    sizes, errors, resolved_as = gdb_sizeof(elf, sorted_types, sites, args.gdb)

    report = format_report(elf, sizes, errors, resolved_as, sites, args.max_sites)

    out_path = Path(args.out)
    if not out_path.is_absolute():
        out_path = project_root() / out_path

    out_path.write_text(report, encoding="utf-8")
    print(f"wrote: {out_path}")
    print(f"types: {len(sorted_types)}, resolved: {len(sizes)}, unresolved: {len(errors)}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
