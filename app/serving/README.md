# Model serving (FastAPI) and C++ client example

Quick steps:

- Install Python deps:

```
conda activate web
pip install -r app/serving/requirements.txt
```

If the `web` conda environment does not exist, you can install into the project `.venv` instead.

- Build the project so the serving files and model artifacts are copied into `build/app`:

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
```

- Start the full stack after build:

```
./build/app/loop.sh start
./build/app/loop.sh status
./build/app/loop.sh stop
```

`./build/app/loop.sh start` starts both the C++ HttpServer and the Python model service.

- Start the service:

```
bash build/app/serving/run_model.sh
```

- Start in background and keep it running after terminal exits:

```
bash build/app/serving/run_model.sh start
```

- Stop or inspect the background service:

```
bash build/app/serving/run_model.sh status
bash build/app/serving/run_model.sh stop
```

You can still run it directly from [app/serving](app/serving):

```
./run_model.sh
```

Interpreter selection logic in `run_model.sh`:

- Prefer the conda environment named `web`
- If `web` is unavailable, fall back to `repo_root/.venv/bin/python`
- If neither is available, fall back to `python3`

- Test with curl:

```
curl -X POST "http://127.0.0.1:8000/predict" -H "Content-Type: application/json" -d '{"features":[6,148,72,35,0,33.6,0.627,50]}'
```

Notes:

- The build copies `app/serving` to `build/app/serving` and `app/artifacts` to `build/app/artifacts`.
- The service expects the Keras model and `preprocessor.joblib` under `app/artifacts/` in source mode, or `build/app/artifacts/` in build mode.
- The predict endpoint currently expects 8 input features in the order declared by `manifest.json`.
- `./run_model.sh` defaults to foreground development mode with `--reload`; `./run_model.sh start` runs without reload and writes PID/log files under `build/app/serving/`.
- `./run_model.sh status` prints the Python source currently selected by the script.
- For production, a process manager such as systemd or docker is still preferable.
