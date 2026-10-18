cmake_minimum_required(VERSION 3.20)

project(MarineRadarSim VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_standard_project_setup()

add_executable(MarineRadarSim
    src/main.cpp

    src/domain/simulation/simulation.cpp
    src/domain/simulation/simulation.h

    src/presentation/main_window/main_window.cpp
    src/presentation/main_window/main_window.h

    src/presentation/ppi/ppi_widget.cpp
    src/presentation/ppi/ppi_widget.h

    src/presentation/settings/settings_page.cpp
    src/presentation/settings/settings_page.h

    src/presentation/theme/theme.cpp
    src/presentation/theme/theme.h

    src/presentation/title_bar/title_bar.cpp
    src/presentation/title_bar/title_bar.h

    src/presentation/widgets/clock_icon.cpp
    src/presentation/widgets/clock_icon.h

    src/presentation/widgets/data_panel.cpp
    src/presentation/widgets/data_panel.h

    src/presentation/widgets/level_control.cpp
    src/presentation/widgets/level_control.h

    src/presentation/widgets/tuning_indicator.cpp
    src/presentation/widgets/tuning_indicator.h
)

target_include_directories(MarineRadarSim
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_definitions(MarineRadarSim
    PRIVATE
        MRS_VERSION="${PROJECT_VERSION}"
)

target_compile_options(MarineRadarSim
    PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
        $<$<CXX_COMPILER_ID:MSVC>:/W4>
)

target_link_libraries(MarineRadarSim
    PRIVATE
        Qt6::Widgets
)
