set(target_name sequencer_gpio)

add_library(${target_name} STATIC
    ${target_name}.c
)

target_link_libraries(${target_name} pico_stdlib)
