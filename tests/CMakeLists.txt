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
