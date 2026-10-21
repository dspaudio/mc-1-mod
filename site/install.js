(() => {
  "use strict";

  const isObject = value => value !== null && typeof value === "object" && !Array.isArray(value);
  const isHash = value => typeof value === "string" && /^[a-f0-9]{64}$/i.test(value);
  const isByteCount = value => Number.isSafeInteger(value) && value >= 0;
  const isText = value => typeof value === "string" && value.length > 0;

  function validManifest(data) {
    return isObject(data)
      && isText(data.product)
      && isText(data.version)
      && data.experimental === true
      && data.hardware_verified === false
      && data.subjective_listening_approved === false
      && Array.isArray(data.files)
      && data.files.length > 0
      && data.files.every(file => isObject(file)
        && isText(file.name) && isText(file.url) && isByteCount(file.bytes)
        && isHash(file.sha256) && isText(file.kind))
      && isObject(data.target)
      && isHash(data.target.source_input_sha256)
      && isByteCount(data.target.globals_bytes)
      && isByteCount(data.target.pool_bytes)
      && isByteCount(data.target.app_bytes)
      && isObject(data.wasm)
      && isHash(data.wasm.source_input_sha256)
      && isHash(data.wasm.sha256)
      && isByteCount(data.wasm.bytes)
      && Array.isArray(data.limitations)
      && data.limitations.every(isText);
  }

  function addRow(list, label, value) {
    const term = document.createElement("dt");
    const description = document.createElement("dd");
    term.textContent = label;
    description.textContent = value;
    list.append(term, description);
  }

  function render(data) {
    document.title = `펌웨어 다운로드 ${data.version} experimental — mc-1-mod`;
    const metadata = document.querySelector("#metadata");
    addRow(metadata, "제품", data.product);
    addRow(metadata, "버전", data.version);
    addRow(metadata, "상태", "experimental · 장치 미검증 · 청취 미승인");

    const files = document.querySelector("#files");
    for (const file of data.files) {
      const entry = document.createElement("article");
      entry.className = "file";
      const name = document.createElement("div");
      name.className = "file-name";
      name.textContent = file.name;
      const detail = document.createElement("div");
      detail.className = "file-meta";
      detail.textContent = `${file.kind} · ${file.bytes.toLocaleString("ko-KR")} bytes · SHA-256 ${file.sha256}`;
      const link = document.createElement("a");
      link.className = "button";
      link.href = file.url;
      link.download = "";
      link.textContent = `${file.name} 다운로드`;
      entry.append(name, detail, link);
      files.append(entry);
    }

    const build = document.querySelector("#build-metadata");
    addRow(build, "타깃 입력 SHA-256", data.target.source_input_sha256);
    addRow(build, "타깃 globals", `${data.target.globals_bytes.toLocaleString("ko-KR")} bytes`);
    addRow(build, "타깃 pool", `${data.target.pool_bytes.toLocaleString("ko-KR")} bytes`);
    addRow(build, "타깃 app", `${data.target.app_bytes.toLocaleString("ko-KR")} bytes`);
    addRow(build, "WASM 입력 SHA-256", data.wasm.source_input_sha256);
    addRow(build, "WASM SHA-256", data.wasm.sha256);
    addRow(build, "WASM 크기", `${data.wasm.bytes.toLocaleString("ko-KR")} bytes`);

    const limitations = document.querySelector("#limitations");
    for (const limitation of data.limitations) {
      const item = document.createElement("li");
      item.textContent = limitation;
      limitations.append(item);
    }
    document.querySelector("#manifest-section").hidden = false;
  }

  async function loadManifest() {
    try {
      let response;
      try {
        response = await fetch("./FIRMWARE.json", { cache: "no-store" });
      } catch {
        throw new Error("NETWORK");
      }
      if (!response.ok) throw new Error(`HTTP ${response.status}`);
      let data;
      try {
        data = await response.json();
      } catch {
        throw new Error("JSON");
      }
      if (!validManifest(data)) throw new Error("SCHEMA");
      render(data);
    } catch (error) {
      const reason = error.message;
      const panel = document.querySelector("#fetch-error");
      panel.dataset.cause = reason;
      const detail = reason.startsWith("HTTP ")
        ? `서버 응답 오류(${reason})입니다. 잠시 후 페이지를 새로고침하세요.`
        : reason === "NETWORK" ? "서버에 연결하지 못했습니다. 네트워크 연결을 확인하고 다시 시도하세요."
        : reason === "JSON" ? "서버의 패키지 정보를 해석하지 못했습니다. 소스 저장소에서 배포 상태를 확인하세요."
        : "패키지 정보 형식이 올바르지 않습니다. 소스 저장소에서 배포 상태를 확인하세요.";
      panel.textContent = `펌웨어 매니페스트를 불러오지 못했습니다. ${detail}`;
      panel.hidden = false;
    } finally {
      document.querySelector("#manifest-loading").hidden = true;
      document.querySelector("#manifest-slot").setAttribute("aria-busy", "false");
    }
  }

  loadManifest();
})();
