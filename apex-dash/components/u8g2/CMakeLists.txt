file(GLOB_RECURSE U8G2_CSRCS "csrc/*.c")
file(GLOB_RECURSE U8G2_CPPSRCS "cppsrc/*.cpp")

idf_component_register(
    SRCS ${U8G2_CSRCS} ${U8G2_CPPSRCS}
    INCLUDE_DIRS "csrc" "cppsrc"
)
