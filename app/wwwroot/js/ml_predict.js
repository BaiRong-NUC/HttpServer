const outputEl = document.getElementById("api-output");
const statusEl = document.getElementById("api-status");
const formEl = document.getElementById("predict-form");
const exampleBtn = document.getElementById("btn-example");

const fieldIds = [
    "pregnancies",
    "glucose",
    "blood-pressure",
    "skin-thickness",
    "insulin",
    "bmi",
    "dpf",
    "age",
];
const exampleFeatures = [6, 148, 72, 35, 0, 33.6, 0.627, 50];

// r=54  =>  C = 2π×54 ≈ 339.29
const RING_C = 2 * Math.PI * 54;

// 保存弹窗打开前的焦点，关闭时恢复；并在弹窗打开时锁定 body 滚动（移动端避免背景滚动/地址栏抖动）
let _prevFocus = null;
let _scrollY = 0;
let _prevBodyStyles = {};

function fillExampleValues() {
    fieldIds.forEach((id, i) => {
        document.getElementById(id).value = String(exampleFeatures[i]);
    });
}

function collectFeatures() {
    return fieldIds.map((id) => {
        const v = Number(document.getElementById(id).value);
        if (!Number.isFinite(v)) throw new Error(`字段 ${id} 输入无效`);
        return v;
    });
}

/* -------- 诊断结果弹窗 -------- */
function showResult(data) {
    const isDanger = data.label === 1;
    const prob = data.probability;
    const pct = Math.round(prob * 100);
    const cls = isDanger ? "danger" : "safe";

    // 图标
    const iconWrap = document.getElementById("result-icon-wrap");
    iconWrap.className = `result-icon-wrap ${cls}`;
    const iconSvg = document.getElementById("result-icon-svg");
    if (isDanger) {
        iconSvg.setAttribute("stroke", "#e4572e");
        iconSvg.innerHTML =
            '<path d="M10.29 3.86L1.82 18a2 2 0 0 0 1.71 3h16.94a2 2 0 0 0 1.71-3L13.71 3.86a2 2 0 0 0-3.42 0z"/>' +
            '<line x1="12" y1="9" x2="12" y2="13"/>' +
            '<line x1="12" y1="17" x2="12.01" y2="17"/>';
    } else {
        iconSvg.setAttribute("stroke", "#197278");
        iconSvg.innerHTML =
            '<path d="M22 11.08V12a10 10 0 1 1-5.93-9.14"/>' +
            '<polyline points="22 4 12 14.01 9 11.01"/>';
    }

    // 标题 & 副标题
    const titleEl = document.getElementById("result-title");
    titleEl.textContent = isDanger ? "糖尿病风险警示" : "检测结果正常";
    titleEl.className = `result-title ${cls}`;
    document.getElementById("result-subtitle").textContent = isDanger
        ? "模型检测到较高患病风险，请尽快咨询医生进行确诊"
        : "未检测到明显糖尿病风险，请保持健康生活方式";

    // 圆环动画
    const ring = document.getElementById("ring-fill");
    ring.className = `ring-fill ${cls}`;
    // 使用 SVG 属性而不是 style，这在某些浏览器/平台上更可靠
    ring.setAttribute("stroke-dasharray", String(RING_C));
    ring.setAttribute("stroke-dashoffset", String(RING_C)); // 从 0 开始
    ring.setAttribute("stroke-linecap", "round");
    // 明确设置 stroke 为对应的 gradient，避免样式解析问题
    if (isDanger) {
        ring.setAttribute("stroke", "url(#dangerGrad)");
    } else {
        ring.setAttribute("stroke", "url(#safeGrad)");
    }
    requestAnimationFrame(() =>
        requestAnimationFrame(() => {
            ring.setAttribute("stroke-dashoffset", String(RING_C * (1 - prob)));
        }),
    );

    // 概率文字
    const probEl = document.getElementById("prob-pct");
    probEl.textContent = `${pct}%`;
    probEl.className = `prob-pct ${cls}`;

    // 风险标签
    const badge = document.getElementById("risk-badge");
    badge.textContent = isDanger
        ? pct >= 80
            ? "极高风险"
            : pct >= 60
              ? "高风险"
              : "中等风险"
        : pct >= 30
          ? "低风险"
          : "极低风险";
    badge.className = `risk-badge ${cls}`;

    // 临床建议
    const items = isDanger
        ? [
              "建议尽快前往内分泌科进行空腹血糖及糖化血红蛋白 (HbA1c) 检查",
              "控制碳水化合物摄入，增加膳食纤维，严格减少精制糖与含糖饮料",
              "每周保持 150 分钟以上中等强度有氧运动（快走、游泳等）",
              "定期监测血压与体重，目标 BMI < 24，收缩压 < 130 mmHg",
          ]
        : [
              "保持均衡饮食，每年进行一次空腹血糖筛查",
              "维持规律的有氧运动习惯，保持健康体重（BMI 18.5–23.9）",
              "避免长期高糖、高脂饮食，减少久坐时间",
          ];
    document.getElementById("advice-list").innerHTML = items
        .map((t) => `<li class="${cls}-item">${t}</li>`)
        .join("");

    // 记录当前焦点并锁定背景滚动，避免移动端地址栏/视口抖动
    try {
        _prevFocus = document.activeElement;
    } catch (e) {
        _prevFocus = null;
    }
    // 如果当前有输入元素聚焦，先 blur，防止 iOS 在打开弹窗时放大视图
    try {
        if (
            document.activeElement &&
            /INPUT|TEXTAREA/i.test(document.activeElement.tagName)
        ) {
            document.activeElement.blur();
        }
    } catch (e) {}

    // 更可靠的滚动锁定：记录滚动位置并将 body 设为 fixed
    _scrollY = window.scrollY || window.pageYOffset || 0;
    // 保存当前行内样式以便恢复，避免覆盖已有样式
    _prevBodyStyles = {
        position: document.body.style.position || "",
        top: document.body.style.top || "",
        left: document.body.style.left || "",
        right: document.body.style.right || "",
    };
    document.body.style.position = "fixed";
    document.body.style.top = `-${_scrollY}px`;
    document.body.style.left = "0";
    document.body.style.right = "0";
    document.getElementById("result-overlay").classList.remove("hidden");
    // 将焦点移到弹窗的关闭按钮，便于键盘/屏幕阅读器操作
    try {
        document.getElementById("result-close").focus();
    } catch (e) {}
}

function closeResult() {
    document.getElementById("result-overlay").classList.add("hidden");
    // 恢复 body 行内样式并回滚到先前位置
    try {
        document.body.style.position = _prevBodyStyles.position;
        document.body.style.top = _prevBodyStyles.top;
        document.body.style.left = _prevBodyStyles.left;
        document.body.style.right = _prevBodyStyles.right;
        window.scrollTo(0, _scrollY || 0);
    } catch (e) {}

    // 将焦点回传给先前元素（若可用）
    try {
        if (_prevFocus && typeof _prevFocus.focus === "function") {
            _prevFocus.focus();
        }
    } catch (e) {}
    _prevFocus = null;
    _scrollY = 0;

    formEl.reset();
}

document.getElementById("result-close").addEventListener("click", () => {
    closeResult();
    formEl.reset();
    // outputEl.textContent = "尚未发起请求";
    // setStatus("等待请求");
});
document.getElementById("result-close2").addEventListener("click", () => {
    closeResult();
    formEl.reset();
    // outputEl.textContent = "尚未发起请求";
    // setStatus("等待请求");
});
document.getElementById("result-overlay").addEventListener("click", (e) => {
    if (e.target === e.currentTarget) closeResult();
});
document.getElementById("result-repredict").addEventListener("click", () => {
    closeResult();
    formEl.reset();
    fillExampleValues();
    outputEl.textContent = "尚未发起请求";
    setStatus("等待请求");
});

/* -------- 预测请求 -------- */
async function runPredict(features) {
    const payload = { features };
    const result = await HttpApi.runTextRequest({
        url: "/ml_predict",
        options: {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify(payload),
        },
        title: "POST /ml_predict",
        outputEl,
        statusEl,
        requestBody: payload,
        responseTextFormatter: (text) => {
            const parsed = HttpApi.tryParseJson(text);
            return parsed ? JSON.stringify(parsed, null, 2) : text;
        },
    });

    if (!result.ok) {
        return;
    }

    const data = HttpApi.tryParseJson(result.text);
    if (data && "label" in data && "probability" in data) {
        showResult(data);
    }
}

formEl.addEventListener("submit", async (e) => {
    e.preventDefault();
    try {
        await runPredict(collectFeatures());
    } catch (err) {
        HttpApi.setStatus(statusEl, "输入有误", true);
        outputEl.textContent = err instanceof Error ? err.message : String(err);
    }
});

exampleBtn.addEventListener("click", () => {
    fillExampleValues();
    HttpApi.setStatus(statusEl, "示例数据已填充");
});

fillExampleValues();
