# G-Engine — tools/env.sh (sourced by the other scripts, not run directly)
# Puts the tools installed by tools/setup.sh (pip --user) on the PATH.
if command -v python3 >/dev/null 2>&1; then
    GE_USER_BASE="$(python3 -m site --user-base 2>/dev/null || true)"
    if [ -n "$GE_USER_BASE" ] && [ -d "$GE_USER_BASE/bin" ]; then
        PATH="$GE_USER_BASE/bin:$PATH"
    fi
fi
export PATH
