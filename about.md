# GDRBot

Bot replay untuk **Geometry Dash 2.2081** (Geode SDK **5.11.2**) dengan format
**GDReplayFormat `.gdr2`**. Fungsi mirip xdBot: record, play, dan practice fix.

## Fitur

- Record input P1/P2 + platformer, seed, coins, LDM.
- Playback presisi per frame dengan auto-restart & auto-stop.
- Simpan/muat `.gdr2` (msgpack GDR) dan `.gdr.json`/`.gdr`.
- Practice fix: state player dipulihkan penuh saat checkpoint dimuat, input di
  sekitar checkpoint dirapikan, jadi macro tidak desync setelah mati.
- Menu in-game di pause menu (Android & desktop).
