cmake_minimum_required(VERSION 3.22)

project(OB8Native VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
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
