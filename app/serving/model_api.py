from fastapi import FastAPI, HTTPException
from pydantic import BaseModel
import numpy as np
import joblib
from tensorflow import keras # type: ignore
import os

app = FastAPI(title="Model Serving")


class PredictRequest(BaseModel):
    features: list


def _load_paths():
    base = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
    model_path = os.path.join(base, 'artifacts', 'diabetes_model.keras')
    pre_path = os.path.join(base, 'artifacts', 'preprocessor.joblib')
    return model_path, pre_path


MODEL_PATH, PRE_PATH = _load_paths()


try:
    model = keras.models.load_model(MODEL_PATH)
except Exception as e:
    model = None
    print(f"Failed to load model: {e}")

try:
    pre = joblib.load(PRE_PATH)
except Exception as e:
    pre = None
    print(f"Failed to load preprocessor: {e}")


@app.get("/health")
def health():
    return {"status": "ok", "model_loaded": model is not None}


@app.post("/predict")
def predict(req: PredictRequest):
    if model is None or pre is None:
        raise HTTPException(status_code=500, detail="Model or preprocessor not loaded")
    try:
        x = np.array(req.features).reshape(1, -1)
        # pre can be a preprocessor object with transform(), or a dict containing a 'scaler'
        if hasattr(pre, 'transform'):
            x_p = pre.transform(x)
        elif isinstance(pre, dict) and 'scaler' in pre and hasattr(pre['scaler'], 'transform'):
            x_p = pre['scaler'].transform(x)
        else:
            raise RuntimeError('preprocessor does not support transform')
        pred = model.predict(x_p)
        # convert numpy arrays to python types
        return {"pred": pred.tolist()}
    except Exception as e:
        raise HTTPException(status_code=400, detail=str(e))


if __name__ == "__main__":
    import uvicorn
    uvicorn.run("app.serving.model_api:app", host="127.0.0.1", port=8000, log_level="info")
