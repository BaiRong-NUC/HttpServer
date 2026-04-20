# Model serving (FastAPI) and C++ client example

Quick steps:

- Install Python deps:

```
conda activate web
pip install -r app/serving/requirements.txt
```

If the `web` conda environment does not exist, you can install into the project `.venv` instead.

- Start the service:

```
bash app/serving/run_model.sh
```

- Start in background and keep it running after terminal exits:

```
bash app/serving/run_model.sh start
```

- Stop or inspect the background service:

```
bash app/serving/run_model.sh status
bash app/serving/run_model.sh stop
```

You can also run it directly from [app/serving](app/serving):

```
./run_model.sh
```

Interpreter selection logic in `run_model.sh`:

- Prefer the conda environment named `web`
- If `web` is unavailable, fall back to `repo_root/.venv/bin/python`
- If neither is available, fall back to `python3`

- Test with curl:

```
curl -X POST "http://127.0.0.1:8000/predict" -H "Content-Type: application/json" -d '{"features":[0.1,0.2,0.3,0.4]}'
```

- Build C++ example (requires libcurl dev headers):

```
g++ -std=c++17 -o model_client app/serving/model_client.cpp -lcurl
./model_client
```

Notes:

- The service expects the Keras model and `preprocessor.joblib` to be at `app/artifacts/` (this repo already contains them).
- If your feature vector length differs, update the JSON in `model_client.cpp` and the example curl body.
- `./run_model.sh` defaults to foreground development mode with `--reload`; `./run_model.sh start` runs without reload and writes PID/log files under `build/app/serving/`.
- `./run_model.sh status` prints the Python source currently selected by the script.
- For production, a process manager such as systemd or docker is still preferable.
