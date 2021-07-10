# try to make this snippet as compatible as possible
if [ -z $BASH_VERSION ]; then
	echo ptef/bash-builtins.sh is Bash-only.
	echo Do not source it from non-Bash shells.
	exit 1
fi

# Bash only below

[ -z "$PTEF_BASH_LIB_SOURCED" ] || return 0

if [ "${BASH_VERSINFO[0]}" -lt 4 -o "${BASH_VERSINFO[0]}" -eq 4 -a "${BASH_VERSINFO[1]}" -lt 4 ]; then
	echo "ptef/bash-builtins.sh supports only Bash 4.4+" >&2
	return 1
fi

PTEF_BASH_LIB="${PTEF_BASH_LIB:-/var/lib/ptef/bash-builtins.so}"
[ "${PTEF_BASH_LIB::1}" != "/" ] && PTEF_BASH_LIB="$PWD/$PTEF_BASH_LIB"

enable -f "$PTEF_BASH_LIB" \
	ptef_report \
	ptef_mklog

# alias runner for compatibility
# see explanation for why this isn't a true builtin in bash-builtins.c
alias ptef_runner='ptef-runner'

PTEF_BASH_LIB_SOURCED=1
