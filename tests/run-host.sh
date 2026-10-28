#!/usr/bin/env bash
set -euo pipefail
export JAVA_TOOL_OPTIONS="${JAVA_TOOL_OPTIONS:--Xmx384m}"
# Run in the project directory. JDK with javac, Android android.jar and C compiler.
: "${ANDROID_JAR:?Set ANDROID_JAR to platforms/android-35/android.jar}"
PR_TEST_BUILD="${PR_TEST_BUILD:-/tmp/piano-room-host-tests}"
mkdir -p "$PR_TEST_BUILD/classes" "$PR_TEST_BUILD/adapters" "$PR_TEST_BUILD/native" "$PR_TEST_BUILD/fake"
: "${AAPT2:?Set AAPT2 to Android build-tools aapt2}"
mkdir -p "$PR_TEST_BUILD/generated"
"$AAPT2" compile --dir res -o "$PR_TEST_BUILD/resources.zip"
"$AAPT2" link -o "$PR_TEST_BUILD/resources.apk" --manifest AndroidManifest.xml -I "$ANDROID_JAR" --java "$PR_TEST_BUILD/generated" "$PR_TEST_BUILD/resources.zip"
javac -source 8 -target 8 -cp "$ANDROID_JAR" -d "$PR_TEST_BUILD/classes" "$PR_TEST_BUILD/generated/com/luci/pianoroom/R.java" src/com/luci/pianoroom/*.java $(rg --files src/org/jflac -g '*.java')
javac -source 8 -target 8 -cp "$PR_TEST_BUILD/classes:$ANDROID_JAR" -d "$PR_TEST_BUILD/adapters" tests/androidstub/android/media/*.java tests/androidstub/android/os/*.java tests/*.java
: "${JAVA_HOME:?Set JAVA_HOME to the JDK directory}"
cc -shared -fPIC -O2 -fno-strict-aliasing -I"$JAVA_HOME/include" -I"$JAVA_HOME/include/linux" native/bridge.c -lm -ldl -o "$PR_TEST_BUILD/native/libpianoroom.so"
cc -shared -fPIC -O2 tests/FakeAAudio.c -pthread -lm -o "$PR_TEST_BUILD/fake/libaaudio.so"
PR_TEST_CP="$PR_TEST_BUILD/adapters:$PR_TEST_BUILD/classes"
timeout -s KILL 45s java -Djava.library.path="$PR_TEST_BUILD/native" -cp "$PR_TEST_CP" AudioReadinessTest assets/GeneralUser-GS.sf2
LD_LIBRARY_PATH="$PR_TEST_BUILD/fake" PR_FAKE_OPEN_DELAY_MS=350 timeout -s KILL 45s java -Djava.library.path="$PR_TEST_BUILD/native" -cp "$PR_TEST_CP" AudioReadinessTest assets/GeneralUser-GS.sf2 aaudio
for test in MidiParserTest ProtocolAndBufferTest SessionPolicyTest CaptureLifecycleTest RecordingStorageTest; do timeout -s KILL 45s java -cp "$PR_TEST_CP" "$test"; done
timeout -s KILL 45s java -Djava.library.path="$PR_TEST_BUILD/native" -cp "$PR_TEST_CP" SynthLoopTest assets/GeneralUser-GS.sf2
timeout -s KILL 45s java -Djava.library.path="$PR_TEST_BUILD/native" -cp "$PR_TEST_CP" StudioTest assets/GeneralUser-GS.sf2
LD_LIBRARY_PATH="$PR_TEST_BUILD/fake" timeout -s KILL 45s java -Djava.library.path="$PR_TEST_BUILD/native" -cp "$PR_TEST_CP" StudioTest assets/GeneralUser-GS.sf2 aaudio
for scenario in exclusive shared reject disconnect; do
 unset PR_FAKE_SHARED PR_FAKE_REJECT_ALL PR_FAKE_DISCONNECT
 case "$scenario" in shared) export PR_FAKE_SHARED=1;; reject) export PR_FAKE_REJECT_ALL=1;; disconnect) export PR_FAKE_DISCONNECT=1;; esac
 LD_LIBRARY_PATH="$PR_TEST_BUILD/fake" timeout 20s timeout -s KILL 45s java -Djava.library.path="$PR_TEST_BUILD/native" -cp "$PR_TEST_CP" NativeBackendTest assets/GeneralUser-GS.sf2 "$scenario"
done
unset PR_FAKE_SHARED PR_FAKE_REJECT_ALL PR_FAKE_DISCONNECT
cc -O2 tests/NativeCoreTest.c -lm -ldl -pthread -o "$PR_TEST_BUILD/native_core"
LD_LIBRARY_PATH="$PR_TEST_BUILD/fake" "$PR_TEST_BUILD/native_core" assets/GeneralUser-GS.sf2
cc -O2 tests/AudioProtectionTest.c -lm -o "$PR_TEST_BUILD/protection"
"$PR_TEST_BUILD/protection"
cc -O2 tests/NativeContinuityTest.c -lm -ldl -pthread -o "$PR_TEST_BUILD/continuity"
"$PR_TEST_BUILD/continuity" assets/GeneralUser-GS.sf2
cc -O2 -fno-strict-aliasing tests/audio_test.c -lm -o "$PR_TEST_BUILD/banks"
"$PR_TEST_BUILD/banks" assets/GeneralUser-GS.sf2 "$PR_TEST_BUILD/presets.tsv" "$PR_TEST_BUILD/demo.wav"

cc -O2 tests/SmartVoicesTest.c -lm -o "$PR_TEST_BUILD/smart"
for font in assets/GeneralUser-GS.sf2 assets/FluidR3-GM.sf2; do "$PR_TEST_BUILD/smart" "$font"; done
for font in assets/FluidR3-GM.sf2 assets/YDP-GrandPiano.sf2; do "$PR_TEST_BUILD/banks" "$font" "$PR_TEST_BUILD/$(basename "$font").tsv" "$PR_TEST_BUILD/$(basename "$font").wav"; done

cc -O2 tests/SoundQualityTest.c -lm -o "$PR_TEST_BUILD/quality"
for font in assets/GeneralUser-GS.sf2 assets/FluidR3-GM.sf2 assets/YDP-GrandPiano.sf2; do "$PR_TEST_BUILD/quality" "$font"; done
timeout -s KILL 45s java -Djava.library.path="$PR_TEST_BUILD/native" -cp "$PR_TEST_CP" SoundQualityTest assets/GeneralUser-GS.sf2
LD_LIBRARY_PATH="$PR_TEST_BUILD/fake" timeout -s KILL 45s java -Djava.library.path="$PR_TEST_BUILD/native" -cp "$PR_TEST_CP" SoundQualityTest assets/GeneralUser-GS.sf2 aaudio
for font in assets/GeneralUser-GS.sf2 assets/FluidR3-GM.sf2 assets/YDP-GrandPiano.sf2; do "$PR_TEST_BUILD/banks" "$font" "$PR_TEST_BUILD/$(basename "$font")-hq.tsv" "$PR_TEST_BUILD/$(basename "$font")-hq.wav" hq; done

# Piano Room0.8: scene graph and selected performance features.
timeout -s KILL 45s java -cp "$PR_TEST_CP" com.luci.pianoroom.YamahaMixerTest
timeout -s KILL 45s java -cp "$PR_TEST_CP" com.luci.pianoroom.PerformanceFeaturesTest
timeout -s KILL 45s java -cp "$PR_TEST_CP" com.luci.pianoroom.SfzBankTest "$PR_TEST_BUILD/sfz-fixture"
timeout -s KILL 45s java -cp "$PR_TEST_CP" com.luci.pianoroom.SfzCompatibilityTest "$PR_TEST_BUILD/sfz-compatibility"
timeout -s KILL 45s java -Djava.library.path="$PR_TEST_BUILD/native" -cp "$PR_TEST_CP" SceneIntegrationTest assets/GeneralUser-GS.sf2
LD_LIBRARY_PATH="$PR_TEST_BUILD/fake" timeout -s KILL 45s java -Djava.library.path="$PR_TEST_BUILD/native" -cp "$PR_TEST_CP" SceneIntegrationTest assets/GeneralUser-GS.sf2 aaudio
cc -O2 tests/SceneContinuityTest.c -lm -o "$PR_TEST_BUILD/scenes"
"$PR_TEST_BUILD/scenes" assets/GeneralUser-GS.sf2
cc -O2 tests/PackedBankTest.c -lm -o "$PR_TEST_BUILD/packed"
"$PR_TEST_BUILD/packed" "$PR_TEST_BUILD/sfz-fixture/fixture.prb"
cc -O2 tests/RetrospectiveTest.c -lm -pthread -o "$PR_TEST_BUILD/retro"
"$PR_TEST_BUILD/retro"

# 0.11: real metadata parity and native sympathetic resonance / pedal lifecycle.
timeout -s KILL 45s java -Xmx384m -Djava.library.path="$PR_TEST_BUILD/native" -cp "$PR_TEST_CP" com.luci.pianoroom.InstrumentIndexTest assets/GeneralUser-GS.sf2 assets/FluidR3-GM.sf2 assets/YDP-GrandPiano.sf2 "$PR_TEST_BUILD/sfz-fixture/fixture.prb"
cc -O2 tests/PianoResonanceTest.c -lm -o "$PR_TEST_BUILD/resonance"
"$PR_TEST_BUILD/resonance"
cc -O2 tests/PedalResonanceIntegrationTest.c -lm -o "$PR_TEST_BUILD/pedal_resonance"
"$PR_TEST_BUILD/pedal_resonance" assets/GeneralUser-GS.sf2

# 0.12: duration-aware releases, velocity blends, bank estimates, mechanical DSP.
timeout -s KILL 45s java -Djava.library.path="$PR_TEST_BUILD/native" -cp "$PR_TEST_CP" com.luci.pianoroom.PianoRealismTest "$PR_TEST_BUILD/piano-realism"
cc -O2 tests/PianoRealismTest.c -lm -o "$PR_TEST_BUILD/piano_realism"
"$PR_TEST_BUILD/piano_realism" "$PR_TEST_BUILD/piano-realism/realism.prb"

# 0.13: reduced body/soundboard, virtual lid, retained tails and 3D transforms.
timeout -s KILL 45s java -cp "$PR_TEST_CP" com.luci.pianoroom.PianoVisualTest assets/piano3d.pr3d "$PR_TEST_BUILD/render-states.txt"
cc -O2 tests/PianoBodyTest.c -lm -o "$PR_TEST_BUILD/body"
"$PR_TEST_BUILD/body"
cc -O2 tests/PianoBodyIntegrationTest.c -lm -o "$PR_TEST_BUILD/body_integration"
"$PR_TEST_BUILD/body_integration" assets/GeneralUser-GS.sf2
timeout -s KILL 45s java -Djava.library.path="$PR_TEST_BUILD/native" -cp "$PR_TEST_CP" PianoBodyJniTest assets/GeneralUser-GS.sf2
LD_LIBRARY_PATH="$PR_TEST_BUILD/fake" timeout -s KILL 45s java -Djava.library.path="$PR_TEST_BUILD/native" -cp "$PR_TEST_CP" PianoBodyJniTest assets/GeneralUser-GS.sf2 aaudio

# 0.14: hybrid measured IR, pitch texture, spectral profile and ready recipes.
timeout -s KILL 45s java -cp "$PR_TEST_CP" com.luci.pianoroom.SoundPresetsTest
cc -O2 tests/HybridSpaceTest.c -lm -o "$PR_TEST_BUILD/hybrid_space"
"$PR_TEST_BUILD/hybrid_space"
cc -O2 tests/HarmonicRepetitionTest.c -lm -o "$PR_TEST_BUILD/harmonic_repetition"
"$PR_TEST_BUILD/harmonic_repetition"
timeout -s KILL 45s java -Djava.library.path="$PR_TEST_BUILD/native" -cp "$PR_TEST_CP" SoundSpaceJniTest assets/GeneralUser-GS.sf2
LD_LIBRARY_PATH="$PR_TEST_BUILD/fake" timeout -s KILL 45s java -Djava.library.path="$PR_TEST_BUILD/native" -cp "$PR_TEST_CP" SoundSpaceJniTest assets/GeneralUser-GS.sf2 aaudio

timeout -s KILL 45s java -cp "$PR_TEST_CP" com.luci.pianoroom.BenchmarkTest
cc -O2 tests/VoiceDeClickTest.c -lm -o "$PR_TEST_BUILD/declick"
"$PR_TEST_BUILD/declick" assets/GeneralUser-GS.sf2
cc -O2 tests/SoundSpaceIntegrationTest.c -lm -o "$PR_TEST_BUILD/space_integration"
"$PR_TEST_BUILD/space_integration" assets/GeneralUser-GS.sf2

LD_LIBRARY_PATH="$PR_TEST_BUILD/fake" timeout -s KILL 45s java -Djava.library.path="$PR_TEST_BUILD/native" -cp "$PR_TEST_CP" BenchmarkSynthTest assets/GeneralUser-GS.sf2

# 0.15 live Yamaha USB and large-file sample maps.
timeout -s KILL 45s java -cp "$PR_TEST_CP" com.luci.pianoroom.YamahaAudioSettingsTest
cc -O2 tests/LargePackedBankTest.c -lm -o "$PR_TEST_BUILD/large_packed"
"$PR_TEST_BUILD/large_packed" "$PR_TEST_BUILD/large-3GiB.prb"
cc -O2 tests/YamahaCaptureTest.c -lm -ldl -pthread -o "$PR_TEST_BUILD/yamaha_capture"
PR_FAKE_ALLOW_SILENT=1 PR_FAKE_INPUT_SHORT=1 LD_LIBRARY_PATH="$PR_TEST_BUILD/fake" "$PR_TEST_BUILD/yamaha_capture"

if [ -n "${PR_ZIP64_FIXTURE:-}" ]; then timeout -s KILL 45s java -Xmx128m -Djava.library.path="$PR_TEST_BUILD/native" -cp "$PR_TEST_CP" com.luci.pianoroom.LargeSfzImportTest "$PR_TEST_BUILD/large-import" "$PR_ZIP64_FIXTURE"; fi

# 0.16: exact IR scheduling/SIMD parity and lock-free read-only JNI meters.
cc -O2 -fno-strict-aliasing tests/PartitionScheduleTest.c -lm -o "$PR_TEST_BUILD/partition_schedule"
"$PR_TEST_BUILD/partition_schedule"
cc -O2 -fno-strict-aliasing -I"$JAVA_HOME/include" -I"$JAVA_HOME/include/linux" tests/NativeMeterTest.c -lm -ldl -pthread -o "$PR_TEST_BUILD/native_meters"
"$PR_TEST_BUILD/native_meters"

# 0.17: validated recorded mechanics, sustain lifecycle and three-layer stress.
cc -O2 tests/RecordedMechanicsTest.c -lm -o "$PR_TEST_BUILD/recorded_mechanics"
"$PR_TEST_BUILD/recorded_mechanics" assets/mechanics-v17.prm
cc -O2 -fno-strict-aliasing tests/RecordedMechanicsIntegrationTest.c -lm -o "$PR_TEST_BUILD/recorded_integration"
"$PR_TEST_BUILD/recorded_integration" assets/GeneralUser-GS.sf2 assets/mechanics-v17.prm
timeout -s KILL 45s java -Djava.library.path="$PR_TEST_BUILD/native" -cp "$PR_TEST_CP" RecordedMechanicsJniTest assets/mechanics-v17.prm assets/GeneralUser-GS.sf2
LD_LIBRARY_PATH="$PR_TEST_BUILD/fake" timeout -s KILL 45s java -Djava.library.path="$PR_TEST_BUILD/native" -cp "$PR_TEST_CP" RecordedMechanicsJniTest assets/mechanics-v17.prm assets/GeneralUser-GS.sf2 aaudio

# 0.17 distribution fix: lossless original-bank recovery and interrupted copies.
if [ ! -f assets/GeneralUser-GS.sf2.prsf ] || [ ! -f assets/FluidR3-GM.sf2.prsf ] || [ ! -f assets/YDP-GrandPiano.sf2.prsf ]; then python3 build-support/prepare-bundled-banks.py; fi
timeout -s KILL 90s java -Xmx128m -cp "$PR_TEST_CP" com.luci.pianoroom.BundledBanksTest assets "$PR_TEST_BUILD/bundled-bank-cache"

# 0.18: SFZ controller defaults, multi-instrument imports and inner-rim containment.
timeout -s KILL 45s java -cp "$PR_TEST_CP" com.luci.pianoroom.SfzDefaultsBatchTest "$PR_TEST_BUILD/sfz-defaults"
cc -O2 tests/SfzDefaultsTest.c -lm -o "$PR_TEST_BUILD/sfz_defaults"
"$PR_TEST_BUILD/sfz_defaults" "$PR_TEST_BUILD/sfz-defaults/defaults.prb"
python3 tests/piano_containment_test.py

"$PR_TEST_BUILD/large_packed" "$PR_TEST_BUILD/large-PRB4-3GiB.prb" prb4

# 0.19: authored pedal, explicit controller amplitude, note masking and ZIP regression.
timeout -s KILL 45s java -cp "$PR_TEST_CP" com.luci.pianoroom.SfzPerformanceTest "$PR_TEST_BUILD/sfz-performance"
cc -O2 tests/SfzPerformanceTest.c -lm -o "$PR_TEST_BUILD/sfz_performance"
"$PR_TEST_BUILD/sfz_performance" "$PR_TEST_BUILD/sfz-performance/performance.prb"
if [ -n "${PR_NEBLINA_ZIP:-}" ]; then
 timeout -s KILL 90s java -Xmx128m -Djava.library.path="$PR_TEST_BUILD/native" -cp "$PR_TEST_CP" com.luci.pianoroom.NeblinaImportTest "$PR_TEST_BUILD/neblina" "$PR_NEBLINA_ZIP"
 "$PR_TEST_BUILD/sfz_performance" "$PR_TEST_BUILD/neblina/neblina.prb"
fi

# 0.20: controller-dependent velocity, the reported oncc99 import failure.
timeout -s KILL 45s java -Xmx128m -Djava.library.path="$PR_TEST_BUILD/native" -cp "$PR_TEST_CP" com.luci.pianoroom.SfzVelocityControllerTest "$PR_TEST_BUILD/sfz-velocity"
cc -O2 tests/SfzVelocityControllerTest.c -lm -o "$PR_TEST_BUILD/sfz_velocity"
"$PR_TEST_BUILD/sfz_velocity" "$PR_TEST_BUILD/sfz-velocity/velocity.prb" "$PR_TEST_BUILD/sfz-velocity/endpoints.prb"

# 0.21: original reported SFZ controller curves and sampled mechanics.
timeout -s KILL 90s java -Xmx128m -Djava.library.path="$PR_TEST_BUILD/native" -cp "$PR_TEST_CP" com.luci.pianoroom.SfzAutomationTest "$PR_TEST_BUILD/sfz-automation" ${PR_PIANO_REFERENCIA_SFZ:+"$PR_PIANO_REFERENCIA_SFZ"}
cc -O2 tests/SfzAutomationTest.c -lm -o "$PR_TEST_BUILD/sfz_automation"
"$PR_TEST_BUILD/sfz_automation" "$PR_TEST_BUILD/sfz-automation/curves.prb" "$PR_TEST_BUILD/sfz-automation/pedal.prb" "$PR_TEST_BUILD/sfz-automation/signed.prb" "$PR_TEST_BUILD/sfz-automation/zero.prb" ${PR_PIANO_REFERENCIA_SFZ:+"$PR_TEST_BUILD/sfz-automation/reference.prb"}

# 0.21 library-specific pedal fade policy; held keys remain sounding.
timeout -s KILL 45s java -Djava.library.path="$PR_TEST_BUILD/native" -cp "$PR_TEST_CP" com.luci.pianoroom.LibraryPedalTest "$PR_TEST_BUILD/sfz-automation/curves.prb"
cc -O2 tests/LibraryPedalTest.c -lm -o "$PR_TEST_BUILD/library_pedal"
"$PR_TEST_BUILD/library_pedal" "$PR_TEST_BUILD/sfz-automation/curves.prb" "$PR_TEST_BUILD/sfz-automation/legacy-pedal.prb"

# 0.22: stable library edits/deletion, landscape dimensions and camera mode constraints.
timeout -s KILL 45s java -cp "$PR_TEST_CP" com.luci.pianoroom.LibraryManagementTest assets/GeneralUser-GS.sf2 "$PR_TEST_BUILD/sfz-fixture/fixture.prb"
timeout -s KILL 45s java -cp "$PR_TEST_CP" VideoModePolicyTest
for bank in assets/GeneralUser-GS.sf2 "$PR_TEST_BUILD/sfz-automation/curves.prb"; do timeout -s KILL 45s java -Djava.library.path="$PR_TEST_BUILD/native" -cp "$PR_TEST_CP" BankDeletionAudioTest "$bank"; done
