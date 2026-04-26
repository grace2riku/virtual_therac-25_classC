# th25_sim: 仮想ハードウェアモデル(Inc.1 のプレースホルダ)
# 電子銃・ベンディング磁石・ターンテーブル・イオンチェンバ等の C++ クラス群は
# Step 17+ で SDD-TH25-001 v0.1 に基づき追加する。

add_library(th25_sim STATIC
    src/sim_stub.cpp
)

target_include_directories(th25_sim PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_compile_options(th25_sim PRIVATE ${TH25_STRICT_WARNINGS})
