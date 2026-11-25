cmake_minimum_required(VERSION 3.22)
project(ElMaestroSaichan VERSION 1.0.0)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(ElMaestroSaichan
    COMPANY_NAME "Saichan"
    PRODUCT_NAME "EL MAESTRO SAICHAN"
    PLUGIN_MANUFACTURER_CODE Sach
    PLUGIN_CODE Mstr
    FORMATS VST3
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    PLUGIN_NAME "EL MAESTRO SAICHAN"
    VST3_CATEGORIES Fx Dynamics
    NEEDS_WEBVIEW2 FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
)

juce_add_binary_data (SaichanAssets SOURCES
    assets/fede_rodriguez.jpg
    assets/logo.png)

target_sources (ElMaestroSaichan PRIVATE
    PluginProcessor.cpp
    PluginEditor.cpp)

target_compile_definitions (ElMaestroSaichan PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries (ElMaestroSaichan PRIVATE
    SaichanAssets
    juce::juce_audio_basics
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_audio_plugin_client)

target_compile_features (ElMaestroSaichan PRIVATE cxx_std_17)
