# th25_ui: Operator UI Process (SAD §3 ARCH-001、Inc.4 で本格化)
#
# Step 34 (CR-0022) で UNIT-101 PrescriptionEditor を空殻実装第 4 例として
# 最初の具体実装として追加。Inc.4 で本格 UI 実装 + プロセス分離 + IPC 経由
# PrescriptionSet メッセージ送信に発展予定。

add_library(th25_ui STATIC
    src/prescription_editor.cpp
)

target_include_directories(th25_ui PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

# UNIT-101 は th25_ctrl/common_types.hpp の TreatmentMode / Energy_MeV /
# Energy_MV / DoseUnit_cGy / Result / ErrorCode 型を利用するため
# th25_ctrl にリンク (PUBLIC: 利用側にも伝播)。
target_link_libraries(th25_ui PUBLIC th25_ctrl)

target_compile_options(th25_ui PRIVATE ${TH25_STRICT_WARNINGS})
