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
    document.title = `Firmware downloads ${data.version} experimental — mc-1-mod`;
    const metadata = document.querySelector("#metadata");
    addRow(metadata, "Product", data.product);
    addRow(metadata, "Version", data.version);
    addRow(metadata, "Status", "experimental · Hardware unverified · Listening not approved");

    const files = document.querySelector("#files");
    for (const file of data.files) {
      const entry = document.createElement("article");
      entry.className = "file";
      const name = document.createElement("div");
      name.className = "file-name";
      name.textContent = file.name;
      const detail = document.createElement("div");
      detail.className = "file-meta";
      detail.textContent = `${file.kind} · ${file.bytes.toLocaleString("en-US")} bytes · SHA-256 ${file.sha256}`;
      const link = document.createElement("a");
      link.className = "button";
      link.href = `${file.url}?v=${file.sha256}`;
      link.download = "";
      link.textContent = `Download ${file.name}`;
      entry.append(name, detail, link);
      files.append(entry);
    }

    const build = document.querySelector("#build-metadata");
    addRow(build, "Target input SHA-256", data.target.source_input_sha256);
    addRow(build, "Target globals", `${data.target.globals_bytes.toLocaleString("en-US")} bytes`);
    addRow(build, "Target pool", `${data.target.pool_bytes.toLocaleString("en-US")} bytes`);
    addRow(build, "Target app", `${data.target.app_bytes.toLocaleString("en-US")} bytes`);
    addRow(build, "WASM input SHA-256", data.wasm.source_input_sha256);
    addRow(build, "WASM SHA-256", data.wasm.sha256);
    addRow(build, "WASM size", `${data.wasm.bytes.toLocaleString("en-US")} bytes`);

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
        ? `Server response error (${reason}). Reload the page in a moment.`
        : reason === "NETWORK" ? "The server could not be reached. Check your network connection and try again."
        : reason === "JSON" ? "The package information could not be parsed. Check the source repository for deployment status."
        : "The package information has an invalid format. Check the source repository for deployment status.";
      panel.textContent = `The firmware manifest could not be loaded. ${detail}`;
      panel.hidden = false;
    } finally {
      document.querySelector("#manifest-loading").hidden = true;
      document.querySelector("#manifest-slot").setAttribute("aria-busy", "false");
    }
  }

  loadManifest();
})();
