from fastapi import FastAPI, HTTPException
from pydantic import BaseModel
import pandas as pd
import numpy as np
import joblib
import json
from tensorflow import keras # type: ignore
import os

app = FastAPI(title="Model Serving")


class PredictRequest(BaseModel):
    features: list


def _load_artifacts():
    base = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
    manifest_path = os.path.join(base, 'artifacts', 'manifest.json')
    with open(manifest_path, 'r') as f:
        manifest = json.load(f)
    model_path = os.path.join(base, 'artifacts', manifest['model_path'])
    pre_path = os.path.join(base, 'artifacts', manifest['preprocessor_path'])
    return model_path, pre_path, manifest


MODEL_PATH, PRE_PATH, MANIFEST = _load_artifacts()


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


def _get_transformer(pre):
    if hasattr(pre, 'transform'):
        return pre
    if isinstance(pre, dict) and 'scaler' in pre and hasattr(pre['scaler'], 'transform'):
        return pre['scaler']
    return None


def _build_features(raw: list) -> pd.DataFrame:
    """按 manifest 流程：原始输入 -> 筛选基础特征 -> 构建交互项 -> final_input_columns"""
    input_cols = MANIFEST['example_input_columns']
    selected = MANIFEST['selected_base_features']
    final_cols = MANIFEST['final_input_columns']

    if len(raw) != len(input_cols):
        raise ValueError(
            f"期望 {len(input_cols)} 个特征值（顺序：{input_cols}），实际收到 {len(raw)} 个"
        )

    df = pd.DataFrame([raw], columns=input_cols)

    # 筛选基础特征
    df = df[selected].copy()

    # 构建交互特征
    if 'Glucose_BMI' in final_cols and 'Glucose' in df.columns and 'BMI' in df.columns:
        df['Glucose_BMI'] = df['Glucose'] * df['BMI']
    if 'Age_BMI' in final_cols and 'Age' in df.columns and 'BMI' in df.columns:
        df['Age_BMI'] = df['Age'] * df['BMI']
    if 'Glucose_BP' in final_cols and 'Glucose' in df.columns and 'BloodPressure' in df.columns:
        df['Glucose_BP'] = df['Glucose'] * df['BloodPressure']

    # 按 final_input_columns 顺序排列
    df = df[final_cols]
    return df


@app.get("/health")
def health():
    return {"status": "ok", "model_loaded": model is not None}


@app.post("/predict")
def predict(req: PredictRequest):
    if model is None or pre is None:
        raise HTTPException(status_code=500, detail="Model or preprocessor not loaded")
    try:
        transformer = _get_transformer(pre)
        if transformer is None:
            raise RuntimeError('preprocessor does not support transform')

        df = _build_features(req.features)
        x_p = transformer.transform(df)
        pred = model.predict(x_p, verbose=0)
        prob = float(pred[0][0])
        threshold = MANIFEST.get('threshold', 0.5)
        return {
            "probability": prob,
            "label": int(prob > threshold),
            "diagnosis": "糖尿病" if prob > threshold else "健康"
        }
    except Exception as e:
        raise HTTPException(status_code=400, detail=str(e))


if __name__ == "__main__":
    import uvicorn
    uvicorn.run("app.serving.model_api:app", host="127.0.0.1", port=8000, log_level="info")
