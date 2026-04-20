const outputEl = document.getElementById("api-output");
const statusEl = document.getElementById("api-status");

function setStatus(text, isError = false) {
    statusEl.textContent = text;
    statusEl.style.background = isError
        ? "rgba(220, 106, 86, 0.15)"
        : "rgba(25, 114, 120, 0.1)";
    statusEl.style.color = isError ? "#8b2b18" : "#0f646a";
}

async function runApiTest(url, options, title) {
    setStatus("请求中...");
    outputEl.textContent = "";
    try {
        const resp = await fetch(url, options);
        const text = await resp.text();
        setStatus(`完成: ${resp.status} ${resp.statusText}`);
        outputEl.textContent = [
            `[${title}]`,
            `URL: ${url}`,
            `METHOD: ${options.method || "GET"}`,
            `STATUS: ${resp.status} ${resp.statusText}`,
            "",
            "RESPONSE:",
            text,
        ].join("\n");
    } catch (err) {
        setStatus("请求失败", true);
        outputEl.textContent = [
            `[${title}]`,
            `URL: ${url}`,
            `METHOD: ${options.method || "GET"}`,
            "",
            "ERROR:",
            err instanceof Error ? err.message : String(err),
        ].join("\n");
    }
}

document.getElementById("btn-hello").addEventListener("click", () => {
    runApiTest("/hello", {}, "测试 GET /hello");
});

document.getElementById("btn-login").addEventListener("click", () => {
    runApiTest(
        "/login",
        {
            method: "POST",
            headers: {
                "Content-Type": "application/x-www-form-urlencoded",
            },
            body: "username=test&password=123456",
        },
        "测试 POST /login",
    );
});

document.getElementById("btn-update").addEventListener("click", () => {
    runApiTest(
        "/update",
        {
            method: "PUT",
            headers: {
                "Content-Type": "application/x-www-form-urlencoded",
            },
            body: "id=1&field=value",
        },
        "测试 PUT /update",
    );
});

document.getElementById("btn-delete").addEventListener("click", () => {
    runApiTest(
        "/delete",
        {
            method: "DELETE",
            headers: {
                "Content-Type": "application/x-www-form-urlencoded",
            },
            body: "id=1",
        },
        "测试 DELETE /delete",
    );
});

document.getElementById("btn-mlpredict").addEventListener("click", () => {
    runApiTest(
        "/ml_predict",
        {
            method: "POST",
            headers: {
                "Content-Type": "application/json",
            },
            body: JSON.stringify({
                // 已替换为用户提供的样例数据（按 manifest 的 example_input_columns 顺序）
                // 示例数据: [Pregnancies, Glucose, BloodPressure, SkinThickness, Insulin, BMI, DiabetesPedigreeFunction, Age]
                features: [6, 148, 72, 35, 0, 33.6, 0.627, 50],
            }),
        },
        "测试 POST /ml_predict",
    );
});
