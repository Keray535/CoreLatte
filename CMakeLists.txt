cmake_minimum_required(VERSION 3.22)

project(CoreLate VERSION 1.0.0)

# ── JUCE ──────────────────────────────────────────────────────────────────────
# Add JUCE as a subdirectory (place the JUCE SDK folder at ./JUCE)
add_subdirectory(JUCE)

# ── Plugin Target ─────────────────────────────────────────────────────────────
juce_add_plugin(CoreLate
    COMPANY_NAME            "YourCompany"
    PLUGIN_MANUFACTURER_CODE "YrCo"
    PLUGIN_CODE             "CrLt"
    FORMATS                 VST3
    PRODUCT_NAME            "CoreLate"
    IS_SYNTH                FALSE
    NEEDS_MIDI_INPUT        FALSE
    NEEDS_MIDI_OUTPUT       FALSE
    IS_MIDI_EFFECT          FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES         "Analyzer"
)

# ── JuceHeader.h ──────────────────────────────────────────────────────────────
# Sources include <JuceHeader.h>, so the header must be generated.
juce_generate_juce_header(CoreLate)

# ── Source Files ──────────────────────────────────────────────────────────────
target_sources(CoreLate
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
)

# ── Compile Definitions ───────────────────────────────────────────────────────
target_compile_definitions(CoreLate
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
)

# ── JUCE Module Dependencies ──────────────────────────────────────────────────
target_link_libraries(CoreLate
    PRIVATE
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_plugin_client
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_core
        juce::juce_data_structures
        juce::juce_dsp
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# ── C++ Standard ──────────────────────────────────────────────────────────────
target_compile_features(CoreLate PRIVATE cxx_std_17)
