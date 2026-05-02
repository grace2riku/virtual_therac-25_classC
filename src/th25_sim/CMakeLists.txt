# th25_sim: 仮想ハードウェアモデル(Inc.1)
# Step 14 でプレースホルダ (sim_stub.cpp) として骨格を確立。
# Step 26 (CR-0014) で UNIT-301 ElectronGunSim を最初の具体実装として追加。
# Step 27 (CR-0015) で UNIT-302 BendingMagnetSim を 2 ユニット目として追加。
# UNIT-303/304 (TurntableSim / IonChamberSim) は後続 Step で追加。

add_library(th25_sim STATIC
    src/sim_stub.cpp
    src/electron_gun_sim.cpp
    src/bending_magnet_sim.cpp
)

target_include_directories(th25_sim PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

# UNIT-301 は th25_ctrl/common_types.hpp の ElectronGunCurrent_mA 強い型を利用するため
# th25_ctrl にリンク (PUBLIC: 利用側にも伝播)。
target_link_libraries(th25_sim PUBLIC th25_ctrl)

target_compile_options(th25_sim PRIVATE ${TH25_STRICT_WARNINGS})
