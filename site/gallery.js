(() => {
  const gallery = document.querySelector("#gallery");
  const status = document.querySelector("#gallery-status");
  const error = document.querySelector("#gallery-error");
  if (!gallery || !status || !error) return;

  const groups = [
    ["song", "힙합·테크노 댄스송"],
    ["preset", "32개 프리셋"],
    ["kit", "드럼 킷 8개"],
    ["demo", "모던 데모"],
    ["stress", "스트레스 렌더"]
  ];
  const label = (value) => Array.isArray(value) ? value.join(", ") : value == null ? "—" : String(value);
  const sourceLabel = (mask) => ["PCM", "FM", "VA"].filter((_, bit) => mask & (1 << bit)).join(" + ") || "없음";
  const genres = (tags) => String(tags).split(" ").map(tag => ({T:"테크노",H:"힙합",P:"팝",E:"유로신스"})[tag] || tag).join(" · ");

  function makeTrack(item, title) {
    const row = document.createElement("article");
    row.className = "audio-block";
    const heading = document.createElement("h3");
    heading.textContent = title;
    row.append(heading);

    const meta = document.createElement("p");
    meta.className = "small";
    const details = [
      item.role && `역할 ${label(item.role)}`,
      item.genre_tags && `장르 ${genres(item.genre_tags)}`,
      item.bpm != null && `${item.bpm} BPM`,
      item.song_bars != null && `${item.song_bars}마디 SONG`,
      item.swing != null && `스윙 ${item.swing}`,
      item.source_flags != null && `소스 ${sourceLabel(item.source_flags)}`,
      item.available_source_flags != null && `사용 가능 소스 ${sourceLabel(item.available_source_flags)}`,
      item.seconds != null && `${item.seconds}초`,
      item.peak_pcm16 != null && `peak ${item.peak_pcm16}`,
      item.rms_pcm16 != null && `RMS ${Number(item.rms_pcm16).toFixed(1)}`,
      item.quality_status && (item.quality_status === "UNAPPROVED_CANDIDATE" ? "청취 미승인" : "음질 검토 필요"),
      item.ended_tails != null && (item.ended_tails ? "꼬리 종료 정상" : "꼬리 미종료")
    ].filter(Boolean);
    meta.textContent = details.join(" · ");
    row.append(meta);

    if (typeof item.file !== "string" || !/^[\w.-]+\.wav$/i.test(item.file) || item.file === "." || item.file === "..") {
      const invalid = document.createElement("p");
      invalid.className = "small";
      invalid.textContent = "유효하지 않은 오디오 파일 항목입니다.";
      row.append(invalid);
      return row;
    }
    const url = `./audio/${encodeURIComponent(item.file)}`;
    const audio = document.createElement("audio");
    audio.controls = true;
    audio.preload = "none";
    audio.src = url;
    audio.setAttribute("aria-label", `${title} 오디오`);
    row.append(audio);
    const download = document.createElement("a");
    download.className = "text-link";
    download.href = url;
    download.download = item.file;
    download.textContent = "WAV 다운로드";
    row.append(download);
    return row;
  }

  function addGroup(items, kind, title, sortById) {
    const section = document.createElement("section");
    section.className = "gallery-group";
    if (kind === "song") section.id = "beat-songs";
    const heading = document.createElement("h3");
    heading.textContent = title;
    section.append(heading);
    const list = document.createElement("div");
    list.className = "audio-list";
    const selected = items.filter((item) => item.kind === kind);
    if (sortById) selected.sort((a, b) => Number(kind === "kit" ? a.drum_kit : a.preset_id) - Number(kind === "kit" ? b.drum_kit : b.preset_id));
    if (!selected.length) {
      const note = document.createElement("p");
      note.className = "small";
      note.textContent = "이 항목은 매니페스트에 없습니다.";
      list.append(note);
    }
    selected.forEach((item) => {
      const id = kind === "kit" ? item.drum_kit : item.preset_id;
      const name = item.name == null ? "" : String(item.name);
      const prefix = kind === "preset" ? `ID ${String(id).padStart(2, "0")}` : kind === "kit" ? `킷 ${String(id).padStart(2, "0")}` : "";
      list.append(makeTrack(item, [prefix, name].filter(Boolean).join(" · ") || title));
    });
    section.append(list);
    gallery.append(section);
  }

  function addSourceComparisons(items) {
    const sources = items.filter((item) => item.kind === "source");
    if (!sources.length) return;
    const details = document.createElement("details");
    const summary = document.createElement("summary");
    summary.textContent = `소스별 비교 오디오 (${sources.length})`;
    details.append(summary);
    const list = document.createElement("div");
    list.className = "audio-list";
    sources.forEach((item) => {
      const id = item.preset_id == null ? "—" : `ID ${String(item.preset_id).padStart(2, "0")}`;
      const flags = item.source_flags == null ? "" : ` · 소스 ${sourceLabel(item.source_flags)}`;
      list.append(makeTrack(item, `${id} · ${item.name == null ? "소스 비교" : String(item.name)}${flags}`));
    });
    details.append(list);
    gallery.append(details);
  }

  fetch("./auditions.json")
    .then((response) => {
      if (!response.ok) throw new Error(`HTTP ${response.status}`);
      return response.json();
    })
    .then((manifest) => {
      if (!manifest || !Array.isArray(manifest.files)) throw new Error("매니페스트에 files 배열이 없습니다.");
      gallery.replaceChildren();
      for (const [kind, title] of groups) addGroup(manifest.files, kind, title, kind === "preset" || kind === "kit");
      addSourceComparisons(manifest.files);
      status.textContent = `매니페스트에서 ${manifest.files.length}개 오디오 항목을 읽었습니다.`;
    })
    .catch((cause) => {
      status.textContent = "오디션 목록을 불러오지 못했습니다.";
      error.hidden = false;
      error.textContent = `오디오 매니페스트 오류: ${cause instanceof Error ? cause.message : String(cause)}. 파일이 준비되면 페이지를 새로고침하세요.`;
    });

  document.addEventListener("play", (event) => {
    if (!(event.target instanceof HTMLAudioElement)) return;
    document.querySelectorAll("audio").forEach((audio) => {
      if (audio !== event.target) audio.pause();
    });
  }, true);
})();
