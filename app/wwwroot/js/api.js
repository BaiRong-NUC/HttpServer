(function () {
    function setStatus(statusEl, text, isError = false) {
        if (!statusEl) {
            return;
        }
        statusEl.textContent = text;
        statusEl.style.background = isError
            ? "rgba(220, 106, 86, 0.15)"
            : "rgba(25, 114, 120, 0.1)";
        statusEl.style.color = isError ? "#8b2b18" : "#0f646a";
    }

    function setOutput(outputEl, content = "") {
        if (!outputEl) {
            return;
        }
        outputEl.textContent = content;
    }

    function renderLines(outputEl, lines) {
        setOutput(outputEl, lines.join("\n"));
    }

    function formatTextBlockLines({
        title,
        url,
        method = "GET",
        status,
        request,
        response,
        error,
    }) {
        const lines = [];

        if (title) {
            lines.push(`[${title}]`);
        }

        lines.push(`URL: ${url}`);
        lines.push(`METHOD: ${method}`);

        if (status) {
            lines.push(`STATUS: ${status}`);
        }

        if (request !== undefined) {
            lines.push("");
            lines.push("REQUEST:");
            lines.push(
                typeof request === "string"
                    ? request
                    : JSON.stringify(request, null, 2),
            );
        }

        if (response !== undefined) {
            lines.push("");
            lines.push("RESPONSE:");
            lines.push(response);
        }

        if (error !== undefined) {
            lines.push("");
            lines.push("ERROR:");
            lines.push(error);
        }

        return lines;
    }

    async function requestText(url, options = {}) {
        const response = await fetch(url, options);
        const text = await response.text();
        return { response, text };
    }

    function tryParseJson(text) {
        try {
            return JSON.parse(text);
        } catch {
            return null;
        }
    }

    async function runTextRequest({
        url,
        options = {},
        title,
        outputEl,
        statusEl,
        requestBody,
        responseTextFormatter,
    }) {
        setStatus(statusEl, "请求中...");
        setOutput(outputEl, "");

        try {
            const { response, text } = await requestText(url, options);
            const status = `${response.status} ${response.statusText}`;
            const formattedResponse = responseTextFormatter
                ? responseTextFormatter(text, response)
                : text;

            setStatus(statusEl, `完成: ${status}`);
            renderLines(
                outputEl,
                formatTextBlockLines({
                    title,
                    url,
                    method: options.method || "GET",
                    status,
                    request: requestBody,
                    response: formattedResponse,
                }),
            );

            return { ok: true, response, text };
        } catch (error) {
            const message =
                error instanceof Error ? error.message : String(error);

            setStatus(statusEl, "请求失败", true);
            renderLines(
                outputEl,
                formatTextBlockLines({
                    title,
                    url,
                    method: options.method || "GET",
                    error: message,
                }),
            );

            return { ok: false, error };
        }
    }

    window.HttpApi = {
        setStatus,
        setOutput,
        renderLines,
        requestText,
        tryParseJson,
        runTextRequest,
    };
})();
