#!/usr/bin/env python3
"""
bmsignore.py - simple utility to list files excluded by .bmsignore patterns

Usage:
  python tools/bmsignore.py [path]

Reads .bmsignore from workspace root (or provided path) and prints matching files.
"""
import sys
import os
import fnmatch


def load_patterns(path):
    patterns = []
    try:
        with open(path, 'r', encoding='utf-8') as f:
            for line in f:
                s = line.strip()
                if not s or s.startswith('#'):
                    continue
                patterns.append(s)
    except FileNotFoundError:
        return []
    return patterns


def matches_any(path, patterns):
    for p in patterns:
        # directory pattern
        if p.endswith('/'):
            if path.startswith(p[:-1]):
                return True
        if fnmatch.fnmatch(path, p) or fnmatch.fnmatch(os.path.basename(path), p):
            return True
    return False


def find_ignored(root, patterns):
    ignored = []
    for base, dirs, files in os.walk(root):
        rel = os.path.relpath(base, root)
        if rel == '.':
            rel = ''
        if matches_any(rel + '/', patterns):
            ignored.append(rel + '/')
            # skip descend into ignored dir
            dirs[:] = []
            continue
        for f in files:
            relf = os.path.join(rel, f) if rel else f
            if matches_any(relf, patterns):
                ignored.append(relf)
    return ignored


def main():
    root = os.getcwd()
    if len(sys.argv) > 1:
        root = sys.argv[1]
    pattern_path = os.path.join(root, '.bmsignore')
    patterns = load_patterns(pattern_path)
    ignored = find_ignored(root, patterns)
    for i in ignored:
        print(i)


if __name__ == '__main__':
    main()
