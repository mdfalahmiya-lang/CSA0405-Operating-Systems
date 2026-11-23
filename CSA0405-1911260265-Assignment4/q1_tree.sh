#!/bin/bash
# Task 1 evidence: the proposed flat directory is replaced by a per-student
# tree with explicit permission bits. This script builds a small demonstration
# tree under /tmp so the structure can be inspected without touching real data.
set -e

ROOT=/tmp/q1_demo
BASE="$ROOT/students/2026/CSE/1911260265"

rm -rf "$ROOT"
mkdir -p "$BASE"/documents "$BASE"/projects/shared "$BASE"/backup
printf 'Lab report\n' > "$BASE/documents/report.docx"
printf 'Revision notes\n' > "$BASE/documents/notes.txt"
printf 'Shared design\n' > "$BASE/projects/shared/design.md"
printf 'Snapshot\n' > "$BASE/backup/2026-11-22.tar"

# Home and private directories: owner only.
chmod 700 "$ROOT/students" "$ROOT/students/2026" "$ROOT/students/2026/CSE" \
          "$BASE" "$BASE/documents" "$BASE/backup"
chmod 600 "$BASE/documents/report.docx" "$BASE/documents/notes.txt" \
          "$BASE/backup/2026-11-22.tar"

# Shared project directory: group write, setgid so new files inherit the group.
chmod 2770 "$BASE/projects" "$BASE/projects/shared"
chmod 660 "$BASE/projects/shared/design.md"

echo "Proposed design: one flat directory, 12,000 students, student-chosen names"
echo "  entries at the root        : 12,000 students x ~4 files = 48,000"
echo "  name clashes               : birthday problem over common names"
echo "  protection                 : none (any student can read or delete any file)"
echo "  growth                     : contiguous, so a file cannot extend past a neighbour"
echo
echo "Improved design: tree rooted at /students/<year>/<department>/<register>/"
find "$ROOT" -mindepth 1 -printf '%M %u:%g %6s %P\n' | sort
echo
echo "Path walk for the shared project file (permission of every component):"
namei -l "$BASE/projects/shared/design.md"
echo
echo "Entries per directory: students=1, 2026=1, CSE=1, student=3, projects=1, shared=1"
echo "The root holds one entry per year, not 12,000 student directories."
