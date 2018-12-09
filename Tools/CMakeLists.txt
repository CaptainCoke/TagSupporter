cmake_minimum_required(VERSION 2.8)

file(GLOB ToolSources
    "*.h"
    "*.cpp"
)

target_sources(TagSupporter PUBLIC "${ToolSources}" )