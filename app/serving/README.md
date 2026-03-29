# Model serving (FastAPI) and C++ client example

Quick steps:

- Install Python deps:

```
pip install -r app/serving/requirements.txt
```

- Start the service (run from repository root):

```
bash app/serving/run_model.sh
```

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
- For production, remove `--reload` and consider using a process manager (systemd, docker, etc.).
