#!/bin/sh
# Start the Crow backend in the background
Clef "$@" &

# Start nginx in the foreground (keeps container alive)
nginx -g "daemon off;"