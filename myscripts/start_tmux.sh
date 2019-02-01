#!/bin/sh
BAR=/home/kippesp/projects/ops-class-os161/setenv.sh
tmux new-session -s os161 \; \
  setenv TMUX_SESSION_HOOK $BAR \; \
  send-keys -t 0 "export TMUX_SESSION_HOOK="$BAR C-m \; \
  send-keys -t 0 '. $TMUX_SESSION_HOOK' C-m \; \
  send-keys -t 0 'clear' C-m \; \
  send-keys -t 0 'h' C-m \;
