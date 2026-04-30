# th25_ctrl: Inc.1 のビーム生成・モード制御コア(プレースホルダ)
# 本 CMakeLists.txt は Step 14 でビルドシステムを成立させるための骨格。
# 実体ソースは Step 17+ で SDD-TH25-001 v0.1 に基づき追加する。

add_library(th25_ctrl STATIC
    src/version.cpp
    src/safety_core_orchestrator.cpp
    src/treatment_mode_manager.cpp
)

target_include_directories(th25_ctrl PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_compile_options(th25_ctrl PRIVATE ${TH25_STRICT_WARNINGS})
