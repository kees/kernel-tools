# git shell aliases (`~/.bashrc`) (See also "git-scripts")

```
# git shell aliases
alias l='git log --oneline'
alias s='git show'
alias d='git diff'
alias latr='git latr | cat'
alias short='git short'
```


# git command aliases (`~/.gitconfig`)

```
[alias]
	merge-find = "!sh -c 'commit=$0 && branch=${1:-HEAD} && (git rev-list $commit..$branch --ancestry-path | cat -n; git rev-list $commit..$branch --first-parent | cat -n) | sort -k2 -s | uniq -f1 -d | sort -n | tail -1 | cut -f2'"
	merge-show = "!sh -c 'merge=$(git merge-find $0 $1) && [ -n \"$merge\" ] && git show $merge'"
	merge-log-short = "!f() { git log --pretty=oneline \"$1^..$1\"; }; f"
	merge-log = "!f() { git log \"$1^..$1\"; }; f"
	smash = "!f() { sha=\"${1:-$(git rev-parse HEAD)}\" && git commit -a --no-edit --fixup \"$sha\" && EDITOR=true git rebase --autosquash -i \"$sha\"^; }; f"
	contains = "!f() { match=\"${2:-v*}\" && git describe --match \"$match\" --contains \"$1\" | sed 's/[~^].*//'; }; f"
	latest = "!f() { if [ \"$1\" = \"${1%%/*}\" ]; then branch=\"$1/master\"; else branch=\"$1\"; fi && git describe --tags --abbrev=0 \"$branch\"; }; f"
	short = "!f() { for i in \"$@\"; do git log -1 --pretty='%h (\"%s\")' \"$i\"; done; }; f"
	latr = branch --sort=committerdate
	grpe = grep
```

# misc git settings (`~/.gitconfig`)

```
[core]
	abbrev = 12
[url "https://git.kernel.org"]
	insteadOf = git://git.kernel.org
	insteadOf = http://git.kernel.org
[diff]
	renames = true
[rebase]
	autoSquash = true
[sendemail]
	envelopesender = auto
	confirm = auto
[rerere]
	enabled = true
[format]
	pretty = fuller
[b4]
	attestation-trust-model = tofu
	attestation-uid-match = strict
	thanks-commit-url-mask = https://git.kernel.org/kees/c/%.12s
```

# bash prompt

```
# enable programmable completion features (you don't need to enable
# this, if it's already enabled in /etc/bash.bashrc and /etc/profile
# sources /etc/bash.bashrc).
if [ -f /etc/bash_completion ] && ! shopt -oq posix; then
    . /etc/bash_completion
fi

COLOR_RED="\[\e[31;40m\]"
COLOR_GREEN="\[\e[32;40m\]"
COLOR_YELLOW="\[\e[33;40m\]"
COLOR_BLUE="\[\e[34;40m\]"
COLOR_MAGENTA="\[\e[35;40m\]"
COLOR_CYAN="\[\e[36;40m\]"

COLOR_RED_BOLD="\[\e[31;1m\]"
COLOR_GREEN_BOLD="\[\e[32;1m\]"
COLOR_YELLOW_BOLD="\[\e[33;1m\]"
COLOR_BLUE_BOLD="\[\e[34;1m\]"
COLOR_MAGENTA_BOLD="\[\e[35;1m\]"
COLOR_CYAN_BOLD="\[\e[36;1m\]"

COLOR_NONE="\[\e[0m\]"

promptFunc()
{
    PREV_RET_VAL=$?

    PS1=""
    chroot="${debian_chroot:+($debian_chroot)}"

    # Update window title (with external override)
    title="${WINDOW_TITLE:-\u@${chroot}\h: \w}"
    PS1="\[\e]0;${title}\a\]"

    # Current directory
    PS1="${PS1}${COLOR_YELLOW_BOLD}\w${COLOR_NONE}"

    # Current git prompt
    PS1="${PS1}\$(__git_ps1 ' ${COLOR_BLUE_BOLD}(%s)${COLOR_NONE}')"

    if test $PREV_RET_VAL -ne 0
    then
        PREV_RET_VAL_name="${PREV_RET_VAL}"
        if test $PREV_RET_VAL -gt 128
        then
	    # This is a signal death, report it
            PREV_RET_VAL_signal=$(( $PREV_RET_VAL - 128 ))
            PREV_RET_VAL_signal=$(kill -l $PREV_RET_VAL_signal 2>/dev/null)
            if [ -n "$PREV_RET_VAL_signal" ]; then
                PREV_RET_VAL_name="${PREV_RET_VAL}: SIG${PREV_RET_VAL_signal}"
            fi
        fi
        PS1="${PS1} ${COLOR_RED_BOLD}[${PREV_RET_VAL_name}]${COLOR_NONE}"
    fi

    # New line
    PS1="${PS1}\n"

    # Current time
    PS1="${PS1}\$(date +"%H:%M") "

    # User (be loud about being root)
    if test `whoami` != "root"
    then
        PS1="${PS1}${COLOR_CYAN_BOLD}\u${COLOR_NONE}"
    else
        PS1="${PS1}${COLOR_RED_BOLD}\u${COLOR_NONE}"
    fi

    # Host (with chroot override)
    PS1="${PS1}@${chroot}\h"

    # Command prompt end
    PS1="${PS1}${COLOR_GREEN_BOLD}"'\$'"${COLOR_NONE} "
}

PROMPT_COMMAND=promptFunc
```
