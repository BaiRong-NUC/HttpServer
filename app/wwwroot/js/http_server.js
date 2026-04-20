const outputEl = document.getElementById("api-output");
const statusEl = document.getElementById("api-status");

function runApiTest(url, options, title) {
    HttpApi.runTextRequest({
        url,
        options,
        title,
        outputEl,
        statusEl,
    });
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
