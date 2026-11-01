# GDRBot

Bot/replay tool untuk Geometry Dash 2.2081 (Geode SDK 5.11.2), dengan format
macro **GDReplayFormat (`.gdr2`)** — record, play, dan practice fix.

Mirip xdBot dari sisi fungsi (record / play / frame step / practice fix), tapi
macro-nya disimpan sebagai `.gdr2` (msgpack GDR) supaya bisa dipakai lintas bot.

## Fitur

- **Record** — rekam input P1/P2 (termasuk platformer kiri/kanan), seed, coins, LDM.
- **Play** — playback berbasis frame yang presisi, auto-restart & auto-stop.
- **Format `.gdr2`** — msgpack GDReplayFormat + ekstensi `gdrbot` (frame fix &
  info tambahan) tanpa merusak kompatibilitas bot lain.
- **Practice fix** — saat checkpoint di-load, state player dipulihkan (mode, gravity,
  velocity, dash, hold) dan input di sekitar frame checkpoint dibersihkan, sehingga
  macro tetap jalan setelah checkpoint.
- **Frame stepper / speedhack / auto-safe-mode** sederhana.
- **Tombol in-game** — menu bot di `PauseLayer` dan `LevelInfoLayer` (Android & desktop).

## Build

Tidak perlu build lokal. Push ke GitHub → Actions build otomatis.

- SDK Geode: **5.11.2** (diambil dari `mod.json` lewat `sdk: given`)
- GD: **2.2081**
- Target: Android64, Android32, Windows

Artefak `.geode` ada di tab **Actions → artifact**.

## Pemakaian

1. Install `.geode` lewat mod manager / letakkan di folder `geode/mods`.
2. Masuk level → pause → tombol **GDRBot**.
3. `Record` untuk mulai rekam, `Stop` untuk berhenti; lalu `Save` (nama file `.gdr2`).
4. `Load` untuk memilih file, `Play` untuk playback.
5. `Practice Fix` dinyalakan default; matikan kalau ingin perilaku vanilla.

## Format file

`.gdr2` = GDReplayFormat (msgpack). Field standar: `author`, `duration`,
`framerate`, `gameVersion`, `seed`, `coins`, `ldm`, `bot`, `level`, `inputs`.
Ekstensi `gdrbot` menyimpan `frameFixes` + versi bot; bot lain boleh mengabaikannya.

`.gdr`/`.gdr.json` (JSON polos) juga bisa dibaca.
