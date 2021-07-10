# try to make this snippet as compatible as possible
if [ -z $BASH_VERSION ]; then
	echo ptef/bash-builtins.sh is Bash-only.
	echo Do not source it from non-Bash shells.
	exit 1
fi

# Bash only below

if [[ ${BASH_VERSINFO[0]} < 4 || (${BASH_VERSINFO[0]} == 4 && ${BASH_VERSINFO[1]} < 4) ]]; then
	echo "ptef/bash-builtins.sh supports only Bash 4.4+" >&2
	return 1
fi

PTEF_BASH_LIB="${PTEF_BASH_LIB:-/var/lib/ptef/bash-builtins.so}"
[[ ${PTEF_BASH_LIB::1} != / ]] && PTEF_BASH_LIB="$PWD/$PTEF_BASH_LIB"

enable -f "$PTEF_BASH_LIB" \
	ptef_runner \
	ptef_report \
	ptef_mklog
