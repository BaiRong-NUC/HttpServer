function copyToClipboard(text) {
    if (navigator.clipboard && window.isSecureContext) {
        return navigator.clipboard.writeText(text);
    }
    // HTTP 环境 fallback
    const ta = document.createElement("textarea");
    ta.value = text;
    ta.style.cssText = "position:fixed;top:-9999px;left:-9999px;opacity:0";
    document.body.appendChild(ta);
    ta.focus();
    ta.select();
    const ok = document.execCommand("copy");
    document.body.removeChild(ta);
    return ok ? Promise.resolve() : Promise.reject(new Error("copy failed"));
}

const copyBtn = document.getElementById("copy-email");
copyBtn.addEventListener("click", function () {
    const btn = this;
    copyToClipboard("gbr@edu.nuc.email")
        .then(() => {
            btn.textContent = "✓ 已复制";
            btn.classList.add("btn-feedback");
            setTimeout(() => {
                btn.textContent = "复制邮箱";
                btn.classList.remove("btn-feedback");
            }, 1500);
        })
        .catch(() => {
            btn.textContent = "复制失败";
            setTimeout(() => (btn.textContent = "复制邮箱"), 1500);
        });
});

document.getElementById("export-pdf").addEventListener("click", function () {
    const btn = this;
    btn.textContent = "正在准备...";
    btn.classList.add("btn-feedback");
    setTimeout(() => {
        window.print();
        btn.textContent = "📄 导出为 PDF";
        btn.classList.remove("btn-feedback");
    }, 200);
});
