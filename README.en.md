# Piano Universe

An independent Android instrument for MIDI keyboards, SF2 soundfonts and compatible SFZ libraries. Formerly Piano Room.

Four instrument channels, saved scenes, pedal settings, effects, recording, a 3D piano view and exportable benchmarks. AudioTrack, AAudio and optional Oboe backends are retained. SFZ support is partial; unsupported features are reported by the importer.

## Build
Android Studio / JDK 17, SDK 35, NDK 27.2.12479018, CMake 3.22.1, AGP 8.8.2 and Gradle 8.10.2.
Restore the matching release asset archive with `python scripts/restore_assets.py /path/Piano-Universe-Bancos-Incluidos.zip`, then open the project in Android Studio and build a debug APK. Signing keys are not distributed. Third-party builds cannot update the official installation without its private signing key.

Original code is MIT. Dependencies and bundled sounds retain their individual licences; see THIRD_PARTY_NOTICES.md and assets/legal. The full modified jFLAC source is included in src/org/jflac.

Suggestions, device testing, translations and documentation are welcome. Read CONTRIBUTING.md before proposing changes. Donations are optional. The official repository is https://github.com/biblia1924198/piano-universe. Voluntary support via Pix is available on the official site. Always verify the recipient in your banking app.
