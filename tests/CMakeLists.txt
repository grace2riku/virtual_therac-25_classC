# GoogleTest 経由のテストツリー.
# Step 17 (CR-0005) で UNIT-200 CommonTypes の本格 UT を追加.
# 後続 Step 18+ で他 UNIT の UT を順次追加する.

find_package(GTest CONFIG REQUIRED)

include(GoogleTest)

# Smoke test: ビルドシステム + GoogleTest + Sanitizer の動作確認のみを目的とする
# (Step 14 で導入). UNIT 機能要求の検証は test_common_types.cpp 以降で実施.
add_executable(th25_smoke_tests
    unit/test_smoke.cpp
)
target_link_libraries(th25_smoke_tests
    PRIVATE
        th25_ctrl
        th25_sim
        GTest::gtest
        GTest::gtest_main
)
target_compile_options(th25_smoke_tests PRIVATE ${TH25_STRICT_WARNINGS})
gtest_discover_tests(th25_smoke_tests)

# UNIT-200 CommonTypes の本格 UT (Step 17 / CR-0005, UTPR-TH25-001 v0.1 §7.2).
# - 強い型 / enum class / Result<T,E> / PulseCounter (lock-free atomic)
# - HZ-002 (race condition) / HZ-003 (整数オーバフロー) / HZ-007 (レガシー前提) 直接対応
add_executable(th25_common_types_tests
    unit/test_common_types.cpp
)
target_link_libraries(th25_common_types_tests
    PRIVATE
        th25_ctrl
        GTest::gtest
        GTest::gtest_main
)
target_compile_options(th25_common_types_tests PRIVATE ${TH25_STRICT_WARNINGS})
gtest_discover_tests(th25_common_types_tests)

# UNIT-401 InProcessQueue (SPSC lock-free) の本格 UT (Step 18 / CR-0006,
# UTPR-TH25-001 v0.2 §7.2).
# - 容量境界 / FIFO / wrap-around / メッセージ型独立性
# - HZ-002 (race condition) 直接対応: 1 producer + 1 consumer で 100k メッセージ
#   concurrent 試験 (tsan プリセットで race detection 0 を検証)
add_executable(th25_in_process_queue_tests
    unit/test_in_process_queue.cpp
)
target_link_libraries(th25_in_process_queue_tests
    PRIVATE
        th25_ctrl
        GTest::gtest
        GTest::gtest_main
)
target_compile_options(th25_in_process_queue_tests PRIVATE ${TH25_STRICT_WARNINGS})
gtest_discover_tests(th25_in_process_queue_tests)

# UNIT-201 SafetyCoreOrchestrator (LifecycleState 状態機械) の本格 UT
# (Step 19 / CR-0007, UTPR-TH25-001 v0.3 §7.2).
# - 状態遷移許可表 (SDD §6.1) の網羅試験
# - 不正遷移は Halted + shutdown_requested_ で構造的に拒否
# - HZ-001 (mode/target mismatch): RCM-001 状態機械の構造的不可能化
# - HZ-002 (race condition): SPSC イベント配送 concurrent 試験 (tsan)
add_executable(th25_safety_core_orchestrator_tests
    unit/test_safety_core_orchestrator.cpp
)
target_link_libraries(th25_safety_core_orchestrator_tests
    PRIVATE
        th25_ctrl
        GTest::gtest
        GTest::gtest_main
)
target_compile_options(th25_safety_core_orchestrator_tests PRIVATE ${TH25_STRICT_WARNINGS})
gtest_discover_tests(th25_safety_core_orchestrator_tests)

# UNIT-202 TreatmentModeManager (HZ-001 直接対応の中核) の本格 UT
# (Step 20 / CR-0008, UTPR-TH25-001 v0.4 §7.2).
# - SDD §4.3 モード遷移許可表 (SRS-002) の網羅試験
# - 強い型 Energy_MeV / Energy_MV による単位安全 (RCM-008)
# - BeamState != Off / エネルギー範囲外 (SRS-004) の構造的拒否
# - HZ-001 (mode/target mismatch): Therac-25 Tyler 事故型の発現経路を阻止
# - HZ-002 (race condition): std::atomic<TreatmentMode> N writer + reader 並行
add_executable(th25_treatment_mode_manager_tests
    unit/test_treatment_mode_manager.cpp
)
target_link_libraries(th25_treatment_mode_manager_tests
    PRIVATE
        th25_ctrl
        GTest::gtest
        GTest::gtest_main
)
target_compile_options(th25_treatment_mode_manager_tests PRIVATE ${TH25_STRICT_WARNINGS})
gtest_discover_tests(th25_treatment_mode_manager_tests)

# UNIT-203 BeamController (RCM-017 中核 / ビームオン許可フラグ消費型) の本格 UT
# (Step 21 / CR-0009, UTPR-TH25-001 v0.5 §7.2).
# - SDD §4.4 ビームオン許可フラグの compare_exchange による 1 回限り消費
# - BeamState 4 状態機械 (Off/Arming/On/Stopping)
# - HZ-001/HZ-004 直接対応, RCM-017 中核
# - HZ-002 (race condition): N thread による許可フラグ競合で唯一 1 件のみ成功 (tsan)
add_executable(th25_beam_controller_tests
    unit/test_beam_controller.cpp
)
target_link_libraries(th25_beam_controller_tests
    PRIVATE
        th25_ctrl
        GTest::gtest
        GTest::gtest_main
)
target_compile_options(th25_beam_controller_tests PRIVATE ${TH25_STRICT_WARNINGS})
gtest_discover_tests(th25_beam_controller_tests)

# UNIT-204 DoseManager (HZ-005 直接 + HZ-003 構造的排除の中核) の本格 UT
# (Step 22 / CR-0010, UTPR-TH25-001 v0.6 §7.2).
# - SDD §4.5 ドーズ目標管理 + 積算 + 目標到達検知
# - PulseCounter (UNIT-200) を内包し std::atomic<uint64_t> 5,800 万年余裕で HZ-003 構造的排除
# - target_pulses_ を std::atomic<uint64_t> で整数比較化 (浮動小数 precision 回避)
# - target_reached_ / target_set_ を std::atomic<bool> でフラグ管理
# - HZ-002 (race condition): producer-producer-reader / producer-setter 並行 (tsan)
# - HZ-005 (dose calculation): SRS-008 範囲境界値 + lifecycle ガード + 単位変換
# - HZ-007 (legacy preconditions): is_always_lock_free static_assert を 6 ユニット目に拡大
add_executable(th25_dose_manager_tests
    unit/test_dose_manager.cpp
)
target_link_libraries(th25_dose_manager_tests
    PRIVATE
        th25_ctrl
        GTest::gtest
        GTest::gtest_main
)
target_compile_options(th25_dose_manager_tests PRIVATE ${TH25_STRICT_WARNINGS})
gtest_discover_tests(th25_dose_manager_tests)

# UNIT-205 TurntableManager (HZ-001 直接対応 + RCM-005 中核 / 3 系統センサ集約 med-of-3)
# の本格 UT (Step 23 / CR-0011, UTPR-TH25-001 v0.7 §7.2).
# - SDD §4.6 ターンテーブル位置指令 + 3 系統独立センサ集約 + 偏差判定
# - std::atomic<double> 4 個 (sensor_0/1/2 + target) で並行アクセス保護
# - has_discrepancy(threshold=1.0 mm) でセンサ単一故障・3 系統不一致を構造的検出
# - is_in_position(±0.5 mm) で SRS-006 in-position 判定
# - HZ-001 (Tyler 事故型「ターンテーブル位置不整合」) 直接対応
# - HZ-002 (race condition): 1 writer + 4 reader 並行 (tsan)
# - HZ-007 (legacy preconditions): is_always_lock_free static_assert を 7 ユニット目に拡大
add_executable(th25_turntable_manager_tests
    unit/test_turntable_manager.cpp
)
target_link_libraries(th25_turntable_manager_tests
    PRIVATE
        th25_ctrl
        GTest::gtest
        GTest::gtest_main
)
target_compile_options(th25_turntable_manager_tests PRIVATE ${TH25_STRICT_WARNINGS})
gtest_discover_tests(th25_turntable_manager_tests)

# UNIT-206 BendingMagnetManager (HZ-005 部分対応 + RCM-008 中核 / 強い型による単位安全)
# の本格 UT (Step 24 / CR-0012, UTPR-TH25-001 v0.8 §7.2).
# - SDD §4.7 ベンディング磁石電流指令 (Energy_MeV / Energy_MV → MagnetCurrent_A)
# - EnergyMagnetMap 線形補間 (slope, intercept) を constexpr で内部保持
# - 偏差 ±5% 判定 (SRS-ALM-004) + SRS-D-006 範囲 (0.0〜500.0 A) の防御層
# - std::atomic<double> 2 個 (target/actual) + std::atomic<bool> (target_set_) で並行アクセス保護
# - HZ-005 (dose calculation): 強い型 Energy_MeV / Energy_MV / MagnetCurrent_A (RCM-008)
# - HZ-001 (mode/target mismatch): mode + energy 不整合の構造的拒否
# - HZ-002 (race condition): 1 setter + 4 reader 並行 (tsan)
# - HZ-007 (legacy preconditions): is_always_lock_free static_assert を 8 ユニット目に拡大
add_executable(th25_bending_magnet_manager_tests
    unit/test_bending_magnet_manager.cpp
)
target_link_libraries(th25_bending_magnet_manager_tests
    PRIVATE
        th25_ctrl
        GTest::gtest
        GTest::gtest_main
)
target_compile_options(th25_bending_magnet_manager_tests PRIVATE ${TH25_STRICT_WARNINGS})
gtest_discover_tests(th25_bending_magnet_manager_tests)

# UNIT-208 StartupSelfCheck (RCM-013 中核 + HZ-009 部分対応 / 起動時 4 項目自己診断)
# の本格 UT (Step 25 / CR-0013, UTPR-TH25-001 v0.9 §7.2).
# - SDD §4.9 起動時 4 項目自己診断 (電子銃 0 mA / ターンテーブル Light 位置 / 磁石 0 A
#   / ドーズカウンタ 0)
# - Manager 群経由 (UNIT-204/205/206) でセンサ計測値取得、UNIT-301 ElectronGunSim 未実装
#   のため inject_electron_gun_current で UT 駆動可能
# - 4 項目それぞれ既存 ErrorCode 階層で分類 (ElectronGunCurrentOutOfRange /
#   TurntableOutOfPosition / MagnetCurrentDeviation / InternalUnexpectedState)
# - HZ-009 (起動時不定動作) 部分対応, RCM-013 中核
# - HZ-002 (race condition): std::atomic<double> + UT-208-29 1 setter + 4 reader 並行 (tsan)
# - HZ-007 (legacy preconditions): is_always_lock_free static_assert を 9 ユニット目に拡大
add_executable(th25_startup_self_check_tests
    unit/test_startup_self_check.cpp
)
target_link_libraries(th25_startup_self_check_tests
    PRIVATE
        th25_ctrl
        GTest::gtest
        GTest::gtest_main
)
target_compile_options(th25_startup_self_check_tests PRIVATE ${TH25_STRICT_WARNINGS})
gtest_discover_tests(th25_startup_self_check_tests)

# UNIT-301 ElectronGunSim (Hardware Simulator 層の最初の具体実装) の本格 UT
# (Step 26 / CR-0014, UTPR-TH25-001 v0.10 §7.2).
# - SDD §4.12 仮想電子銃応答模擬 (set_current + read_actual_current)
# - SRS-IF-001 / SRS-O-001 / SRS-I-008 / SRS-D-008 (0.0〜10.0 mA) 完全一致
# - std::atomic<double> + std::atomic<bool> + std::atomic<std::uint32_t> の三重 atomic
# - ノイズ模擬 ±1% (default OFF、enable_noise(seed) で有効化、UT 駆動安定化)
# - 物理的飽和 (clamp_to_range) で範囲外指令を吸収
# - HZ-002 (race condition): UT-301-19 で 1 setter + 4 reader × 5000 race-free (tsan)
# - HZ-007 (legacy preconditions): is_always_lock_free static_assert 三重表明を 10 ユニット目に拡大
add_executable(th25_electron_gun_sim_tests
    unit/test_electron_gun_sim.cpp
)
target_link_libraries(th25_electron_gun_sim_tests
    PRIVATE
        th25_sim
        GTest::gtest
        GTest::gtest_main
)
target_compile_options(th25_electron_gun_sim_tests PRIVATE ${TH25_STRICT_WARNINGS})
gtest_discover_tests(th25_electron_gun_sim_tests)

# UNIT-302 BendingMagnetSim (Hardware Simulator 層 2 ユニット目) の本格 UT
# (Step 27 / CR-0015, UTPR-TH25-001 v0.11 §7.2).
# - SDD §4.13 仮想ベンディング磁石応答模擬 (set_current + read_actual_current)
# - SRS-IF-002 / SRS-O-002 / SRS-I-007 / SRS-D-006 (0.0〜500.0 A) 完全一致
# - std::atomic<double> + std::atomic<bool> + std::atomic<std::uint32_t> の三重 atomic
# - ノイズ模擬 ±2% (default OFF、enable_noise(seed) で有効化、UT 駆動安定化)
# - 物理的飽和 (clamp_to_range) で範囲外指令を吸収
# - HZ-002 (race condition): UT-302-19 で 1 setter + 4 reader × 5000 race-free (tsan)
# - HZ-007 (legacy preconditions): is_always_lock_free static_assert 三重表明を 11 ユニット目に拡大
add_executable(th25_bending_magnet_sim_tests
    unit/test_bending_magnet_sim.cpp
)
target_link_libraries(th25_bending_magnet_sim_tests
    PRIVATE
        th25_sim
        GTest::gtest
        GTest::gtest_main
)
target_compile_options(th25_bending_magnet_sim_tests PRIVATE ${TH25_STRICT_WARNINGS})
gtest_discover_tests(th25_bending_magnet_sim_tests)

# UNIT-303 TurntableSim (Hardware Simulator 層 3 ユニット目 / 故障注入機能あり) の本格 UT
# (Step 28 / CR-0016, UTPR-TH25-001 v0.12 §7.2).
# - SDD §4.12 仮想ターンテーブル応答模擬 (command_position + read_sensor + inject_fault)
# - SRS-IF-003 / SRS-O-003 / SRS-I-005 / SRS-006 / SRS-D-007 (-100.0〜+100.0 mm) 完全一致
# - std::atomic<double> + std::atomic<bool> + std::atomic<std::uint32_t> + std::atomic<FaultMode>
#   の四重 atomic (UNIT-301/302 三重 + FaultMode atomic を追加)
# - 3 系統独立センサ (Sensor0/1/2) + 故障モード 4 種 (None/StuckAt/Delay/NoResponse)
# - センサノイズ ±0.1 mm 絶対値 (default OFF、enable_sensor_noise(seed) で有効化、
#   UT 駆動安定化、UNIT-301/302 と同パターン)
# - 物理的飽和 (clamp_to_range) で範囲外指令を吸収
# - HZ-001 (Tyler 事故型「ターンテーブル位置不整合」): 故障注入 9 通りで UNIT-205
#   med-of-3 + has_discrepancy 判定の試験ハーネス基盤
# - HZ-002 (race condition): UT-303-23 commander + multi reader / UT-303-24
#   fault injector + multi reader race-free × 5000 (tsan)
# - HZ-007 (legacy preconditions): is_always_lock_free static_assert 四重表明を 12 ユニット目に拡大
add_executable(th25_turntable_sim_tests
    unit/test_turntable_sim.cpp
)
target_link_libraries(th25_turntable_sim_tests
    PRIVATE
        th25_sim
        GTest::gtest
        GTest::gtest_main
)
target_compile_options(th25_turntable_sim_tests PRIVATE ${TH25_STRICT_WARNINGS})
gtest_discover_tests(th25_turntable_sim_tests)

# UNIT-207 SafetyMonitor (Inc.2 で本格化、本 v0.1 では空殻) の本格 UT
# (Step 30 / CR-0018, UTPR-TH25-001 v0.14 §7.2).
# - SDD §4.8 独立並行タスクのインターロック監視 (空殻 IF 骨格)
# - run_monitor_thread / stop / is_stop_requested / tick_count / reset_for_test
# - std::atomic<bool> + std::atomic<std::uint64_t> の二重 atomic
# - HZ-002 (race condition): UT-207-09 1 runner + 4 reader / UT-207-10 N stopper 並行 (tsan)
# - HZ-007 (legacy preconditions): is_always_lock_free static_assert 二重表明を 14 ユニット目
#   (Inc.1 範囲全 14 ユニット完成) に拡大
add_executable(th25_safety_monitor_tests
    unit/test_safety_monitor.cpp
)
target_link_libraries(th25_safety_monitor_tests
    PRIVATE
        th25_ctrl
        GTest::gtest
        GTest::gtest_main
)
target_compile_options(th25_safety_monitor_tests PRIVATE ${TH25_STRICT_WARNINGS})
gtest_discover_tests(th25_safety_monitor_tests)

# UNIT-304 IonChamberSim (Hardware Simulator 層 4 ユニット目 / 本層最終 / 故障注入機能あり)
# の本格 UT (Step 29 / CR-0017, UTPR-TH25-001 v0.13 §7.2).
# - SDD §4.12 仮想イオンチェンバ応答模擬 (read_dose + inject_saturation + inject_channel_failure)
# - SRS-IF-004 / SRS-I-006 / SRS-D-004 (0.0〜10000.0 cGy 計測値範囲) 完全一致
# - std::atomic<double> × 2 + std::atomic<FaultMode> × 2 の二重 atomic 種
#   (UNIT-303 四重から本ユニットに必要な atomic 種に絞り込み、シンプルさ優先)
# - 2 系統独立チャネル (Channel0/Channel1) + 故障モード 3 種 (None/Saturation/ChannelFailure)
# - 物理的飽和 (clamp_to_range) で範囲外指令を吸収 (SRS-I-006 0.0〜10000.0 cGy)
# - HZ-005 (dose calculation): 故障注入 Saturation/ChannelFailure × 2 チャネル + 部分故障 +
#   混在故障モードで UNIT-204 DoseManager + UNIT-207 SafetyMonitor 結線時の冗長性検証経路
#   を IT/ST で機械検証可能化する試験ハーネス基盤
# - HZ-002 (race condition): UT-304-23 incrementer + multi reader / UT-304-24
#   fault injector + multi reader race-free × 5000 (tsan)
# - HZ-007 (legacy preconditions): is_always_lock_free static_assert 二重表明
#   (double + FaultMode) を 13 ユニット目に拡大 (本層最終)
add_executable(th25_ion_chamber_sim_tests
    unit/test_ion_chamber_sim.cpp
)
target_link_libraries(th25_ion_chamber_sim_tests
    PRIVATE
        th25_sim
        GTest::gtest
        GTest::gtest_main
)
target_compile_options(th25_ion_chamber_sim_tests PRIVATE ${TH25_STRICT_WARNINGS})
gtest_discover_tests(th25_ion_chamber_sim_tests)

# UNIT-209 AuditLogger (Inc.4 で本格化、本 v0.1 では空殻) の本格 UT
# (Step 31 / CR-0019, UTPR-TH25-001 v0.15 §7.2).
# - SDD §4.10 安全関連イベントの集約・永続化ユニット (空殻 IF 骨格)
# - publish / run_io_thread / stop / is_stop_requested / pending_count /
#   processed_count / reset_for_test
# - std::atomic<bool> + std::atomic<std::uint64_t> + std::mutex (cerr 出力アトミック性)
# - HZ-002 (race condition): UT-209-09 4 publishers race-free / UT-209-10
#   1 io_runner + 2 publishers + 4 readers 並行 (tsan)
# - HZ-006 (cryptic / bypass): publish() で全 UNIT 安全関連イベントを集約する
#   事後監査経路の構造的位置づけを確立 (Inc.4 で永続化により事後追跡可能性を完成)
# - HZ-007 (legacy preconditions): is_always_lock_free static_assert 二重表明を
#   15 ユニット目 (Inc.1 範囲全 14 ユニット + 空殻ユニット第 2 例) に拡大
add_executable(th25_audit_logger_tests
    unit/test_audit_logger.cpp
)
target_link_libraries(th25_audit_logger_tests
    PRIVATE
        th25_ctrl
        GTest::gtest
        GTest::gtest_main
)
target_compile_options(th25_audit_logger_tests PRIVATE ${TH25_STRICT_WARNINGS})
gtest_discover_tests(th25_audit_logger_tests)

# UNIT-210 CoreAuthenticationGateway (Inc.4 で本格化、本 v0.1 では空殻) の本格 UT
# (Step 32 / CR-0020, UTPR-TH25-001 v0.16 §7.2).
# - SDD §4.11 操作者認証ゲートウェイ (空殻 IF 骨格、verify() のみ同期 API)
# - verify (常時 AuthRequired) / verify_count / reset_for_test
# - std::atomic<std::uint64_t> 単一 atomic (空殻範囲、Inc.4 で MessageBus 経由
#   非同期化時に std::atomic<bool> 等を追加予定)
# - AuthenticationVerify / AuthenticationResult / OperatorId / Credential 構造体
#   (IF-U-010 と整合、Inc.4 で MessageBus 経由非同期 dispatch 時に使用)
# - HZ-002 (race condition): UT-210-11 で 4 threads × 500 verify race-free (tsan)
# - HZ-006 (cryptic / bypass): verify() 常時拒否で「未認証時は治療パラメータ確定
#   不可」インタフェース契約を IF 骨格段階で確立 (Inc.4 で実認証 + AuditLogger 転送)
# - HZ-010 (操作者認証・権限管理不備): 空殻 IF 骨格として確立 (Inc.4 で本格化)
# - HZ-007 (legacy preconditions): is_always_lock_free static_assert 単一表明を
#   16 ユニット目 (Inc.1 範囲全 14 ユニット + 空殻ユニット第 3 例) に拡大
add_executable(th25_core_authentication_gateway_tests
    unit/test_core_authentication_gateway.cpp
)
target_link_libraries(th25_core_authentication_gateway_tests
    PRIVATE
        th25_ctrl
        GTest::gtest
        GTest::gtest_main
)
target_compile_options(th25_core_authentication_gateway_tests PRIVATE ${TH25_STRICT_WARNINGS})
gtest_discover_tests(th25_core_authentication_gateway_tests)

# UNIT-101 PrescriptionEditor (Inc.4 で本格化、本 v0.1 では空殻) の本格 UT
# (Step 34 / CR-0022, UTPR-TH25-001 v0.19 §7.2).
# - SDD §3.2 治療計画入力 UI (空殻 IF 骨格、submit_prescription() 同期 API のみ)
# - submit_prescription (常時 InternalUnexpectedState) / submit_count / reset_for_test
# - std::atomic<std::uint64_t> 単一 atomic (空殻範囲、Inc.4 で IPC 経由非同期化時に
#   std::atomic<bool> 等を追加予定)
# - 新 namespace th25_ui (SAD §3 ARCH-001 Operator UI Process、Inc.4 でプロセス分離予定)
# - 新ライブラリ th25_ui (src/th25_ui/) で th25_ctrl の型を PUBLIC リンク
# - HZ-002 (race condition): UT-101-11 で 4 threads × 500 submit race-free (tsan)
# - HZ-006 (cryptic / bypass): submit_prescription() 常時拒否で「UI 層未実装時は
#   治療計画送信を受付不可」インタフェース契約を IF 骨格段階で確立
# - HZ-007 (legacy preconditions): is_always_lock_free static_assert 単一表明を
#   17 ユニット目 (Inc.1 範囲全 14 ユニット + 空殻ユニット第 4 例) に拡大
add_executable(th25_prescription_editor_tests
    unit/test_prescription_editor.cpp
)
target_link_libraries(th25_prescription_editor_tests
    PRIVATE
        th25_ui
        th25_ctrl
        GTest::gtest
        GTest::gtest_main
)
target_compile_options(th25_prescription_editor_tests PRIVATE ${TH25_STRICT_WARNINGS})
gtest_discover_tests(th25_prescription_editor_tests)

# UNIT-103 AlarmDisplay (Inc.4 で本格化、本 v0.1 では空殻) の本格 UT
# (Step 36 / CR-0024, UTPR-TH25-001 v0.23 §7.2).
# - SDD §3.2 アラーム/エラー表示 UI (空殻 IF 骨格、display_alarm() 同期 API のみ)
# - display_alarm(Severity, ErrorCode) (常時 InternalUnexpectedState) /
#   display_count / reset_for_test
# - std::atomic<std::uint64_t> 単一 atomic (空殻範囲、Inc.4 で IPC 経由非同期化時に
#   std::atomic<bool> 等を追加予定)
# - 既存 th25_ui ライブラリ (Step 34/35 で確立) の 3 cpp 目として組込
# - 新規型定義なし (Severity / ErrorCode は UNIT-200 common_types.hpp で定義済を再利用)
# - HZ-002 (race condition): UT-103-11 で 4 threads × 500 display_alarm race-free (tsan)
# - HZ-006 (cryptic / bypass): display_alarm() 常時拒否で「UI 層未実装時はアラーム表示
#   要求を受付不可」インタフェース契約を IF 骨格段階で確立 (RCM-009 中核の構造的位置づけ、
#   Therac-25 East Texas 事故型「MALFUNCTION 54 単独表示」廃止に向けた設計)
# - HZ-007 (legacy preconditions): is_always_lock_free static_assert 単一表明を
#   19 ユニット目 (Inc.1 範囲全 14 ユニット + 空殻ユニット第 6 例) に拡大
add_executable(th25_alarm_display_tests
    unit/test_alarm_display.cpp
)
target_link_libraries(th25_alarm_display_tests
    PRIVATE
        th25_ui
        th25_ctrl
        GTest::gtest
        GTest::gtest_main
)
target_compile_options(th25_alarm_display_tests PRIVATE ${TH25_STRICT_WARNINGS})
gtest_discover_tests(th25_alarm_display_tests)

# UNIT-102 BeamCommandConsole (Inc.4 で本格化、本 v0.1 では空殻) の本格 UT
# (Step 35 / CR-0023, UTPR-TH25-001 v0.20 §7.2).
# - SDD §3.2 ビームオン/オフ操作 UI (空殻 IF 骨格、submit_beam_command() 同期 API のみ)
# - BeamCommand 列挙体 { On, Off } (SRS-I-004 と整合、空殻範囲で本ヘッダ内に定義)
# - submit_beam_command (常時 InternalUnexpectedState) / submit_count / reset_for_test
# - std::atomic<std::uint64_t> 単一 atomic (空殻範囲、Inc.4 で IPC 経由非同期化時に
#   std::atomic<bool> 等を追加予定)
# - 既存 th25_ui ライブラリ (Step 34 で新設) の 2 cpp 目として組込
# - HZ-002 (race condition): UT-102-11 で 4 threads × 500 submit race-free (tsan)
# - HZ-006 (cryptic / bypass): submit_beam_command() 常時拒否で「UI 層未実装時は
#   致死的エラーバイパス試行を含めてビーム指令を受付不可」インタフェース契約を IF
#   骨格段階で確立 (RCM-010 中核の構造的位置づけ)
# - HZ-007 (legacy preconditions): is_always_lock_free static_assert 単一表明を
#   18 ユニット目 (Inc.1 範囲全 14 ユニット + 空殻ユニット第 5 例) に拡大
add_executable(th25_beam_command_console_tests
    unit/test_beam_command_console.cpp
)
target_link_libraries(th25_beam_command_console_tests
    PRIVATE
        th25_ui
        th25_ctrl
        GTest::gtest
        GTest::gtest_main
)
target_compile_options(th25_beam_command_console_tests PRIVATE ${TH25_STRICT_WARNINGS})
gtest_discover_tests(th25_beam_command_console_tests)
