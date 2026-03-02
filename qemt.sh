#!/bin/sh
# qemt - Q Emergency Tools
# Shell function to toggle the emergency toolkit on/off
#
# Copyright (c) 2026 Quaylyn Rimer. All rights reserved.
# Licensed under the MIT License. See LICENSE for details.
#
# Usage:
#   source /usr/local/share/qemt/qemt.sh   (or add to .bashrc/.profile)
#   qemt on       - Activate emergency tools (prepend to PATH)
#   qemt off      - Deactivate emergency tools (restore PATH)
#   qemt status   - Show if qemt is active
#   qemt list     - List available emergency tools
#   qemt help     - Show usage

QEMT_DIR="/usr/local/qemt"

# Count tools without relying on external commands (which may be shadowed)
_qemt_count_tools() {
    _cnt=0
    for _f in "$QEMT_DIR"/*; do
        [ -f "$_f" ] && _cnt=$((_cnt + 1))
    done
    echo "$_cnt"
}

qemt() {
    case "$1" in
        on)
            if [ ! -d "$QEMT_DIR" ]; then
                echo "qemt: tools not installed at $QEMT_DIR"
                echo "  Run: sudo make install-rescue"
                return 1
            fi

            # Check if already active
            case ":$PATH:" in
                *":$QEMT_DIR:"*)
                    echo "qemt: already active"
                    return 0
                    ;;
            esac

            # Save original PATH before modification
            if [ -z "$QEMT_ORIG_PATH" ]; then
                export QEMT_ORIG_PATH="$PATH"
            fi

            # Prepend qemt directory so our tools shadow the originals
            export PATH="$QEMT_DIR:$PATH"
            export QEMT_ACTIVE=1

            # Count available tools
            _qemt_count=$(_qemt_count_tools)
            echo "qemt: activated - $_qemt_count emergency tools now in PATH"
            echo "  Tools directory: $QEMT_DIR"
            echo "  Run 'qemt off' to deactivate"
            echo "  Run 'qemt list' to see available tools"
            unset _qemt_count
            ;;

        off)
            if [ -z "$QEMT_ACTIVE" ]; then
                echo "qemt: not active"
                return 0
            fi

            # Restore original PATH
            if [ -n "$QEMT_ORIG_PATH" ]; then
                export PATH="$QEMT_ORIG_PATH"
                unset QEMT_ORIG_PATH
            else
                # Fallback: strip QEMT_DIR from PATH manually
                export PATH=$(echo "$PATH" | sed "s|$QEMT_DIR:||g; s|:$QEMT_DIR||g; s|$QEMT_DIR||g")
            fi

            unset QEMT_ACTIVE
            echo "qemt: deactivated - system tools restored"
            ;;

        status)
            if [ -n "$QEMT_ACTIVE" ]; then
                echo "qemt: ACTIVE"
                echo "  Tools directory: $QEMT_DIR"
                _qemt_count=$(_qemt_count_tools)
                echo "  Tools available: $_qemt_count"
                unset _qemt_count
                echo "  Original PATH saved: yes"
                echo ""
                echo "  Run 'qemt off' to deactivate"
            else
                echo "qemt: inactive"
                if [ -d "$QEMT_DIR" ]; then
                    _qemt_count=$(_qemt_count_tools)
                    echo "  Tools installed: $_qemt_count (at $QEMT_DIR)"
                    unset _qemt_count
                    echo "  Run 'qemt on' to activate"
                else
                    echo "  Tools not installed at $QEMT_DIR"
                    echo "  Run: sudo make install-rescue"
                fi
            fi
            ;;

        list)
            if [ ! -d "$QEMT_DIR" ]; then
                echo "qemt: tools not installed at $QEMT_DIR"
                return 1
            fi

            echo "qemt - Q Emergency Tools"
            echo "========================"
            echo ""

            if [ -n "$QEMT_ACTIVE" ]; then
                echo "Status: ACTIVE (these tools are shadowing system tools)"
            else
                echo "Status: inactive (run 'qemt on' to activate)"
            fi
            echo ""
            echo "Available tools:"
            echo ""

            # List tools in columns (using shell glob to avoid depending on external ls)
            _qemt_count=0
            _qemt_line=""
            for _f in "$QEMT_DIR"/*; do
                [ -f "$_f" ] || continue
                _name="${_f##*/}"
                _qemt_count=$((_qemt_count + 1))
                _padded="$(printf '%-18s' "$_name")"
                _qemt_line="${_qemt_line}${_padded}"
                if [ $(( _qemt_count % 4 )) -eq 0 ]; then
                    echo "  $_qemt_line"
                    _qemt_line=""
                fi
            done
            [ -n "$_qemt_line" ] && echo "  $_qemt_line"
            echo ""
            echo "Total: $_qemt_count tools"
            unset _qemt_count _qemt_line _padded _name
            ;;

        help|--help|-h|"")
            echo "qemt - Q Emergency Tools"
            echo ""
            echo "A complete set of Unix tools built without glibc."
            echo "When activated, qemt tools shadow the system tools in PATH."
            echo ""
            echo "Usage:"
            echo "  qemt on       Activate emergency tools"
            echo "  qemt off      Deactivate (restore system tools)"
            echo "  qemt status   Show current state"
            echo "  qemt list     List available tools"
            echo "  qemt help     Show this help"
            echo ""
            echo "How it works:"
            echo "  'qemt on' prepends $QEMT_DIR to your PATH."
            echo "  All commands (ls, cat, sudo, etc.) then resolve to"
            echo "  the glibc-free qemt versions first."
            echo "  'qemt off' restores your original PATH."
            echo ""
            echo "Setup:"
            echo "  Add to ~/.bashrc or ~/.profile:"
            echo "    source /usr/local/share/qemt/qemt.sh"
            ;;

        *)
            echo "qemt: unknown command '$1'"
            echo "  Run 'qemt help' for usage"
            return 1
            ;;
    esac
}
