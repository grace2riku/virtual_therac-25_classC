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
