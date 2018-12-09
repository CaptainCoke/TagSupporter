cmake_minimum_required(VERSION 2.8)

file(GLOB OnlineParserSources
    "*.h"
    "*.cpp"
)

target_sources(TagSupporter PUBLIC "${OnlineParserSources}" )