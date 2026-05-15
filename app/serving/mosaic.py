from collections.abc import AsyncIterator, Iterator, Sequence
from contextlib import asynccontextmanager
import os
from pathlib import Path
import tempfile
from typing import Any, Protocol, cast
from uuid import uuid4

from dotenv import load_dotenv
from fastapi import (
    BackgroundTasks,
    FastAPI,
    HTTPException,
    Query,
    Request,
    Response,
    status,
)
import replicate
from starlette.concurrency import run_in_threadpool
import uvicorn

from wechat.user import User

SERVICE_DIR = Path(__file__).resolve().parent
PROJECT_ROOT = Path(__file__).resolve().parents[2]
OUTPUT_IMAGE = PROJECT_ROOT / "test" / "output" / "output.png"
PUBLIC_OUTPUT_DIR = PROJECT_ROOT / "app" / "wwwroot" / "output"
MODEL_VERSION = (
    "sczhou/codeformer:7de2ea26c616d5bf2245ad0d5e24f0ff9a6204578a5c876db53142edd9d2cd56"
)


@asynccontextmanager
async def lifespan(_: FastAPI) -> AsyncIterator[None]:
    load_service_env()
    yield


app = FastAPI(title="Mosaic Restored Service", lifespan=lifespan)


class ReplicateFileOutput(Protocol):
    url: str

    def read(self) -> bytes: ...


def env_file_candidates() -> list[Path]:
    candidates = [
        SERVICE_DIR / ".env",
        PROJECT_ROOT / "app" / ".env",
        PROJECT_ROOT / ".env",
        Path.cwd() / ".env",
    ]
    unique_candidates: list[Path] = []
    seen_paths: set[Path] = set()
    for candidate in candidates:
        resolved = candidate.resolve()
        if resolved in seen_paths:
            continue
        seen_paths.add(resolved)
        unique_candidates.append(candidate)
    return unique_candidates


def first_output(result: Any) -> Any:
    if isinstance(result, Iterator):
        return next(result)
    if isinstance(result, Sequence) and not isinstance(result, (str, bytes, bytearray)):
        return result[0]
    return result


def load_service_env() -> None:
    for env_path in env_file_candidates():
        if env_path.is_file():
            load_dotenv(env_path)


def env_flag_enabled(name: str, default: bool = True) -> bool:
    value = os.environ.get(name)
    if value is None:
        return default
    return value.strip().lower() not in {"0", "false", "no", "off"}


def ensure_replicate_token() -> None:
    if not os.environ.get("REPLICATE_API_TOKEN"):
        searched_paths = ", ".join(str(path) for path in env_file_candidates())
        raise RuntimeError(
            f"Missing REPLICATE_API_TOKEN. Searched env files: {searched_paths}"
        )


def image_suffix(image_bytes: bytes) -> str:
    if image_bytes.startswith(b"\x89PNG\r\n\x1a\n"):
        return ".png"
    if image_bytes.startswith(b"\xff\xd8\xff"):
        return ".jpg"
    if image_bytes.startswith(b"RIFF") and image_bytes[8:12] == b"WEBP":
        return ".webp"
    return ".img"


def send_restore_done_notification(output_url: str) -> None:
    if not env_flag_enabled("WECHAT_NOTIFY_ENABLED"):
        return

    content = os.environ.get(
        "WECHAT_RESTORE_DONE_MESSAGE", "图片处理完毕，请回到页面查看结果。"
    )
    if output_url and env_flag_enabled("WECHAT_NOTIFY_INCLUDE_URL", default=False):
        content = f"{content}\n{output_url}"

    try:
        sent = User().send_message(content)
    except Exception as error:
        print(f"Failed to send WeChat restore notification: {error}")
        return

    if not sent:
        print("Failed to send WeChat restore notification.")


def public_base_url(request: Request) -> str:
    configured_base_url = os.environ.get("MOSAIC_PUBLIC_BASE_URL")
    if configured_base_url:
        return configured_base_url.rstrip("/")

    forwarded_host = request.headers.get("x-forwarded-host")
    if forwarded_host:
        forwarded_proto = request.headers.get("x-forwarded-proto", "http")
        return f"{forwarded_proto}://{forwarded_host}".rstrip("/")

    return str(request.base_url).rstrip("/")


def save_public_output_image(restored_bytes: bytes) -> str:
    PUBLIC_OUTPUT_DIR.mkdir(parents=True, exist_ok=True)

    filename = f"restored-{uuid4().hex}{image_suffix(restored_bytes)}"
    output_path = PUBLIC_OUTPUT_DIR / filename
    output_path.write_bytes(restored_bytes)
    return f"/output/{filename}"


def generate_restored_image(
    image_bytes: bytes,
    upscale: int,
    face_upsample: bool,
    background_enhance: bool,
    codeformer_fidelity: float,
) -> tuple[bytes, str]:
    ensure_replicate_token()

    with tempfile.NamedTemporaryFile(suffix=image_suffix(image_bytes)) as image_file:
        image_file.write(image_bytes)
        image_file.flush()
        image_file.seek(0)

        result = replicate.run(
            MODEL_VERSION,
            input={
                "image": Path(image_file.name),
                "upscale": upscale,
                "face_upsample": face_upsample,
                "background_enhance": background_enhance,
                "codeformer_fidelity": codeformer_fidelity,
            },
        )

    output = cast(ReplicateFileOutput, first_output(result))
    return output.read(), output.url


@app.get("/health")
def health() -> dict[str, str]:
    return {
        "status": "ok",
        "replicate_token": (
            "configured" if os.environ.get("REPLICATE_API_TOKEN") else "missing"
        ),
    }


@app.post("/restore")
async def restore(
    request: Request,
    background_tasks: BackgroundTasks,
    upscale: int = Query(default=2, ge=1, le=4),
    face_upsample: bool = True,
    background_enhance: bool = True,
    codeformer_fidelity: float = Query(default=0.2, ge=0.0, le=1.0),
    save: bool = False,
) -> Response:
    image_bytes = await request.body()
    if not image_bytes:
        raise HTTPException(
            status_code=status.HTTP_400_BAD_REQUEST,
            detail="Request body must contain image bytes.",
        )

    try:
        restored_bytes, output_url = await run_in_threadpool(
            generate_restored_image,
            image_bytes,
            upscale,
            face_upsample,
            background_enhance,
            codeformer_fidelity,
        )
    except RuntimeError as error:
        raise HTTPException(
            status_code=status.HTTP_500_INTERNAL_SERVER_ERROR, detail=str(error)
        ) from error
    except Exception as error:
        raise HTTPException(
            status_code=status.HTTP_502_BAD_GATEWAY,
            detail=f"Replicate request failed: {error}",
        ) from error

    if save:
        OUTPUT_IMAGE.parent.mkdir(parents=True, exist_ok=True)
        OUTPUT_IMAGE.write_bytes(restored_bytes)

    public_output_url = ""
    try:
        public_path = save_public_output_image(restored_bytes)
        public_output_url = f"{public_base_url(request)}{public_path}"
    except Exception as error:
        print(f"Failed to save public output image: {error}")

    background_tasks.add_task(send_restore_done_notification, public_output_url)

    headers = {"X-Replicate-Output-Url": output_url}
    if public_output_url:
        headers["X-Output-Url"] = public_output_url

    return Response(
        content=restored_bytes,
        media_type="image/png",
        headers=headers,
    )


if __name__ == "__main__":
    load_service_env()
    uvicorn.run(app, host="127.0.0.1", port=8091)
