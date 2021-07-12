

export PATH="$PWD/bin:$PATH"
export LD_LIBRARY_PATH="$PWD/bin"
export TEST_BINDIR="$PWD/bin"

#function err { echo >&2; echo "ERROR: $@" >&2; echo >&2; _failed=1; }

#function hide {
#	{ set +x; } 2>/dev/null
#	"$@"
#	{ set -x; } 2>/dev/null
#}

function assert_grep {
	grep "$@" >&2 || {
		{
			echo "assert_grep failed, ${@: -1} contains:"
			cat "${@: -1}"
			return 1
		} 1>&2 2>/dev/null
	}
}
function assert_nogrep {
	! grep "$@" >&2 || {
		{
			echo "assert_nogrep failed, ${@: -1} contains:"
			cat "${@: -1}"
			return 1
		} 1>&2 2>/dev/null
	}
}

function assert_ptef_runner {
	ptef-runner "$@" 2>runner_err
	! grep '' runner_err >&2
}

_added_tests=()
function + {
	_added_tests+=("$*")
}
function _run_test {
	rm -rf testdir && mkdir testdir && \
	rm -rf tmpdir && mkdir tmpdir || return 1
	(
		set -e
		unset PTEF_PREFIX PTEF_LOGS PTEF_RESULTS_FD PTEF_NOLOGS \
			PTEF_COLOR PTEF_IGNORE_FILES
		TEST_TMPDIR="$PWD/tmpdir"
		cd testdir
		set -x
		"$1"
	)
}

function run_tests {
	local _tst= _tests=
	[ "$#" -gt 0 ] && _tests=("$@") || _tests=("${_added_tests[@]}")
	for _tst in "${_tests[@]}"; do
		ptef-report RUN "$_tst"
		# this must not be inside 'if', otherwise set -e will not
		# work in the subshell
		_run_test "$_tst"
		if [ $? -eq 0 ]; then
			ptef-report PASS "$_tst"
		else
			ptef-report FAIL "$_tst"
		fi
	done
}
