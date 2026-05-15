cmake_minimum_required(VERSION 3.22)

project(OB8Native VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Emit compile_commands.json so clang-tidy / IDEs / language servers can
# find the right -I / -D flags for each source file.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "" FORCE)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# ---- macOS specifics ---------------------------------------------------------
if(APPLE)
    # Minimum supported macOS. 10.13 is JUCE 8's lower bound.
    if(NOT CMAKE_OSX_DEPLOYMENT_TARGET)
        set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "" FORCE)
    endif()
    # Build a universal binary by default (Intel + Apple Silicon).
    if(NOT CMAKE_OSX_ARCHITECTURES)
        set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
    endif()
endif()

include(FetchContent)

set(JUCE_GIT_TAG "8.0.4" CACHE STRING "JUCE git tag/branch to fetch")

FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG ${JUCE_GIT_TAG}
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(OB8Native
    COMPANY_NAME            "OB8Native"
    COMPANY_WEBSITE         "https://example.com"
    BUNDLE_ID               "com.example.ob8native"
    PLUGIN_MANUFACTURER_CODE Ob8n
    PLUGIN_CODE             Ob8s
    FORMATS                 VST3 Standalone AU
    PRODUCT_NAME            "OB-8 Native"
    IS_SYNTH                TRUE
    NEEDS_MIDI_INPUT        TRUE
    NEEDS_MIDI_OUTPUT       FALSE
    IS_MIDI_EFFECT          FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES         "Instrument" "Synth"
    AU_MAIN_TYPE            "kAudioUnitType_MusicDevice"
)

juce_generate_juce_header(OB8Native)

target_sources(OB8Native PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/Parameters.cpp
    Source/Parameters.h
    Source/FactoryPresets.h
    Source/DSP/Voice.cpp
    Source/DSP/Voice.h
    Source/DSP/PolyBLEPOscillator.h
    Source/DSP/StateVariableFilter.h
    Source/DSP/Envelope.h
    Source/DSP/LFO.h
    Source/DSP/NoiseGenerator.h
    Source/DSP/AnalogDrift.h
    Source/DSP/Oversampler.h
    Source/DSP/DCBlocker.h
    Source/GUI/OB8LookAndFeel.cpp
    Source/GUI/OB8LookAndFeel.h
    Source/GUI/OB8Knob.cpp
    Source/GUI/OB8Knob.h
)

target_compile_definitions(OB8Native PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0
)

target_link_libraries(OB8Native PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_dsp
    juce::juce_gui_extra
    juce::juce_core
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

target_include_directories(OB8Native PRIVATE Source)

# ---- Linting / formatting ----------------------------------------------------
# Manual-invoke targets so the regular `cmake --build` cycle stays fast.
# Both rely on the project-root .clang-format / .clang-tidy.
file(GLOB_RECURSE OB8_LINT_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.hpp"
)

find_program(OB8_CLANG_FORMAT NAMES clang-format)
if(OB8_CLANG_FORMAT)
    # `cmake --build build --target format`        -> rewrite files in place
    # `cmake --build build --target format-check`  -> report and fail on diffs
    add_custom_target(format
        COMMAND ${OB8_CLANG_FORMAT} -i --style=file ${OB8_LINT_SOURCES}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Running clang-format on ${CMAKE_CURRENT_SOURCE_DIR}/Source"
        VERBATIM
    )
    add_custom_target(format-check
        COMMAND ${OB8_CLANG_FORMAT} --dry-run --Werror --style=file ${OB8_LINT_SOURCES}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Checking clang-format compliance"
        VERBATIM
    )
endif()

find_program(OB8_CLANG_TIDY NAMES clang-tidy)
if(OB8_CLANG_TIDY)
    # `cmake --build build --target tidy` -> run clang-tidy across our sources
    # using build/compile_commands.json for accurate include paths.
    add_custom_target(tidy
        COMMAND ${OB8_CLANG_TIDY}
                -p ${CMAKE_CURRENT_BINARY_DIR}
                --quiet
                ${OB8_LINT_SOURCES}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Running clang-tidy across Source/"
        VERBATIM
    )
endif()

# ---- macOS post-build: strip xattrs and ad-hoc codesign all bundles ----------
# Locally-built JUCE bundles have no signature, so Gatekeeper refuses to open
# them with "破損しているか不完全である可能性があります". The fix is to
# remove the quarantine attribute and apply an ad-hoc signature.
if(APPLE)
    foreach(fmt IN ITEMS Standalone VST3 AU)
        set(plugin_target OB8Native_${fmt})
        if(TARGET ${plugin_target})
            add_custom_command(TARGET ${plugin_target} POST_BUILD
                COMMAND xattr -cr  "$<TARGET_BUNDLE_DIR:${plugin_target}>"
                COMMAND codesign --force --deep --sign - --timestamp=none
                                 "$<TARGET_BUNDLE_DIR:${plugin_target}>"
                COMMENT "Ad-hoc codesign ${plugin_target}"
                VERBATIM
            )
        endif()
    endforeach()
endif()
