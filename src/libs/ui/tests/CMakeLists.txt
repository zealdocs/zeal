# SPDX-FileCopyrightText: Oleg Shparber, et al. <https://zealdocs.org>
# SPDX-License-Identifier: MIT

find_package(Qt6 REQUIRED COMPONENTS Test Network Widgets)
find_package(LibArchive REQUIRED)

add_executable(docsetsdialog_test docsetsdialog_test.cpp)
target_link_libraries(docsetsdialog_test PRIVATE
    Ui Core Registry Util Qt6::Test Qt6::Network Qt6::Widgets LibArchive::LibArchive
)

zeal_add_test(docsetsdialog_test)
set_tests_properties(docsetsdialog_test PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    TIMEOUT 60
)
