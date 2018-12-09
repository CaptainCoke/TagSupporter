cmake_minimum_required(VERSION 2.8)

file(GLOB WidgetSources
    "*.h"
    "*.cpp"
    "*.ui"
)

target_sources(TagSupporter PUBLIC "${WidgetSources}" )