#!/usr/bin/env bash
set -euo pipefail
# Run from repository root. Starts uvicorn serving the model.
python3 -m uvicorn app.serving.model_api:app --host 127.0.0.1 --port 8000 --reload
