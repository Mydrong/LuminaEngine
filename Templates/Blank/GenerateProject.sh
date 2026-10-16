#!/usr/bin/env bash
# Any engine on this machine starts this, and the build tool hands it to the engine the .lproject names.

set -uo pipefail

ProjectDir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
Store="${XDG_DATA_HOME:-$HOME/.local/share}/Lumina/Engines.json"

EngineDir=""
if [ -f "$Store" ]; then
    while IFS= read -r Candidate; do
        if [ -x "$Candidate/LuminaBuild.sh" ]; then
            EngineDir="$Candidate"
            break
        fi
    done < <(sed -n 's/^ *"[^"]*": *"\(.*\)",\{0,1\} *$/\1/p' "$Store")
fi

if [ -z "$EngineDir" ]; then
    echo "No Lumina engine is known on this machine. Run Setup.sh in an engine checkout once, then try again." >&2
    exit 1
fi

# Extra arguments pass straight through.
if ! "$EngineDir/LuminaBuild.sh" GenerateProjectFiles -Project="$ProjectDir" "$@"; then
    echo
    echo "Project generation failed." >&2
    exit 1
fi
