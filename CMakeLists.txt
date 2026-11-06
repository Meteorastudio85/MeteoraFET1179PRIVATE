cmake_minimum_required(VERSION 3.22)
project(MeteoraFET1179 VERSION 0.2.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.6
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(MeteoraFET1179
    COMPANY_NAME "Meteora Plugins"
    PLUGIN_MANUFACTURER_CODE Mtra
    PLUGIN_CODE Mf79
    FORMATS VST3
    PRODUCT_NAME "meteoraFET1179"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Fx Dynamics)

juce_add_binary_data(MeteoraFET1179Assets SOURCES Assets/ui.jpg Assets/logo_glow.png Assets/lever_on.png Assets/lever_off.png Assets/vu_glass.png)

target_sources(MeteoraFET1179 PRIVATE Source/PluginProcessor.cpp Source/PluginEditor.cpp)
target_compile_definitions(MeteoraFET1179 PUBLIC
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(MeteoraFET1179
    PRIVATE MeteoraFET1179Assets juce::juce_audio_utils juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags
           juce::juce_recommended_warning_flags)
