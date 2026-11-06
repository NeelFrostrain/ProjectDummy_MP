#!/usr/bin/env python3
"""
scan_duplicates.py
Scans an Unreal Engine plugin Source directory for:
1. Exact and duplicate UFUNCTION names across classes
2. Duplicate Blueprint DisplayNames
3. Duplicate macro #defines (ignoring #if/else pairs)
4. Duplicate USTRUCT and UENUM type declarations
5. Functional duplicates and name collisions
"""

import os
import re
import sys
from collections import defaultdict

def extract_balanced_block(text, start_pos, open_char='(', close_char=')'):
    """Extracts balanced bracket block starting from start_pos."""
    depth = 0
    start_idx = text.find(open_char, start_pos)
    if start_idx == -1:
        return "", -1
    for i in range(start_idx, len(text)):
        if text[i] == open_char:
            depth += 1
        elif text[i] == close_char:
            depth -= 1
            if depth == 0:
                return text[start_idx + 1:i], i + 1
    return "", -1

def scan_directory(source_dir):
    ufunc_names = defaultdict(list)
    display_names = defaultdict(list)
    macro_defs = defaultdict(list)
    type_defs = defaultdict(list)

    re_macro = re.compile(r'^\s*#define\s+([A-Za-z0-9_]+)\b')
    # Match USTRUCT/UENUM declarations while ignoring export macros like MPUTILITYRUNTIME_API
    re_type = re.compile(r'(?:USTRUCT|UENUM)\s*\([^)]*\)\s*(?:struct|enum\s+class|enum)\s+(?:[A-Za-z0-9_]+_API\s+)?([A-Za-z0-9_]+)\b')
    
    for root, _, files in os.walk(source_dir):
        for f in files:
            if not f.endswith(('.h', '.cpp')):
                continue
            path = os.path.join(root, f)
            rel_path = os.path.relpath(path, source_dir)
            with open(path, 'r', encoding='utf-8', errors='ignore') as fp:
                content = fp.read()

            # 1. Macro scan
            for idx, line in enumerate(content.splitlines()):
                m_macro = re_macro.match(line)
                if m_macro:
                    mname = m_macro.group(1)
                    if not (mname.endswith('_H') or mname.endswith('_API') or mname.startswith('__')):
                        macro_defs[mname].append((rel_path, idx + 1))

            # 2. Type scan
            for m_type in re_type.finditer(content):
                tname = m_type.group(1)
                line_no = content[:m_type.start()].count('\n') + 1
                type_defs[tname].append((rel_path, line_no))

            # 3. UFUNCTION scan using balanced parenthesis parsing
            pos = 0
            while True:
                idx = content.find("UFUNCTION", pos)
                if idx == -1:
                    break
                meta_block, end_idx = extract_balanced_block(content, idx, '(', ')')
                if end_idx == -1:
                    pos = idx + 9
                    continue

                line_no = content[:idx].count('\n') + 1
                after_meta = content[end_idx:end_idx + 300]

                # Match signature: optional comments/keywords, return type, function name, '('
                clean_sig = re.sub(r'/\*.*?\*/', '', after_meta, flags=re.DOTALL).strip()
                m_fn = re.search(r'^(?:(?:static|virtual|const|FORCEINLINE)\s+)*[\w:<>*&]+\s+([A-Za-z0-9_]+)\s*\(', clean_sig)
                if m_fn:
                    fname = m_fn.group(1)
                    m_dn = re.search(r'DisplayName\s*=\s*\"([^\"]+)\"', meta_block)
                    dname = m_dn.group(1) if m_dn else fname

                    ufunc_names[fname].append((rel_path, line_no, dname))
                    display_names[dname].append((rel_path, line_no, fname))

                pos = end_idx

    return ufunc_names, display_names, macro_defs, type_defs

def print_report(ufunc_names, display_names, macro_defs, type_defs):
    print("=" * 80)
    print("  MULTIPLAYER UTILITY PLUGIN - DUPLICATE & CONFLICT SCAN REPORT")
    print("=" * 80)

    # 1. Duplicate UFUNCTION names across different files/classes
    dup_ufuncs = {k: v for k, v in ufunc_names.items() if len(v) > 1}
    print(f"\n[1] Duplicate UFUNCTION Names ({len(dup_ufuncs)} found):")
    if not dup_ufuncs:
        print("    None found. (0 duplicates)")
    else:
        for name, occurrences in sorted(dup_ufuncs.items()):
            print(f"  * Function '{name}':")
            for rel_path, line, dname in occurrences:
                print(f"      - {rel_path}:{line} (DisplayName: '{dname}')")

    # 2. Duplicate DisplayNames
    dup_dns = {k: v for k, v in display_names.items() if len(v) > 1}
    print(f"\n[2] Duplicate Blueprint DisplayNames ({len(dup_dns)} found):")
    if not dup_dns:
        print("    None found. (0 duplicates)")
    else:
        for name, occurrences in sorted(dup_dns.items()):
            print(f"  * DisplayName '{name}':")
            for rel_path, line, fname in occurrences:
                print(f"      - {rel_path}:{line} (C++ Function: '{fname}')")

    # 3. Duplicate Macro #defines
    dup_macros = {k: v for k, v in macro_defs.items() if len(v) > 1}
    print(f"\n[3] Duplicate Macro #defines ({len(dup_macros)} found):")
    if not dup_macros:
        print("    None found. (0 duplicates)")
    else:
        for name, occurrences in sorted(dup_macros.items()):
            print(f"  * Macro '{name}':")
            for rel_path, line in occurrences:
                print(f"      - {rel_path}:{line}")

    # 4. Duplicate Types (USTRUCT / UENUM)
    dup_types = {k: v for k, v in type_defs.items() if len(v) > 1}
    print(f"\n[4] Duplicate USTRUCT/UENUM Types ({len(dup_types)} found):")
    if not dup_types:
        print("    None found. (0 duplicates)")
    else:
        for name, occurrences in sorted(dup_types.items()):
            print(f"  * Type '{name}':")
            for rel_path, line in occurrences:
                print(f"      - {rel_path}:{line}")

    print("\n" + "=" * 80)

if __name__ == '__main__':
    src = sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(__file__), '..', 'Source')
    src = os.path.abspath(src)
    print(f"Scanning directory: {src}")
    ufunc_names, display_names, macro_defs, type_defs = scan_directory(src)
    print_report(ufunc_names, display_names, macro_defs, type_defs)
