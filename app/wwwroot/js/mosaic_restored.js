const statusEl = document.getElementById("status");
const fileInputEl = document.getElementById("fileInput");
const dropzoneEl = document.getElementById("dropzone");
const fileMetaEl = document.getElementById("fileMeta");
const inputBodyEl = document.getElementById("inputBody");
const resultBodyEl = document.getElementById("resultBody");
const resultInfoEl = document.getElementById("resultInfo");
const downloadLinkEl = document.getElementById("downloadLink");
const publicLinkEl = document.getElementById("publicLink");
const replicateLinkEl = document.getElementById("replicateLink");
const restoreButtonEl = document.getElementById("restoreButton");
const clearButtonEl = document.getElementById("clearButton");
const fidelityEl = document.getElementById("fidelity");
const fidelityValueEl = document.getElementById("fidelityValue");
const upscaleEl = document.getElementById("upscale");
const faceUpsampleEl = document.getElementById("faceUpsample");
const backgroundEnhanceEl = document.getElementById("backgroundEnhance");

const DEFAULT_IMAGE_SRC = "/image/test.jpg";
const DEFAULT_IMAGE_NAME = "test.jpg";

let selectedFile = null;
let inputObjectUrl = "";
let outputObjectUrl = "";
let comparePosition = 52;

function formatFileSize(size) {
    if (size < 1024) {
        return `${size} B`;
    }
    if (size < 1024 * 1024) {
        return `${(size / 1024).toFixed(1)} KB`;
    }
    return `${(size / (1024 * 1024)).toFixed(2)} MB`;
}

function revokeUrl(url) {
    if (url) {
        URL.revokeObjectURL(url);
    }
}

function setStatus(text, state = "ready") {
    statusEl.textContent = text;
    statusEl.dataset.state = state;
}

function renderStageImage(container, src, alt) {
    container.innerHTML = "";
    const image = document.createElement("img");
    image.src = src;
    image.alt = alt;
    container.appendChild(image);
}

function updateComparePosition(compareEl, value) {
    comparePosition = Number(value);
    compareEl.style.setProperty("--compare-position", `${comparePosition}%`);
}

function syncCompareAspect(compareEl, imageEl) {
    const applyAspect = () => {
        const { naturalWidth, naturalHeight } = imageEl;
        if (!naturalWidth || !naturalHeight) {
            return;
        }
        compareEl.style.setProperty(
            "--compare-aspect-ratio",
            `${naturalWidth} / ${naturalHeight}`,
        );
        compareEl.style.setProperty(
            "--compare-ratio-number",
            String(naturalWidth / naturalHeight),
        );
    };

    if (imageEl.complete) {
        applyAspect();
        return;
    }

    imageEl.addEventListener("load", applyAspect, { once: true });
}

function renderCompareStage(container, beforeSrc, afterSrc) {
    container.innerHTML = "";

    const compareEl = document.createElement("div");
    compareEl.className = "compare-view";
    compareEl.innerHTML = `
        <div class="compare-layer compare-base">
            <img src="${afterSrc}" alt="修复结果图" />
        </div>
        <div class="compare-layer compare-overlay">
            <img src="${beforeSrc}" alt="原始上传图片" />
        </div>
        <div class="compare-divider" aria-hidden="true">
            <span class="compare-handle"></span>
        </div>
        <span class="compare-label compare-label--before">Before</span>
        <span class="compare-label compare-label--after">After</span>
        <input
            class="compare-slider"
            type="range"
            min="0"
            max="100"
            value="${comparePosition}"
            aria-label="调整修复前后对比位置"
        />
    `;

    const sliderEl = compareEl.querySelector(".compare-slider");
    const baseImageEl = compareEl.querySelector(".compare-base img");
    sliderEl.addEventListener("input", (event) => {
        updateComparePosition(compareEl, event.target.value);
    });

    syncCompareAspect(compareEl, baseImageEl);
    updateComparePosition(compareEl, comparePosition);
    container.appendChild(compareEl);
}

function renderPlaceholder(container, title, subtitle) {
    container.innerHTML = `
        <div class="placeholder-card">
            <strong>${title}</strong>
            <span>${subtitle}</span>
        </div>
    `;
}

function resetResultArea() {
    revokeUrl(outputObjectUrl);
    outputObjectUrl = "";
    comparePosition = 52;
    renderPlaceholder(resultBodyEl, "等待处理", "完成后将在此展示结果图");
    resultInfoEl.textContent = "尚未发起请求";
    downloadLinkEl.hidden = true;
    downloadLinkEl.removeAttribute("href");
    publicLinkEl.hidden = true;
    publicLinkEl.removeAttribute("href");
    replicateLinkEl.hidden = true;
    replicateLinkEl.removeAttribute("href");
}

function updateButtons() {
    const hasFile = Boolean(selectedFile);
    restoreButtonEl.disabled = !hasFile;
    clearButtonEl.disabled = !hasFile;
}

function setSelectedFile(file) {
    selectedFile = file;
    revokeUrl(inputObjectUrl);
    inputObjectUrl = "";

    if (!file) {
        fileMetaEl.textContent = "未选择文件";
        renderPlaceholder(inputBodyEl, "尚未选择图片", "支持 JPG、PNG、WEBP");
        resetResultArea();
        updateButtons();
        setStatus("等待上传");
        return;
    }

    inputObjectUrl = URL.createObjectURL(file);
    renderStageImage(inputBodyEl, inputObjectUrl, file.name || "上传图片预览");
    fileMetaEl.textContent = `${file.name} · ${file.type || "未知类型"} · ${formatFileSize(file.size)}`;
    resetResultArea();
    updateButtons();
    setStatus("图片已就绪");
}

async function loadDefaultImage() {
    try {
        const response = await fetch(DEFAULT_IMAGE_SRC);
        if (!response.ok) {
            throw new Error(`默认图片加载失败: ${response.status}`);
        }

        const blob = await response.blob();
        const file = new File([blob], DEFAULT_IMAGE_NAME, {
            type: blob.type || "image/jpeg",
        });
        setSelectedFile(file);
        setStatus("默认图片已就绪");
    } catch (error) {
        selectedFile = null;
        inputObjectUrl = DEFAULT_IMAGE_SRC;
        renderStageImage(inputBodyEl, DEFAULT_IMAGE_SRC, "默认示例图片");
        fileMetaEl.textContent = "默认示例图片";
        resetResultArea();
        updateButtons();
        setStatus("默认图片加载失败", "error");
    }
}

function buildQuery() {
    const params = new URLSearchParams();
    params.set("upscale", upscaleEl.value);
    params.set("face_upsample", String(faceUpsampleEl.checked));
    params.set("background_enhance", String(backgroundEnhanceEl.checked));
    params.set("codeformer_fidelity", fidelityEl.value);
    return params.toString();
}

function formatResponseInfo(response, publicUrl, replicateUrl) {
    const lines = [
        "[POST /api/restore]",
        `STATUS: ${response.status} ${response.statusText}`,
        `UPSCALE: ${upscaleEl.value}`,
        `FIDELITY: ${fidelityEl.value}`,
        `FACE_UPSAMPLE: ${faceUpsampleEl.checked}`,
        `BACKGROUND_ENHANCE: ${backgroundEnhanceEl.checked}`,
    ];

    if (publicUrl) {
        lines.push(`PUBLIC_URL: ${publicUrl}`);
    }
    if (replicateUrl) {
        lines.push(`REPLICATE_URL: ${replicateUrl}`);
    }

    return lines.join("\n");
}

async function handleRestore() {
    if (!selectedFile) {
        return;
    }

    setStatus("处理中...", "busy");
    restoreButtonEl.disabled = true;
    resultInfoEl.textContent = "请求已发送，等待后端返回结果...";

    try {
        const response = await fetch(`/api/restore?${buildQuery()}`, {
            method: "POST",
            headers: {
                "Content-Type": selectedFile.type || "application/octet-stream",
            },
            body: selectedFile,
        });

        const publicUrl = response.headers.get("X-Output-Url") || "";
        const replicateUrl =
            response.headers.get("X-Replicate-Output-Url") || "";

        if (!response.ok) {
            const errorText = await response.text();
            const errorJson = HttpApi.tryParseJson(errorText);
            const detail = errorJson?.detail || errorText || "未知错误";
            setStatus("请求失败", "error");
            resultInfoEl.textContent = `[POST /api/restore]\nSTATUS: ${response.status} ${response.statusText}\n\nERROR:\n${detail}`;
            renderPlaceholder(
                resultBodyEl,
                "处理失败",
                "请检查服务日志或更换测试图片",
            );
            return;
        }

        const blob = await response.blob();
        revokeUrl(outputObjectUrl);
        outputObjectUrl = URL.createObjectURL(blob);

        if (inputObjectUrl) {
            renderCompareStage(resultBodyEl, inputObjectUrl, outputObjectUrl);
        } else {
            renderStageImage(resultBodyEl, outputObjectUrl, "修复结果图");
        }

        downloadLinkEl.href = outputObjectUrl;
        downloadLinkEl.hidden = false;
        publicLinkEl.hidden = !publicUrl;
        if (publicUrl) {
            publicLinkEl.href = publicUrl;
        }
        replicateLinkEl.hidden = !replicateUrl;
        if (replicateUrl) {
            replicateLinkEl.href = replicateUrl;
        }

        resultInfoEl.textContent = formatResponseInfo(
            response,
            publicUrl,
            replicateUrl,
        );
        setStatus("处理完成");
    } catch (error) {
        const message = error instanceof Error ? error.message : String(error);
        setStatus("请求失败", "error");
        resultInfoEl.textContent = `[POST /api/restore]\nERROR:\n${message}`;
        renderPlaceholder(
            resultBodyEl,
            "网络错误",
            "请确认 C++ 服务和 Python 服务均已启动",
        );
    } finally {
        updateButtons();
    }
}

function handleFileInput(files) {
    const [file] = files || [];
    if (!file) {
        return;
    }
    if (!file.type.startsWith("image/")) {
        setStatus("文件类型无效", "error");
        fileMetaEl.textContent = "请选择图片文件";
        return;
    }
    setSelectedFile(file);
}

dropzoneEl.addEventListener("click", () => fileInputEl.click());
fileInputEl.addEventListener("change", (event) => {
    handleFileInput(event.target.files);
});

["dragenter", "dragover"].forEach((eventName) => {
    dropzoneEl.addEventListener(eventName, (event) => {
        event.preventDefault();
        dropzoneEl.classList.add("is-dragover");
    });
});

["dragleave", "drop"].forEach((eventName) => {
    dropzoneEl.addEventListener(eventName, (event) => {
        event.preventDefault();
        dropzoneEl.classList.remove("is-dragover");
    });
});

dropzoneEl.addEventListener("drop", (event) => {
    handleFileInput(event.dataTransfer?.files);
});

fidelityEl.addEventListener("input", () => {
    fidelityValueEl.textContent = Number(fidelityEl.value).toFixed(2);
});

restoreButtonEl.addEventListener("click", handleRestore);
clearButtonEl.addEventListener("click", () => {
    fileInputEl.value = "";
    setSelectedFile(null);
});

loadDefaultImage();
