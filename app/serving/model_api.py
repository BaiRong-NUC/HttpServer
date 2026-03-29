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

        input_cols = MANIFEST['example_input_columns']
        selected = MANIFEST['selected_base_features']
        final_cols = MANIFEST['final_input_columns']

        if len(req.features) != len(input_cols):
            raise ValueError(
                f"期望 {len(input_cols)} 个特征值（顺序：{input_cols}），实际收到 {len(req.features)} 个"
            )

        # Step 1: 原始 8 列标准化（preprocessor 在原始特征上 fit）
        df_raw = pd.DataFrame([req.features], columns=input_cols)
        scaled = transformer.transform(df_raw)
        df_scaled = pd.DataFrame(scaled, columns=input_cols)

        # Step 2: 筛选 selected_base_features
        df = df_scaled[selected].copy()

        # Step 3: 构建交互项（基于标准化后的值）
        if 'Glucose_BMI' in final_cols and 'Glucose' in df.columns and 'BMI' in df.columns:
            df['Glucose_BMI'] = df['Glucose'] * df['BMI']
        if 'Age_BMI' in final_cols and 'Age' in df.columns and 'BMI' in df.columns:
            df['Age_BMI'] = df['Age'] * df['BMI']
        if 'Glucose_BP' in final_cols and 'Glucose' in df.columns and 'BloodPressure' in df.columns:
            df['Glucose_BP'] = df['Glucose'] * df['BloodPressure']

        # Step 4: 按 final_input_columns 顺序排列
        df = df[final_cols]

        pred = model.predict(df.values, verbose=0)
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
