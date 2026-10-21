(() => {
  const gallery = document.querySelector("#gallery");
  const status = document.querySelector("#gallery-status");
  const error = document.querySelector("#gallery-error");
  if (!gallery || !status || !error) return;

  const groups = [
    ["demo", "MC-1 demos · Original arrangements, new voices"],
    ["song", "Additional hip-hop & techno dance songs"],
    ["preset", "full-tone presets"],
    ["kit", "drum kits"],
    ["stress", "Stress render"]
  ];
  const label = (value) => Array.isArray(value) ? value.join(", ") : value == null ? "—" : String(value);
  const sourceLabel = (item, field) => {
    const names = item.source_names
      ? item.source_names.filter((_, slot) => item[field] & (1 << slot))
      : item.patches.flatMap(patch => patch.source_names.filter((_, slot) =>
        patch[field === "source_flags" ? "mask" : "available_source_flags"] & (1 << slot)));
    return [...new Set(names)].join(" + ") || "None";
  };
  const genres = (tags) => String(tags).split(" ").map(tag => ({T:"Techno",H:"Hip-hop",P:"Pop",E:"Eurosynth"})[tag] || tag).join(" · ");

  function makeTrack(item, title) {
    const row = document.createElement("article");
    row.className = "audio-block";
    const heading = document.createElement("h3");
    heading.textContent = title;
    row.append(heading);

    const meta = document.createElement("p");
    meta.className = "small";
    const details = [
      item.role && `Role ${label(item.role)}`,
      item.genre_tags && `Genre ${genres(item.genre_tags)}`,
      item.bpm != null && `${item.bpm} BPM`,
      item.song_bars != null && `${item.song_bars}-bar SONG`,
      item.pattern_count != null && `${item.pattern_count} patterns`,
      item.automation_count != null && `${item.automation_count} native automations`,
      item.swing != null && `Swing ${item.swing}`,
      item.source_flags != null && `Sources ${item.source_configuration || sourceLabel(item, "source_flags")}`,
      item.available_source_flags != null && `Available sources ${sourceLabel(item, "available_source_flags")}`,
      item.seconds != null && `${item.seconds} sec`,
      item.diagnostic && (item.render_mode === "dry-diagnostic" ? "Dry diagnostic" : "Source-only diagnostic"),
      item.peak_pcm16 != null && `Local WAV peak ${item.peak_pcm16}`,
      item.rms_pcm16 != null && `Local WAV RMS ${Number(item.rms_pcm16).toFixed(1)}`,
      item.quality_status && (item.quality_status === "UNAPPROVED_CANDIDATE" ? "Listening not approved" : "Quality review required"),
      item.ended_tails != null && (item.ended_tails ? "Tail ended cleanly" : "Tail still active")
    ].filter(Boolean);
    meta.textContent = details.join(" · ");
    row.append(meta);

    if (typeof item.file !== "string" || !/^[\w.-]+\.mp3$/i.test(item.file) || item.file === "." || item.file === "..") {
      const invalid = document.createElement("p");
      invalid.className = "small";
      invalid.textContent = "This audio file entry is invalid.";
      row.append(invalid);
      return row;
    }
    const url = `./audio/${encodeURIComponent(item.file)}?v=${encodeURIComponent(item.sha256)}`;
    const audio = document.createElement("audio");
    audio.controls = true;
    audio.preload = "none";
    audio.src = url;
    audio.setAttribute("aria-label", `${title} audio`);
    row.append(audio);
    const download = document.createElement("a");
    download.className = "text-link";
    download.href = url;
    download.download = item.file;
    download.textContent = "Download MP3";
    row.append(download);
    return row;
  }

  function addGroup(items, kind, title, sortById) {
    const section = document.createElement("section");
    section.className = "gallery-group";
    if (kind === "demo") section.id = "mc1-demos";
    if (kind === "song") section.id = "beat-songs";
    const heading = document.createElement("h3");
    section.append(heading);
    const list = document.createElement("div");
    list.className = "audio-list";
    const selected = items.filter((item) => item.kind === kind);
    heading.textContent = kind === "preset" || kind === "kit" ? `${selected.length} ${title}` : title;
    if (sortById) selected.sort((a, b) => Number(kind === "kit" ? a.drum_kit : a.preset_id) - Number(kind === "kit" ? b.drum_kit : b.preset_id));
    if (!selected.length) {
      const note = document.createElement("p");
      note.className = "small";
      note.textContent = "This section is not present in the manifest.";
      list.append(note);
    }
    selected.forEach((item) => {
      const id = kind === "kit" ? item.drum_kit : item.preset_id;
      const name = item.name == null ? "" : String(item.name);
      const prefix = kind === "preset" ? `ID ${String(id).padStart(2, "0")}` : kind === "kit" ? `Kit ${String(id).padStart(2, "0")}` : "";
      list.append(makeTrack(item, [prefix, name].filter(Boolean).join(" · ") || title));
    });
    section.append(list);
    gallery.append(section);
  }

  function addDiagnostics(items) {
    const sources = items.filter((item) => item.diagnostic);
    if (!sources.length) return;
    const details = document.createElement("details");
    const summary = document.createElement("summary");
    const dryCount = sources.filter((item) => item.render_mode === "dry-diagnostic").length;
    const sourceCount = sources.filter((item) => item.render_mode === "source-diagnostic").length;
    summary.textContent = `Diagnostic audio (${dryCount} dry · ${sourceCount} source-only)`;
    details.append(summary);
    const list = document.createElement("div");
    list.className = "audio-list";
    sources.forEach((item) => {
      const id = item.preset_id == null ? "—" : `ID ${String(item.preset_id).padStart(2, "0")}`;
      const flags = item.source_flags == null ? "" : ` · Sources ${item.source_configuration || sourceLabel(item, "source_flags")}`;
      list.append(makeTrack(item, `${id} · ${item.name == null ? "Source comparison" : String(item.name)}${flags}`));
    });
    details.append(list);
    gallery.append(details);
  }

  const assetQuery = new URL(document.currentScript.src).search;
  fetch(`./auditions.json${assetQuery}`, { cache: "no-store" })
    .then((response) => {
      if (!response.ok) throw new Error(`HTTP ${response.status}`);
      return response.json();
    })
    .then((manifest) => {
      if (!manifest || !Array.isArray(manifest.files)) throw new Error("The manifest has no files array.");
      if (manifest.schema_version !== 4) throw new Error("Unsupported audio manifest schema.");
      const primary = manifest.files.filter((item) => !item.diagnostic);
      const presets = primary.filter((item) => item.kind === "preset" && item.render_mode === "full");
      if (manifest.profile_count !== 64 || manifest.source_mask_semantics !== "slot-bits"
        || presets.length !== 64 || new Set(presets.map((item) => item.preset_id)).size !== 64
        || presets.some((item) => !Number.isInteger(item.preset_id) || item.preset_id < 0 || item.preset_id >= 64)) {
        throw new Error("The manifest must contain exactly 64 full-tone presets with slot metadata.");
      }
      gallery.replaceChildren();
      for (const [kind, title] of groups) addGroup(kind === "preset" ? presets : primary, kind, title, kind === "preset" || kind === "kit");
      addDiagnostics(manifest.files);
      status.textContent = `Loaded ${manifest.files.length} audio entries from the manifest.`;
    })
    .catch((cause) => {
      status.textContent = "The audio gallery could not be loaded.";
      error.hidden = false;
      error.textContent = `Audio manifest error: ${cause instanceof Error ? cause.message : String(cause)}. Reload the page once the files are available.`;
    });

  document.addEventListener("play", (event) => {
    if (!(event.target instanceof HTMLAudioElement)) return;
    document.querySelectorAll("audio").forEach((audio) => {
      if (audio !== event.target) audio.pause();
    });
  }, true);
})();
