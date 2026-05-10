# th25_ui: Operator UI Process (SAD §3 ARCH-001、Inc.4 で本格化)
#
# Step 34 (CR-0022) で UNIT-101 PrescriptionEditor を空殻実装第 4 例として
# 最初の具体実装として追加。Step 35 (CR-0023) で UNIT-102 BeamCommandConsole
# を空殻実装第 5 例として 2 cpp 目に追加。Step 36 (CR-0024) で UNIT-103
# AlarmDisplay を空殻実装第 6 例として 3 cpp 目に追加。Step 37 (CR-0025) で
# UNIT-104 UIAuthenticationGateway を空殻実装第 7 例として 4 cpp 目に追加
# (Inc.1 範囲の全 UI 系 4 ユニットの空殻実装が完成)。Inc.4 で本格 UI 実装
# + プロセス分離 + IPC 経由 PrescriptionSet / BeamCommand メッセージ送信
# + アラーム配信受信 + 人間可読メッセージ生成 + Core 側 UNIT-210 への
# IF-U-010 経由 AuthenticationVerify 送信 + AuthenticationResult 待ちに発展予定。

add_library(th25_ui STATIC
    src/prescription_editor.cpp
    src/beam_command_console.cpp
    src/alarm_display.cpp
    src/ui_authentication_gateway.cpp
)

target_include_directories(th25_ui PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

# UNIT-101 は th25_ctrl/common_types.hpp の TreatmentMode / Energy_MeV /
# Energy_MV / DoseUnit_cGy / Result / ErrorCode 型を、UNIT-102 は Result /
# ErrorCode 型を、UNIT-103 は Severity / ErrorCode / Result 型を、
# UNIT-104 は OperatorId / Credential (UNIT-210 ヘッダで先行定義済) +
# Result / ErrorCode 型を利用するため th25_ctrl にリンク
# (PUBLIC: 利用側にも伝播)。
target_link_libraries(th25_ui PUBLIC th25_ctrl)

target_compile_options(th25_ui PRIVATE ${TH25_STRICT_WARNINGS})
