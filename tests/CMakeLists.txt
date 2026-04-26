# GoogleTest 経由のテストツリー(Inc.1 のスモークテストのみ).
# 各 RCM に対応する UT は Step 17+ で追加する.

find_package(GTest CONFIG REQUIRED)

# Smoke test: ビルドシステム + GoogleTest + Sanitizer の動作確認のみを目的とする.
# 機能要求の検証は Step 17+ の UT 群で実施.
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

include(GoogleTest)
gtest_discover_tests(th25_smoke_tests)
