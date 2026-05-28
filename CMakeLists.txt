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
    Source/MacroBridge.h
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
    Source/DSP/Delay.h
    Source/DSP/Reverb.h
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

# ---- Bundled fonts ----------------------------------------------------------
# IBM Plex Mono (OFL 1.1) bundled into the binary so the editor uses the
# same compressed monospace typeface on every install.
#
# The IBM/plex GitHub layout changed across versions; we try several known
# mirrors in order until one returns a real TTF. As a final fallback the
# build is allowed to proceed *without* the bundled font -- the editor
# detects that case at runtime and falls back to the system mono face.
set(OB8_FONT_DIR "${CMAKE_CURRENT_BINARY_DIR}/_fonts")
file(MAKE_DIRECTORY "${OB8_FONT_DIR}")

# Mapping from local file -> list of candidate URLs.
# Order matters: the first one to come back as a successful HTTP 200 wins.
#
# 1) Fontsource via jsDelivr -- versioned, very stable, no GitHub rate-limits.
# 2) jsDelivr's GitHub mirror of IBM/plex at a known-stable tag.
# 3) Raw GitHub at the same tag.
function(_ob8_try_download local_path url_list)
    foreach(_url IN LISTS url_list)
        message(STATUS "  trying ${_url}")
        file(DOWNLOAD "${_url}" "${local_path}"
             STATUS _dl_status
             TIMEOUT 30)
        list(GET _dl_status 0 _code)
        if(_code EQUAL 0)
            file(SIZE "${local_path}" _sz)
            if(_sz GREATER 4096)   # rule out HTML 404 pages
                message(STATUS "  ok (${_sz} bytes)")
                return()
            endif()
            file(REMOVE "${local_path}")
        endif()
    endforeach()
    file(TOUCH "${local_path}.failed")
endfunction()

set(_fontsource_plex
    "https://cdn.jsdelivr.net/npm/@fontsource/ibm-plex-mono@5.0.13/files")
set(_fontsource_fraunces
    "https://cdn.jsdelivr.net/npm/@fontsource/fraunces@5.0.16/files")
set(_plex_tag    "v6.4.0")
set(_plex_legacy "https://cdn.jsdelivr.net/gh/IBM/plex@${_plex_tag}/IBM-Plex-Mono/fonts/complete/ttf")
set(_plex_raw    "https://raw.githubusercontent.com/IBM/plex/${_plex_tag}/IBM-Plex-Mono/fonts/complete/ttf")

# (style, local-name, primary-url, fallback-url, fallback-url-2)
# Three IBM Plex Mono weights + one Fraunces weight for the display title.
set(_font_specs
    "Regular|IBMPlexMono-Regular.ttf|${_fontsource_plex}/ibm-plex-mono-latin-400-normal.ttf|${_plex_legacy}/IBMPlexMono-Regular.ttf|${_plex_raw}/IBMPlexMono-Regular.ttf"
    "Bold|IBMPlexMono-Bold.ttf|${_fontsource_plex}/ibm-plex-mono-latin-700-normal.ttf|${_plex_legacy}/IBMPlexMono-Bold.ttf|${_plex_raw}/IBMPlexMono-Bold.ttf"
    "Italic|IBMPlexMono-Italic.ttf|${_fontsource_plex}/ibm-plex-mono-latin-400-italic.ttf|${_plex_legacy}/IBMPlexMono-Italic.ttf|${_plex_raw}/IBMPlexMono-Italic.ttf"
    "FrauncesSemiBold|Fraunces-SemiBold.ttf|${_fontsource_fraunces}/fraunces-latin-600-normal.ttf|${_fontsource_fraunces}/fraunces-latin-500-normal.ttf|${_fontsource_fraunces}/fraunces-latin-400-normal.ttf"
)

set(OB8_FONT_BINARIES "")
set(OB8_FONT_DOWNLOAD_OK TRUE)
foreach(_spec IN LISTS _font_specs)
    string(REPLACE "|" ";" _parts "${_spec}")
    list(GET _parts 1 _local_name)
    list(GET _parts 2 _url1)
    list(GET _parts 3 _url2)
    list(GET _parts 4 _url3)
    set(_local "${OB8_FONT_DIR}/${_local_name}")

    if(NOT EXISTS "${_local}")
        message(STATUS "Fetching bundled font ${_local_name}...")
        set(_urls "${_url1}" "${_url2}" "${_url3}")
        _ob8_try_download ("${_local}" "${_urls}")
    endif()

    if(EXISTS "${_local}")
        list(APPEND OB8_FONT_BINARIES "${_local}")
    else()
        set(OB8_FONT_DOWNLOAD_OK FALSE)
        message(WARNING "Could not fetch ${_local_name} from any mirror. "
                        "The build will continue without the bundled font; "
                        "the editor will fall back to the system mono face.")
    endif()
endforeach()

if(OB8_FONT_DOWNLOAD_OK AND OB8_FONT_BINARIES)
    juce_add_binary_data(OB8Fonts SOURCES ${OB8_FONT_BINARIES})
    target_link_libraries (OB8Native PRIVATE OB8Fonts)
    target_compile_definitions (OB8Native PRIVATE OB8_HAS_BUNDLED_FONT=1)
else()
    target_compile_definitions (OB8Native PRIVATE OB8_HAS_BUNDLED_FONT=0)
endif()

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

    # ---- Distribution package -----------------------------------------------
    # `cmake --build build --target dist` builds the Standalone and copies it
    # along with dist/install.command and dist/お読みください.txt into
    # build/dist/OB-8 Native/. The resulting folder can be zipped or sent as
    # an AirDrop bundle; the recipient double-clicks install.command once to
    # clear quarantine + ad-hoc-resign on their Mac and the .app then runs.
    if(TARGET OB8Native_Standalone)
        set(OB8_DIST_DIR "${CMAKE_BINARY_DIR}/dist/OB-8 Native")

        # We always include Standalone; VST3 and AU targets are added only
        # if their build targets exist (juce_add_plugin creates them when
        # the format is enabled in the FORMATS list).
        set(OB8_DIST_DEPS OB8Native_Standalone)
        if(TARGET OB8Native_VST3)
            list(APPEND OB8_DIST_DEPS OB8Native_VST3)
        endif()
        if(TARGET OB8Native_AU)
            list(APPEND OB8_DIST_DEPS OB8Native_AU)
        endif()

        # The custom commands have to be assembled in a list so format-
        # specific copy/sign passes can be added conditionally.
        set(OB8_DIST_COMMANDS
            COMMAND ${CMAKE_COMMAND} -E rm -rf  "${OB8_DIST_DIR}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${OB8_DIST_DIR}"
            # Standalone
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                    "$<TARGET_BUNDLE_DIR:OB8Native_Standalone>"
                    "${OB8_DIST_DIR}/OB-8 Native.app"
            COMMAND xattr -cr "${OB8_DIST_DIR}/OB-8 Native.app"
            COMMAND codesign --force --deep --sign - --timestamp=none
                             "${OB8_DIST_DIR}/OB-8 Native.app"
        )
        if(TARGET OB8Native_VST3)
            list(APPEND OB8_DIST_COMMANDS
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                        "$<TARGET_BUNDLE_DIR:OB8Native_VST3>"
                        "${OB8_DIST_DIR}/OB-8 Native.vst3"
                COMMAND xattr -cr "${OB8_DIST_DIR}/OB-8 Native.vst3"
                COMMAND codesign --force --deep --sign - --timestamp=none
                                 "${OB8_DIST_DIR}/OB-8 Native.vst3"
            )
        endif()
        if(TARGET OB8Native_AU)
            list(APPEND OB8_DIST_COMMANDS
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                        "$<TARGET_BUNDLE_DIR:OB8Native_AU>"
                        "${OB8_DIST_DIR}/OB-8 Native.component"
                COMMAND xattr -cr "${OB8_DIST_DIR}/OB-8 Native.component"
                COMMAND codesign --force --deep --sign - --timestamp=none
                                 "${OB8_DIST_DIR}/OB-8 Native.component"
            )
        endif()
        # Installer script + Japanese readme
        list(APPEND OB8_DIST_COMMANDS
            COMMAND ${CMAKE_COMMAND} -E copy
                    "${CMAKE_CURRENT_SOURCE_DIR}/dist/install.command"
                    "${OB8_DIST_DIR}/install.command"
            COMMAND ${CMAKE_COMMAND} -E copy
                    "${CMAKE_CURRENT_SOURCE_DIR}/dist/お読みください.txt"
                    "${OB8_DIST_DIR}/お読みください.txt"
            COMMAND chmod +x "${OB8_DIST_DIR}/install.command"
        )

        add_custom_target(dist
            COMMENT "Building macOS distribution bundle (Standalone + VST3 + AU) at ${OB8_DIST_DIR}"
            DEPENDS ${OB8_DIST_DEPS}
            ${OB8_DIST_COMMANDS}
            VERBATIM
        )

        # `cmake --build build --target dist-zip` produces a ready-to-send
        # ZIP of the dist bundle alongside it.
        add_custom_target(dist-zip
            COMMENT "Zipping distribution bundle"
            DEPENDS dist
            COMMAND ${CMAKE_COMMAND} -E chdir "${CMAKE_BINARY_DIR}/dist"
                    ${CMAKE_COMMAND} -E tar c "OB-8 Native.zip" --format=zip
                                              "OB-8 Native"
            VERBATIM
        )
    endif()
endif()
