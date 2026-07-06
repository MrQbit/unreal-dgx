#!/bin/bash
# Is the GameForge editor service up and answering Remote Control?
code=$(curl -s -o /dev/null -w "%{http_code}" http://127.0.0.1:30010/remote/info 2>/dev/null)
if [ "$code" = "200" ]; then
  echo "GameForge service: UP (RC :30010 answering) — ready to build from chat."
else
  pgrep -f "UnrealEditor.*-RCWebControlEnable" >/dev/null \
    && echo "GameForge service: STARTING (editor up, RC not ready yet — shaders compiling?). http=$code" \
    || echo "GameForge service: DOWN. Start it: scripts/start_forge_service.sh (or systemctl start forge-editor)"
fi
